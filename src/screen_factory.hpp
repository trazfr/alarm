#pragma once

#include <memory>

class Context;
class Screen;

enum class ScreenType
{
    Main,
    SetAlarm,
    SetAlarmFile,
    SetDate,
    SetSensor,
    HandleConfig,
};

/**
 * @brief Factory for Screen. All screens are pre-allocated at startup
 *
 * @sa Screen
 */
class ScreenFactory
{
public:
    struct Impl;

    ScreenFactory(Context &context);
    ~ScreenFactory();

    Screen *get(ScreenType type);

private:
    std::unique_ptr<Impl> pimpl;
};
