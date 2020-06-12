#include "event.hpp"

Event Event::createQuit()
{
    Event e;
    e.type = EventType::Quit;
    return e;
}

Event Event::createClick(float x, float y)
{
    Event e;
    e.type = EventType::Click;
    e.click.x = x;
    e.click.y = y;
    return e;
}
