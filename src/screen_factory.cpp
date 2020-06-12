#include "screen_factory.hpp"

#include "screen.hpp"

#include "screen_handle_config.hpp"
#include "screen_main.hpp"
#include "screen_set_alarm.hpp"
#include "screen_set_alarm_file.hpp"
#include "screen_set_date.hpp"
#include "screen_set_sensor.hpp"

struct ScreenFactory::Impl
{
    explicit Impl(Context &context)
        : main{context},
          setAlarm{context},
          setAlarmFile{context},
          setSensor{context},
          setDate{context},
          handleConfig{context}
    {
    }

    ScreenMain main;
    ScreenSetAlarm setAlarm;
    ScreenSetAlarmFile setAlarmFile;
    ScreenSetSensor setSensor;
    ScreenSetDate setDate;
    ScreenHandleConfig handleConfig;
};

ScreenFactory::ScreenFactory(Context &context)
    : pimpl{std::make_unique<Impl>(context)}
{
}

ScreenFactory::~ScreenFactory() = default;

Screen *ScreenFactory::get(ScreenType type)
{
    switch (type)
    {
    case ScreenType::Main:
        return &pimpl->main;
    case ScreenType::SetAlarm:
        return &pimpl->setAlarm;
    case ScreenType::SetAlarmFile:
        return &pimpl->setAlarmFile;
    case ScreenType::SetDate:
        return &pimpl->setDate;
    case ScreenType::SetSensor:
        return &pimpl->setSensor;
    case ScreenType::HandleConfig:
        return &pimpl->handleConfig;

    default:
        break;
    }

    return nullptr;
}
