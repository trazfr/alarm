#include "window_raspberrypi_hdmi.hpp"

#ifdef PLATFORM_RASPBERRYPI

#include "event.hpp"

#include <ostream>

WindowRaspberryPiHdmi::WindowRaspberryPiHdmi(int width, int height)
    : WindowRaspberryPi{width, height, true}
{
}

WindowRaspberryPiHdmi::~WindowRaspberryPiHdmi() = default;

std::optional<Event> WindowRaspberryPiHdmi::popEvent()
{
    std::optional<Event> result;
    return result;
}

std::ostream &WindowRaspberryPiHdmi::toStream(std::ostream &str) const
{
    str << "To HDMI\n";
    return WindowRaspberryPi::toStream(str);
}

#endif // PLATFORM_RASPBERRYPI