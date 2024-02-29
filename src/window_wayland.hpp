#pragma once

#include "window.hpp"

#ifdef USE_WINDOW_WAYLAND

#include <memory>

class WindowEvent;

/**
 * @brief Very basic Window using Wayland with an OpenGL context
 */
class WindowWayland : public Window
{
public:
    struct Impl;

    explicit WindowWayland(int width, int height);
    ~WindowWayland() override;

    void begin() override;
    void end() override;

    /**
     * WindowEventRingBuffer
     */
    std::unique_ptr<WindowEvent> createDefaultEvent() override;

protected:
    std::ostream &toStream(std::ostream &str) const override;

private:
    std::unique_ptr<Impl> pimpl;
};

#endif // USE_WINDOW_WAYLAND
