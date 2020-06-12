#pragma once

#include "window.hpp"

#ifdef PLATFORM_SDL2

#include <memory>

/**
 * @brief Window using SDL2 with an OpenGL context
 */
class WindowSDL : public Window
{
public:
    struct Impl;

    explicit WindowSDL(int width, int height);
    ~WindowSDL() override;

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

#endif // PLATFORM_SDL2
