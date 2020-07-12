#include "windowevent_ringbuffer.hpp"

#include "event.hpp"

#include <iostream>

WindowEventRingBuffer::WindowEventRingBuffer(Storage &events)
    : events{events}
{
}

WindowEventRingBuffer::~WindowEventRingBuffer() = default;

std::optional<Event> WindowEventRingBuffer::popEvent()
{
    return events.pop();
}

std::ostream &WindowEventRingBuffer::toStream(std::ostream &str) const
{
    return str << "WindowEventRingBuffer";
}
