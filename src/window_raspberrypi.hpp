#pragma once

#include "window.hpp"

#ifndef NO_WINDOW_RASPBERRYPI

#include <memory>

/**
 * @brief Base class for HDMI or TFT output
 */
class WindowRaspberryPi : public Window
{
public:
    struct Impl;

    void begin() override;
    void end() override;

protected:
    explicit WindowRaspberryPi(int width, int height, bool fullScreen);
    ~WindowRaspberryPi() override;

    uint32_t getDispmanxDisplay() const;

    std::ostream &toStream(std::ostream &str) const override;

private:
    std::unique_ptr<Impl> pimpl;
};

#endif // NO_WINDOW_RASPBERRYPI
