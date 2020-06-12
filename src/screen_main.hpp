#pragma once

#include "screen.hpp"

#include <memory>

/**
 * @brief Main screen with clock, date...
 */
class ScreenMain : public Screen
{
public:
    struct Impl;

    explicit ScreenMain(Context &ctx);
    ~ScreenMain() override;

private:
    void enter() override;
    void leave() override;
    void run(const Clock::time_point &time) override;
    void handleClick(Position position) override;

    std::unique_ptr<Impl> pimpl;
};
