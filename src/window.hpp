#pragma once

#include <iosfwd>
#include <memory>
#include <optional>

class WindowEvent;

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
     * Create the events from the WindowManager
     */
    virtual std::unique_ptr<WindowEvent> createDefaultEvent() = 0;

    friend std::ostream &operator<<(std::ostream &str, const Window &window)
    {
        return window.toStream(str);
    }

protected:
    virtual std::ostream &toStream(std::ostream &str) const = 0;
};
