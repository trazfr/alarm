#pragma once

#include "window.hpp"

#ifndef NO_WINDOW_RASPBERRYPI

#include <memory>

/**
 * @brief Window to output to the TFT SPI port
 * 
 * Raspberry PI only
 */
class WindowRaspberryPiFramebuffer : public Window
{
public:
    struct Impl;

    explicit WindowRaspberryPiFramebuffer(int width, int height);
    ~WindowRaspberryPiFramebuffer() override;

    void begin() override;
    void end() override;

    /**
     * Not implemented
     */
    std::optional<Event> popEvent() override;

protected:
    std::ostream &toStream(std::ostream &str) const override;

private:
    std::unique_ptr<Impl> pimpl;
};

#endif // NO_WINDOW_RASPBERRYPI
