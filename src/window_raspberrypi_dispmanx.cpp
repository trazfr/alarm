#include "window_raspberrypi_dispmanx.hpp"

#ifdef USE_WINDOW_DISPMANX

#include "windowevent_linux.hpp"

#include "error.hpp"

// variadic macros issue... we don't need vcos_logging.h
#define VCOS_LOGGING_H
#include <bcm_host.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <ostream>

namespace
{

static constexpr int kDisplayNumber = 0; // LCD

struct Dispmanx
{
    uint32_t displayWidth = 0;
    uint32_t displayHeight = 0;
    DISPMANX_DISPLAY_HANDLE_T dispmanDisplay = 0;

    EGL_DISPMANX_WINDOW_T eglNativeWindow = {};
};

class EGLError : public Error
{
public:
    explicit EGLError(const char *description,
                      const char *func = __builtin_FUNCTION(),
                      const char *file = __builtin_FILE(),
                      int line = __builtin_LINE())
        : Error{description + std::string{". EGL Error: "} + std::to_string(eglGetError()), func, file, line}
    {
    }
};

Dispmanx createDispanxWindow(int width, int height)
{
    Dispmanx result;
    if (graphics_get_display_size(kDisplayNumber, &result.displayWidth, &result.displayHeight) < 0)
    {
        throw std::runtime_error{"graphics_get_display_size() failed"};
    }

    result.dispmanDisplay = vc_dispmanx_display_open(kDisplayNumber);

    const DISPMANX_UPDATE_HANDLE_T dispmanUpdate = vc_dispmanx_update_start(0);
    if (dispmanUpdate == 0)
    {
        throw std::runtime_error{"vc_dispmanx_update_start() failed"};
    }

    const VC_RECT_T srcRect = {0, 0, width << 16, height << 16};
    const VC_RECT_T dstRect = {0, 0, static_cast<int32_t>(result.displayWidth), static_cast<int32_t>(result.displayHeight)};
    VC_DISPMANX_ALPHA_T alpha = {DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 255, 0};
    result.eglNativeWindow.element = vc_dispmanx_element_add(dispmanUpdate,
                                                             result.dispmanDisplay,
                                                             0 /*layer*/,
                                                             &dstRect,
                                                             0 /*src*/,
                                                             &srcRect,
                                                             DISPMANX_PROTECTION_NONE,
                                                             &alpha,
                                                             0 /*clamp*/,
                                                             DISPMANX_NO_ROTATE);
    if (result.eglNativeWindow.element == 0)
    {
        throw std::runtime_error{"vc_dispmanx_element_add() failed"};
    }
    result.eglNativeWindow.width = width;
    result.eglNativeWindow.height = height;

    if (vc_dispmanx_update_submit_sync(dispmanUpdate) != 0)
    {
        throw std::runtime_error{"vc_dispmanx_update_submit_sync() failed"};
    }

    return result;
}

} // namespace

struct WindowRaspberryPiDispmanx::Impl
{
    ~Impl()
    {
        if (eglDisplay)
        {
            eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_DISPLAY);
        }
        if (eglSurface)
        {
            eglDestroySurface(eglDisplay, eglSurface);
        }
        if (eglContext)
        {
            eglDestroyContext(eglDisplay, eglContext);
        }
        if (eglDisplay)
        {
            eglTerminate(eglDisplay);
        }
        if (dispmanx.eglNativeWindow.element)
        {
            const DISPMANX_UPDATE_HANDLE_T dispmanUpdate = vc_dispmanx_update_start(0);
            vc_dispmanx_element_remove(dispmanUpdate, dispmanx.eglNativeWindow.element);
            vc_dispmanx_update_submit_sync(dispmanUpdate);
        }
        if (dispmanx.dispmanDisplay)
        {
            vc_dispmanx_display_close(dispmanx.dispmanDisplay);
        }

        bcm_host_deinit();
    }
    EGLDisplay eglDisplay = nullptr;
    EGLContext eglContext = nullptr;
    Dispmanx dispmanx;
    EGLSurface eglSurface = nullptr;
};

WindowRaspberryPiDispmanx::WindowRaspberryPiDispmanx(int width, int height)
    : pimpl{std::make_unique<Impl>()}
{
    bcm_host_init();

    pimpl->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (pimpl->eglDisplay == EGL_NO_DISPLAY)
    {
        throw EGLError{"eglGetDisplay() failed"};
    }

    if (eglInitialize(pimpl->eglDisplay, nullptr, nullptr) == EGL_FALSE)
    {
        throw EGLError{"eglInitialize() failed"};
    }

    static constexpr EGLint eglConfigAttributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_BUFFER_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE};
    EGLConfig eglConfig = nullptr;
    EGLint numConfig = 0;
    if (eglChooseConfig(pimpl->eglDisplay, eglConfigAttributes, &eglConfig, 1, &numConfig) == EGL_FALSE)
    {
        throw EGLError{"eglChooseConfig() failed"};
    }

    if (eglBindAPI(EGL_OPENGL_ES_API) == EGL_FALSE)
    {
        throw EGLError{"eglBindAPI() failed"};
    }

    static constexpr EGLint eglContextAttributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE};
    pimpl->eglContext = eglCreateContext(pimpl->eglDisplay, eglConfig, EGL_NO_CONTEXT, eglContextAttributes);
    if (pimpl->eglContext == EGL_NO_CONTEXT)
    {
        throw EGLError{"eglCreateContext() failed"};
    }

    pimpl->dispmanx = createDispanxWindow(width, height);
    pimpl->eglSurface = eglCreateWindowSurface(pimpl->eglDisplay, eglConfig, &pimpl->dispmanx.eglNativeWindow, nullptr);
    if (pimpl->eglSurface == EGL_NO_SURFACE)
    {
        throw EGLError{"eglCreateWindowSurface() failed"};
    }

    if (eglMakeCurrent(pimpl->eglDisplay, pimpl->eglSurface, pimpl->eglSurface, pimpl->eglContext) == EGL_FALSE)
    {
        throw EGLError{"eglMakeCurrent() failed"};
    }
}

WindowRaspberryPiDispmanx::~WindowRaspberryPiDispmanx() = default;

void WindowRaspberryPiDispmanx::begin()
{
}

void WindowRaspberryPiDispmanx::end()
{
    eglSwapBuffers(pimpl->eglDisplay, pimpl->eglSurface);
}

std::unique_ptr<WindowEvent> WindowRaspberryPiDispmanx::createDefaultEvent()
{
    return std::make_unique<WindowEventLinux>();
}

std::ostream &WindowRaspberryPiDispmanx::toStream(std::ostream &str) const
{
    str << "Raspberry PI native display: " << pimpl->dispmanx.displayWidth << 'x' << pimpl->dispmanx.displayHeight
        << "\nRaspberry PI: " << pimpl->dispmanx.eglNativeWindow.width << 'x' << pimpl->dispmanx.eglNativeWindow.height
        << "\nEGL info:\n - EGL_CLIENT_APIS: " << eglQueryString(pimpl->eglDisplay, EGL_CLIENT_APIS)
        << "\n - EGL_VENDOR: " << eglQueryString(pimpl->eglDisplay, EGL_VENDOR)
        << "\n - EGL_VERSION: " << eglQueryString(pimpl->eglDisplay, EGL_VERSION)
        << "\n - EGL_EXTENSIONS: " << eglQueryString(pimpl->eglDisplay, EGL_EXTENSIONS);
    return str;
}

#endif // USE_WINDOW_DISPMANX
