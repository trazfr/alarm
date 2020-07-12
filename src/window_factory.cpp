#include "window_factory.hpp"

#include "window_raspberrypi_dispmanx.hpp"
#include "window_raspberrypi_framebuffer.hpp"
#include "window_sdl.hpp"
#include "window_wayland.hpp"
#include "windowevent_dummy.hpp"
#include "windowevent_linux.hpp"
#include "windowevent_ringbuffer.hpp"

#include <algorithm>
#include <iostream>
#include <memory>

namespace
{

constexpr char kEventDefault[] = "default";
constexpr char kEventDummy[] = "dummy";
constexpr char kEventLinux[] = "linux";

/**
 * Helper to create a window to have a single signature as std::make_unique<> is not ok
 */
template <typename W>
std::unique_ptr<Window> createWindow(int width, int height)
{
    return std::make_unique<W>(width, height);
}

/**
 * Type for kDrivers:
 * std::tuple<name, create function, default eventDriver>
 */
using DriverData = std::tuple<const char *, std::unique_ptr<Window> (*)(int, int)>;

/**
 * Stores all the compiled drivers. The 1st is the default
 */
constexpr DriverData kDrivers[] = {
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

const DriverData *getDriverData(std::string_view driver)
{
    for (const auto &item : kDrivers)
    {
        if (std::get<0>(item) == driver)
        {
            return &item;
        }
    }
    return nullptr;
}

/**
 * Helper to create a window to have a single signature as std::make_unique<> is not ok
 */
template <typename W>
std::unique_ptr<WindowEvent> createWindowEvent(Window &)
{
    return std::make_unique<W>();
}

std::unique_ptr<WindowEvent> createDefaultEvent(Window &window)
{
    return window.createDefaultEvent();
}

/**
 * Function signature for kEventDrivers
 */
using EventData = std::tuple<const char *, std::unique_ptr<WindowEvent> (*)(Window &)>;

/// Stores all the event drivers. The 1st is the default
constexpr EventData kEventDrivers[] = {
    {kEventDefault, &createDefaultEvent},
    {kEventDummy, &createWindowEvent<WindowEventDummy>},
    {kEventLinux, &createWindowEvent<WindowEventLinux>},
};

const EventData *getEventData(std::string_view driver)
{
    for (const auto &item : kEventDrivers)
    {
        // std::string_view::starts_with() is C++20
        if (driver.find(std::get<0>(item)) == 0)
        {
            return &item;
        }
    }
    std::cerr << "Unknown event_driver: " << driver << std::endl;
    return kEventDrivers;
}

} // namespace

WindowFactory::WindowFactory() = default;
WindowFactory::~WindowFactory() = default;

void WindowFactory::create(std::string_view displayDriver, std::string_view eventDriver, int width, int height)
{
    clear();

    const auto driverData = getDriverData(displayDriver);
    if (driverData == nullptr)
    {
        throw std::runtime_error{"Unknown display_driver: " + std::string{displayDriver}};
    }
    const auto createWindow = std::get<1>(*driverData);
    window = (*createWindow)(width, height);

    const auto eventData = getEventData(eventDriver);
    const auto createEvent = std::get<1>(*eventData);
    windowEvent = (*createEvent)(*window);
}

Window &WindowFactory::get() const
{
    if (window == nullptr)
    {
        throw std::runtime_error{"No window created"};
    }
    return *window;
}

WindowEvent &WindowFactory::getEvent() const
{
    if (windowEvent == nullptr)
    {
        throw std::runtime_error{"No event created"};
    }
    return *windowEvent;
}

void WindowFactory::clear()
{
    window = nullptr;
    windowEvent = nullptr;
}

size_t WindowFactory::getDriverSize()
{
    return sizeof(kDrivers) / sizeof(*kDrivers);
}

std::string_view WindowFactory::getDriver(size_t index)
{
    if (index >= getDriverSize())
    {
        throw std::runtime_error{"Invalid driver index: " + std::to_string(index)};
    }
    return std::get<0>(kDrivers[index]);
}

std::ostream &WindowFactory::toStream(std::ostream &str) const
{
    str << "Drivers:";
    for (size_t index = 0; index < getDriverSize(); ++index)
    {
        str << "\n - " << getDriver(index);
    }
    str << "\nEvent Drivers:";
    for (const auto event : kEventDrivers)
    {
        str << "\n - " << std::get<0>(event);
    }
    return str;
}
