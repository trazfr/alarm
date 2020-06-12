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

    Window &create(std::string_view driver, int width, int height);
    Window &get() const;

    size_t getDriverSize() const;
    std::string_view getDriver(size_t index) const;

    friend std::ostream &operator<<(std::ostream &str, const WindowFactory &obj)
    {
        return obj.toStream(str);
    }

private:
    std::ostream &toStream(std::ostream &str) const;

    std::unique_ptr<Window> window;
};
