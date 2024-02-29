#pragma once

#include "window.hpp"

#ifdef USE_WINDOW_FRAMEBUFFER

#include <memory>

/**
 * @brief Window to output to the Linux Framebuffer
 */
class WindowFramebuffer : public Window
{
public:
    struct Impl;

    explicit WindowFramebuffer(int width, int height);
    ~WindowFramebuffer() override;

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

#endif // USE_WINDOW_FRAMEBUFFER
