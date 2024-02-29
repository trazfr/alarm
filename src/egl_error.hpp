#pragma once

#include "error.hpp"

#if USE_WINDOW_FRAMEBUFFER || USE_WINDOW_WAYLAND || USE_WINDOW_DISPMANX

#define USE_EGL_ERROR

class EGLError : public Error
{
public:
    explicit EGLError(const char *description,
                      const char *func = __builtin_FUNCTION(),
                      const char *file = __builtin_FILE(),
                      int line = __builtin_LINE());
};

#endif // USE_WINDOW_FRAMEBUFFER || USE_WINDOW_WAYLAND || USE_WINDOW_DISPMANX
