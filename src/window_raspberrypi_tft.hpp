#pragma once

#include "window_raspberrypi.hpp"

#ifndef NO_WINDOW_RASPBERRYPI

/**
 * @brief Window to output to the TFT SPI port
 * 
 * Raspberry PI only
 * 
 * @todo receive a TFT screen to actually test it
 */
class WindowRaspberryPiTft : public WindowRaspberryPi
{
public:
    struct Impl;

    explicit WindowRaspberryPiTft(int width, int height);
    ~WindowRaspberryPiTft() override;

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
