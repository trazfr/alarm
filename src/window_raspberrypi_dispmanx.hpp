#pragma once

#include "window.hpp"

#ifndef NO_WINDOW_RASPBERRYPI

#include <memory>

/**
 * @brief Base class for HDMI or TFT output
 */
class WindowRaspberryPiDispmanx : public Window
{
public:
    struct Impl;

    WindowRaspberryPiDispmanx(int width, int height);
    ~WindowRaspberryPiDispmanx() override;

    void begin() override;
    void end() override;

    /**
     * Not implemented
     */
    std::optional<Event> popEvent() override;

protected:
    uint32_t getDispmanxDisplay() const;

    std::ostream &toStream(std::ostream &str) const override;

private:
    std::unique_ptr<Impl> pimpl;
};

#endif // NO_WINDOW_RASPBERRYPI
