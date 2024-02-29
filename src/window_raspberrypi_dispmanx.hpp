#pragma once

#include "window.hpp"

#ifdef USE_WINDOW_DISPMANX

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
     * WindowEventLinux
     */
    std::unique_ptr<WindowEvent> createDefaultEvent() override;

protected:
    std::ostream &toStream(std::ostream &str) const override;

private:
    std::unique_ptr<Impl> pimpl;
};

#endif // USE_WINDOW_DISPMANX
