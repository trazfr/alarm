#pragma once

#include <iosfwd>
#include <optional>

class Event;

/**
 * @brief base class for Windows
 */
class Window
{
public:
    virtual ~Window() = 0;

    /**
     * To be called before any call to Renderer class
     */
    virtual void begin() = 0;

    /**
     * To be called after all calls to Renderer class
     */
    virtual void end() = 0;

    /**
     * Pop the next event
     * 
     * If the std::optional is invalid, there is no event
     */
    virtual std::optional<Event> popEvent() = 0;

    friend std::ostream &operator<<(std::ostream &str, const Window &window)
    {
        return window.toStream(str);
    }

protected:
    virtual std::ostream &toStream(std::ostream &str) const = 0;
};
