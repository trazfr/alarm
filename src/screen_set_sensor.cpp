#include "screen_set_sensor.hpp"

#include "config.hpp"
#include "context.hpp"
#include "renderer.hpp"
#include "renderer_sprite.hpp"
#include "renderer_text.hpp"
#include "sensor.hpp"
#include "sensor_factory.hpp"
#include "toolbox_i18n.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
constexpr size_t kInvalidSensorIdx = -1;
constexpr size_t kSensorNameSize = 20;
constexpr int kThermalSize = 7;
} // namespace

struct ScreenSetSensor::Impl
{
    explicit Impl(Renderer &renderer)
        : arrowUp{renderer.renderSprite(Asset::Arrow, renderer.getWidth() / 2, renderer.getHeight(), Position::Up)},
          arrowDown{renderer.renderSprite(Asset::Arrow, renderer.getWidth() / 2, 0, Position::Down, 2)},
          previousScreen{renderer.renderStaticText(kMargin, renderer.getHeight() - kMargin, _("Config\nDate"), Position::UpLeft, 16)},
          nextScreen{renderer.renderStaticText(renderer.getWidth() - kMargin, renderer.getHeight() - kMargin, _("Config"), Position::UpRight, 16)},
          thermalNameText{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() / 2, kSensorNameSize, 1, Position::Center)},
          thermalValueText{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() * 2 / 5, kThermalSize, 1, Position::Up, 16)},
          sensorNone{_("       <None>")}
    {
    }

    void refreshThermal(float value)
    {
        if (std::abs(value - savedThermalValue) > 0.1)
        {
            char buffer[kThermalSize + 1];
            std::sprintf(buffer, "%5.1f" DEGREE "C", value);
            if (std::strlen(buffer) == kThermalSize)
            {
                thermalValueText.set(buffer);
            }
            else
            {
                thermalValueText.set(" ?TEMP?");
            }
            savedThermalValue = value;
        }
    }

    void refreshThermalName(const Sensor *sensor)
    {
        if (sensor != savedThermalName)
        {
            std::string_view filename = sensor->getName();
            if (filename.size() > kSensorNameSize)
            {
                filename.remove_suffix(filename.size() - kSensorNameSize);
            }

            char buffer[2 * kSensorNameSize + 1];
            std::memset(buffer, ' ', sizeof(buffer) - 1);
            std::memcpy(buffer + kSensorNameSize, filename.data(), filename.size());
            const size_t missingLen = kSensorNameSize - filename.size();
            char *ptr = buffer + kSensorNameSize - missingLen / 2;
            ptr[kSensorNameSize] = '\0';

            thermalNameText.set(ptr);
            savedThermalName = sensor;
        }
    }

    RendererSprite arrowUp;
    RendererSprite arrowDown;

    RendererTextStatic previousScreen;
    RendererTextStatic nextScreen;

    RendererText thermalNameText;
    RendererText thermalValueText;

    float savedThermalValue = -1000;
    const Sensor *savedThermalName = nullptr;
    size_t sensorIdx = kInvalidSensorIdx;

    const char *sensorNone;
};

ScreenSetSensor::ScreenSetSensor(Context &ctx)
    : Screen{ctx}
{
}

ScreenSetSensor::~ScreenSetSensor() = default;

void ScreenSetSensor::enter()
{
    pimpl = std::make_unique<Impl>(ctx.getRenderer());

    SensorFactory &factory = ctx.getSensorFactory();
    for (size_t i = factory.getSize(SensorFactory::Type::Temperature); i--;)
    {
        if (factory.get(SensorFactory::Type::Temperature, i) == ctx.getTemperatureSensor())
        {
            pimpl->sensorIdx = i;
            break;
        }
    }
}

void ScreenSetSensor::leave()
{
    pimpl = nullptr;
}

void ScreenSetSensor::run(const Clock::time_point &)
{
    SensorFactory &factory = ctx.getSensorFactory();

    pimpl->previousScreen.print();
    pimpl->nextScreen.print();

    if (Sensor *sensor = factory.get(SensorFactory::Type::Temperature, pimpl->sensorIdx))
    {
        pimpl->refreshThermalName(sensor);
        pimpl->thermalNameText.print();

        pimpl->refreshThermal(sensor->get());
        pimpl->thermalValueText.print();
    }
    else
    {
        pimpl->savedThermalName = nullptr;
        pimpl->thermalNameText.set(pimpl->sensorNone);
    }

    pimpl->thermalNameText.print();
    if (factory.get(SensorFactory::Type::Temperature, pimpl->sensorIdx + 1))
    {
        pimpl->arrowUp.print();
    }
    if (pimpl->sensorIdx != kInvalidSensorIdx)
    {
        pimpl->arrowDown.print();
    }
}

void ScreenSetSensor::handleClick(Position position)
{
    switch (position)
    {
    case Position::UpLeft:
        ctx.previousScreen();
        break;
    case Position::UpRight:
        ctx.nextScreen();
        break;
    case Position::Up:
        if (const auto sensor = ctx.getSensorFactory().get(SensorFactory::Type::Temperature, pimpl->sensorIdx + 1))
        {
            ++(pimpl->sensorIdx);
            ctx.getConfig().setSensorThermal(sensor->getName());
            ctx.resetSensors();
        }
        break;
    case Position::Down:
        if (const auto sensor = ctx.getSensorFactory().get(SensorFactory::Type::Temperature, pimpl->sensorIdx - 1))
        {
            --(pimpl->sensorIdx);
            ctx.getConfig().setSensorThermal(sensor->getName());
            ctx.resetSensors();
        }
        else
        {
            pimpl->sensorIdx = kInvalidSensorIdx;
            ctx.getConfig().setSensorThermal({});
            ctx.resetSensors();
        }
        break;
    default:
        break;
    }
}
