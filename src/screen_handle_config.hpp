#pragma once

#include "screen.hpp"

#include <memory>

/**
 * @brief Screen to save / load the configuration
 */
class ScreenHandleConfig : public Screen
{
public:
    struct Impl;
    explicit ScreenHandleConfig(Context &ctx);
    ~ScreenHandleConfig() override;

private:
    void enter() override;
    void leave() override;
    void run(const Clock::time_point &time) override;
    void handleClick(Position position) override;

    std::unique_ptr<Impl> pimpl;
};
