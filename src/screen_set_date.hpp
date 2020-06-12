#pragma once

#include "screen.hpp"

#include <memory>

/**
 * @brief Change the date of the computer (alarm clock intended to run without network)
 * 
 * This needs CAP_SYS_TIME privilege
 * 
 * @sa man 2 stime
 */
class ScreenSetDate : public Screen
{
public:
    struct Impl;

    explicit ScreenSetDate(Context &ctx);
    ~ScreenSetDate() override;

private:
    void enter() override;
    void leave() override;
    void run(const Clock::time_point &time) override;
    void handleClick(Position position) override;

    std::unique_ptr<Impl> pimpl;
};
