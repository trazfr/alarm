#pragma once

#include "window.hpp"

#ifndef NO_WINDOW_SDL

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
     * WindowEventRingBuffer
     */
    std::unique_ptr<WindowEvent> createDefaultEvent() override;

protected:
    std::ostream &toStream(std::ostream &str) const override;

private:
    std::unique_ptr<Impl> pimpl;
};

#endif // NO_WINDOW_SDL
