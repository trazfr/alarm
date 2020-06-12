#pragma once

#include "window.hpp"

#ifdef PLATFORM_WAYLAND

#include <memory>

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
     * Maps the mounse
     */
    std::optional<Event> popEvent() override;

protected:
    std::ostream &toStream(std::ostream &str) const override;

private:
    std::unique_ptr<Impl> pimpl;
};

#endif // PLATFORM_WAYLAND
