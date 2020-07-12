#pragma once

#include "windowevent.hpp"

#include "toolbox_ringbuffer.hpp"

/**
 * @brief Get events from SDL2 / Wayland
 */
class WindowEventRingBuffer : public WindowEvent
{
public:
    using Storage = RingBuffer<Event, 64>;

    explicit WindowEventRingBuffer(Storage &events);
    ~WindowEventRingBuffer() override;

    std::optional<Event> popEvent() override;

protected:
    std::ostream &toStream(std::ostream &str) const override;

    Storage &events;
};
