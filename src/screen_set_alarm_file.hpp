#pragma once

#include "screen.hpp"

#include <memory>

/**
 * @brief For a given alarm, change the file on the hard drive
 */
class ScreenSetAlarmFile : public Screen
{
public:
    struct Impl;

    explicit ScreenSetAlarmFile(Context &ctx);
    ~ScreenSetAlarmFile() override;

private:
    void enter() override;
    void leave() override;
    void run(const Clock::time_point &time) override;
    void handleClick(Position position) override;

    std::unique_ptr<Impl> pimpl;
};
