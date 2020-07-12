#pragma once

#include <iosfwd>
#include <optional>

struct Event;

/**
 * @brief base class for input
 */
class WindowEvent
{
public:
    virtual ~WindowEvent() = 0;
    /**
     * Pop the next event
     *
     * If the std::optional is invalid, there is no event
     */
    virtual std::optional<Event> popEvent() = 0;

    friend std::ostream &operator<<(std::ostream &str, const WindowEvent &windowEvent)
    {
        return windowEvent.toStream(str);
    }

protected:
    virtual std::ostream &toStream(std::ostream &str) const = 0;
};
