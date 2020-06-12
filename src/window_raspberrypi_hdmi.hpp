#pragma once

#include "window_raspberrypi.hpp"

#ifdef PLATFORM_RASPBERRYPI

/**
 * @brief Window to output to the HDMI port
 * 
 * Raspberry PI only
 */
class WindowRaspberryPiHdmi : public WindowRaspberryPi
{
public:
    explicit WindowRaspberryPiHdmi(int width, int height);
    ~WindowRaspberryPiHdmi() override;

    /**
     * Not implemented
     */
    std::optional<Event> popEvent() override;

protected:
    std::ostream &toStream(std::ostream &str) const override;
};

#endif // PLATFORM_RASPBERRYPI
