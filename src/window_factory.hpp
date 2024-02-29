#pragma once

#include <iosfwd>
#include <memory>
#include <string>

class Window;
class WindowEvent;

/**
 * @brief Create a Window and WindowEvent objects given their driver name
 */
class WindowFactory
{
public:
    WindowFactory();
    ~WindowFactory();

    /**
     * Create a window given the driver name
     */
    void create(std::string_view displayDriver, std::string_view eventDriver, int width, int height);

    /**
     * Get the current window
     */
    Window &get() const;

    /**
     * Get the current event
     */
    WindowEvent &getEvent() const;

    /**
     * Remove the current window, removing the OpenGL context
     */
    void clear();

    /**
     * Get the number of compiled drivers. To be used with getDriver()
     *
     * @sa getDriver()
     */
    static size_t getDriverSize();

    /**
     * Get the driver name corresponding to the index
     *
     * @sa getDriverSize()
     */
    static std::string_view getDriver(size_t index);

    friend std::ostream &operator<<(std::ostream &str, const WindowFactory &obj)
    {
        return obj.toStream(str);
    }

private:
    std::ostream &toStream(std::ostream &str) const;

    std::unique_ptr<Window> window;
    std::unique_ptr<WindowEvent> windowEvent;
};
