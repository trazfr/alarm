#include "egl_error.hpp"

#ifdef USE_EGL_ERROR

#include <EGL/egl.h>

namespace
{

static constexpr const char *eglErrorDescription(EGLint eglError)
{
#define EGL_CASE(e) \
    case e:         \
        return #e

    // https://registry.khronos.org/EGL/sdk/docs/man/html/eglGetError.xhtml
    switch (eglError)
    {
        EGL_CASE(EGL_SUCCESS);
        EGL_CASE(EGL_NOT_INITIALIZED);
        EGL_CASE(EGL_BAD_ACCESS);
        EGL_CASE(EGL_BAD_ALLOC);
        EGL_CASE(EGL_BAD_ATTRIBUTE);
        EGL_CASE(EGL_BAD_CONTEXT);
        EGL_CASE(EGL_BAD_CONFIG);
        EGL_CASE(EGL_BAD_CURRENT_SURFACE);
        EGL_CASE(EGL_BAD_DISPLAY);
        EGL_CASE(EGL_BAD_SURFACE);
        EGL_CASE(EGL_BAD_MATCH);
        EGL_CASE(EGL_BAD_PARAMETER);
        EGL_CASE(EGL_BAD_NATIVE_PIXMAP);
        EGL_CASE(EGL_BAD_NATIVE_WINDOW);
        EGL_CASE(EGL_CONTEXT_LOST);
    default:
        return "UNKNOWN";
    }
}

std::string errorDescription(EGLint eglError)
{
    return std::to_string(eglError) + ": " + eglErrorDescription(eglError);
}

} // namespace

EGLError::EGLError(const char *description,
                   const char *func,
                   const char *file,
                   int line)
    : Error{std::string(description) + ". EGL Error: " + errorDescription(eglGetError()), func, file, line}
{
}

#endif // USE_EGL_ERROR
