#pragma once

#include "windowevent.hpp"

#include <memory>

/**
 * @brief Get events from /dev/input/event*
 *
 * It uses either a touchscreen or a touchpad with absolute position
 */
class WindowEventLinux : public WindowEvent
{
public:
    struct Impl;

    explicit WindowEventLinux();
    ~WindowEventLinux() override;

    std::optional<Event> popEvent() override;

protected:
    std::ostream &toStream(std::ostream &str) const override;

    std::unique_ptr<Impl> pimpl;
};
