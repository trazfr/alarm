#include "context.hpp"

#include "alarm.hpp"
#include "audio.hpp"
#include "config.hpp"
#include "config_alarm.hpp"
#include "screen.hpp"
#include "screen_factory.hpp"
#include "sensor.hpp"
#include "sensor_factory.hpp"
#include "serializer_rapidjson.hpp"

#include <cassert>
#include <iostream>

struct Context::Impl
{
    Impl(Context &ctx, Config &config, SerializationHandler &configPersistence, Renderer &renderer)
        : config{config},
          configPersistence{configPersistence},
          renderer{renderer},
          audio{config.getAlsaDevice()},
          alarm{config, audio},
          screenFactory{ctx}
    {
    }

    Config &config;
    SerializationHandler &configPersistence;
    Renderer &renderer;
    Audio audio;

    Alarm alarm;
    SensorFactory sensorFactory;
    ScreenFactory screenFactory;

    ScreenType screenType = ScreenType::Main;
    size_t thermalSensor = -1;
};

namespace
{

void setScreen(Context::Impl &pimpl, ScreenType newType)
{
    if (Screen *newScreen = pimpl.screenFactory.get(newType))
    {
        if (Screen *oldScreen = pimpl.screenFactory.get(pimpl.screenType))
        {
            oldScreen->leave();
        }
        newScreen->enter();
        pimpl.screenType = newType;
    }
    else
    {
        std::cerr << "Unknown screen: " << static_cast<int>(newType) << std::endl;
    }
}

} // namespace

Context::Context(Config &config, SerializationHandler &configPersistence, Renderer &renderer)
    : pimpl{std::make_unique<Impl>(*this, config, configPersistence, renderer)}
{
    resetSensors();
    std::cerr << getSensorFactory();

    if (const auto sensor = getTemperatureSensor())
    {
        std::cerr << "Using " << sensor->getName() << std::endl;
    }
    else if (const auto sensor = getConfig().getSensorThermal(); !sensor.empty())
    {
        std::cerr << "Sensor " << sensor << " not found" << std::endl;
    }
    setScreen(*pimpl, ScreenType::Main);
}

Context::~Context() = default;

Renderer &Context::getRenderer()
{
    return pimpl->renderer;
}

Audio &Context::getAudio()
{
    return pimpl->audio;
}

Config &Context::getConfig()
{
    return pimpl->config;
}

const Config &Context::getConfig() const
{
    return pimpl->config;
}

void Context::saveConfig()
{
    std::cerr << "Save config" << std::endl;
    pimpl->configPersistence.save(pimpl->config);
}

void Context::loadConfig()
{
    std::cerr << "Load config" << std::endl;
    getAlarm().reset();
    pimpl->configPersistence.load(pimpl->config);
}

Alarm &Context::getAlarm()
{
    return pimpl->alarm;
}

void Context::newAlarm()
{
    getConfig().getAlarms().emplace_back();
}

void Context::deleteAlarm(ConfigAlarm &alarm)
{
    getAlarm().reset();

    auto &alarms = getConfig().getAlarms();
    for (auto it = alarms.begin(); it != alarms.end(); ++it)
    {
        if (&(*it) == &alarm)
        {
            alarms.erase(it);
            return;
        }
    }
}

Screen &Context::getScreen()
{
    Screen *const screen = pimpl->screenFactory.get(pimpl->screenType);
    assert(screen);
    return *screen;
}

void Context::previousScreen()
{
    setScreen(*pimpl, ScreenType{static_cast<int>(pimpl->screenType) - 1});
}

void Context::nextScreen()
{
    setScreen(*pimpl, ScreenType{static_cast<int>(pimpl->screenType) + 1});
}

void Context::resetSensors()
{
    pimpl->thermalSensor = -1;
    const auto thermalSensor = getConfig().getSensorThermal();
    for (size_t i = pimpl->sensorFactory.getSize(SensorFactory::Type::Temperature); i--;)
    {
        if (const auto sensor = pimpl->sensorFactory.get(SensorFactory::Type::Temperature, i))
        {
            if (sensor->getName() == thermalSensor)
            {
                pimpl->thermalSensor = i;
            }
        }
    }
}

SensorFactory &Context::getSensorFactory()
{
    return pimpl->sensorFactory;
}

Sensor *Context::getTemperatureSensor()
{
    return getSensorFactory().get(SensorFactory::Type::Temperature, pimpl->thermalSensor);
}

void Context::handleClick(float x, float y)
{
    getScreen().handleClick(getPositionFromCoordinates(x, y));
}

void Context::run(const Clock::time_point &time)
{
    pimpl->alarm.run(time);
    getScreen().run(time);
    if (const auto sensor = getTemperatureSensor())
    {
        sensor->refresh(time);
    }
}
