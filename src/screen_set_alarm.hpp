#pragma once

#include "screen.hpp"

#include <memory>

/**
 * @brief Screen to create or remove alarms, change their time
 */
class ScreenSetAlarm : public Screen
{
public:
    struct Impl;

    explicit ScreenSetAlarm(Context &ctx);
    ~ScreenSetAlarm() override;

private:
    void enter() override;
    void leave() override;
    void run(const Clock::time_point &time) override;
    void handleClick(Position position) override;

    std::unique_ptr<Impl> pimpl;
};
