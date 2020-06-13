#pragma once

#include <iosfwd>
#include <memory>
#include <string>

class Window;

/**
 * @brief Create a Window Object given its driver
 */
class WindowFactory
{
public:
    WindowFactory();
    ~WindowFactory();

    /**
     * Create a window given the driver name
     */
    Window &create(std::string_view driver, int width, int height);

    /**
     * Get the current window
     */
    Window &get() const;

    /**
     * Remove the current window, removing the OpenGL context
     */
    void clear();

    /**
     * Get the number of compiled drivers. To be used with getDriver()
     * 
     * @sa getDriver()
     */
    size_t getDriverSize() const;

    /**
     * Get the driver name corresponding to the index
     * 
     * @sa getDriverSize()
     */
    std::string_view getDriver(size_t index) const;

    friend std::ostream &operator<<(std::ostream &str, const WindowFactory &obj)
    {
        return obj.toStream(str);
    }

private:
    std::ostream &toStream(std::ostream &str) const;

    std::unique_ptr<Window> window;
};
