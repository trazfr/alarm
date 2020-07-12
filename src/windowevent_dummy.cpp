#include "windowevent_dummy.hpp"

#include "event.hpp"

#include <ostream>

WindowEventDummy::~WindowEventDummy() = default;

std::optional<Event> WindowEventDummy::popEvent()
{
    return {};
}

std::ostream &WindowEventDummy::toStream(std::ostream &str) const
{
    return str << "WindowEventDummy";
}
