#include "window_factory.hpp"

#include "window_raspberrypi_dispmanx.hpp"
#include "window_raspberrypi_framebuffer.hpp"
#include "window_sdl.hpp"
#include "window_wayland.hpp"

#include <algorithm>
#include <memory>
#include <ostream>

namespace
{

/**
 * Helper to create a window to have a single signature
 * 
 * @sa CreateFunction_t
 */
template <typename W>
std::unique_ptr<Window> createWindow(int width, int height)
{
    return std::make_unique<W>(width, height);
}
/// Function signature for kDrivers
using CreateFunction_t = std::unique_ptr<Window> (*)(int, int);

/// Stores all the compiled drivers. The 1st is the default
constexpr std::pair<const char *, CreateFunction_t> kDrivers[] = {
#ifndef NO_WINDOW_RASPBERRYPI
    {"raspberrypi_framebuffer", &createWindow<WindowRaspberryPiFramebuffer>},
    {"raspberrypi_dispmanx", &createWindow<WindowRaspberryPiDispmanx>},
#endif
#ifndef NO_WINDOW_SDL
    {"sdl", &createWindow<WindowSDL>},
#endif
#ifndef NO_WINDOW_WAYLAND
    {"wayland", &createWindow<WindowWayland>},
#endif
};

static_assert(sizeof(kDrivers) > 0, "There must be at least 1 driver");

CreateFunction_t getCreateFunction(std::string_view driver)
{
    for (const auto &item : kDrivers)
    {
        if (item.first == driver)
        {
            return item.second;
        }
    }
    return nullptr;
}

} // namespace

WindowFactory::WindowFactory() = default;
WindowFactory::~WindowFactory() = default;

Window &WindowFactory::create(std::string_view driver, int width, int height)
{
    clear();

    const auto function = getCreateFunction(driver);
    if (function == nullptr)
    {
        throw std::runtime_error{"Unknown driver: " + std::string{driver}};
    }
    window = (*function)(width, height);
    return *window;
}

Window &WindowFactory::get() const
{
    if (window == nullptr)
    {
        throw std::runtime_error{"No window created"};
    }
    return *window;
}

void WindowFactory::clear()
{
    window = nullptr;
}

size_t WindowFactory::getDriverSize() const
{
    return sizeof(kDrivers) / sizeof(*kDrivers);
}

std::string_view WindowFactory::getDriver(size_t index) const
{
    if (index >= getDriverSize())
    {
        throw std::runtime_error{"Invalid driver index: " + std::to_string(index)};
    }
    return kDrivers[index].first;
}

std::ostream &WindowFactory::toStream(std::ostream &str) const
{
    str << "Drivers:";
    for (size_t index = 0; index < getDriverSize(); ++index)
    {
        str << "\n - " << getDriver(index);
    }
    return str;
}
