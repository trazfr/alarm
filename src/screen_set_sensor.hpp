#pragma once

#include "screen.hpp"

#include <memory>

/**
 * @brief Choose the sensor to be displayed on the main screen
 */
class ScreenSetSensor : public Screen
{
public:
    /// Private implementation
    struct Impl;

    explicit ScreenSetSensor(Context &ctx);
    ~ScreenSetSensor() override;

private:
    void enter() override;
    void leave() override;
    void run(const Clock::time_point &time) override;
    void handleClick(Position position) override;

    std::unique_ptr<Impl> pimpl;
};
