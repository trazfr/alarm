#include "window_wayland.hpp"

#ifdef PLATFORM_WAYLAND

#include "config.hpp"
#include "error.hpp"
#include "event.hpp"
#include "toolbox_ringbuffer.hpp"

#include <wayland-client.h>
#include <wayland-egl.h>

#include <EGL/egl.h>

#include <linux/input-event-codes.h>

#include <cstring>
#include <ostream>

struct WindowWayland::Impl
{
    ~Impl()
    {
        eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_DISPLAY);
        if (eglSurface)
        {
            eglDestroySurface(eglDisplay, eglSurface);
        }
        if (eglWindow)
        {
            wl_egl_window_destroy(eglWindow);
        }
        if (wlCompositor)
        {
            wl_compositor_destroy(wlCompositor);
        }
        if (wlPointer)
        {
            wl_pointer_destroy(wlPointer);
        }
        if (wlSeat)
        {
            wl_seat_destroy(wlSeat);
        }
        if (wlShellSurface)
        {
            wl_shell_surface_destroy(wlShellSurface);
        }
        if (wlShell)
        {
            wl_shell_destroy(wlShell);
        }
        if (wlSurface)
        {
            wl_surface_destroy(wlSurface);
        }
        if (eglContext)
        {
            eglDestroyContext(eglDisplay, eglContext);
        }

        if (eglDisplay)
        {
            eglTerminate(eglDisplay);
        }
        if (wlDisplay)
        {
            wl_display_disconnect(wlDisplay);
        }
    }

    wl_compositor *wlCompositor = nullptr;
    wl_seat *wlSeat = nullptr;
    wl_pointer *wlPointer = nullptr;
    wl_display *wlDisplay = nullptr;
    wl_shell *wlShell = nullptr;
    wl_surface *wlSurface = nullptr;
    wl_shell_surface *wlShellSurface = nullptr;

    wl_egl_window *eglWindow = nullptr;

    EGLContext eglContext = nullptr;
    EGLDisplay eglDisplay = nullptr;
    EGLSurface eglSurface = nullptr;

    RingBuffer<Event, 64> events;

    int32_t displayWidth = 0;
    int32_t displayHeight = 0;
    int pointerX = 0;
    int pointerY = 0;
};

namespace
{

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

// wl_pointer_listener

void pointerEnter(void *,
                  struct wl_pointer *,
                  uint32_t,
                  struct wl_surface *,
                  wl_fixed_t,
                  wl_fixed_t)
{
}

void pointerLeave(void *,
                  struct wl_pointer *,
                  uint32_t,
                  struct wl_surface *)
{
}

void pointerMotion(void *data,
                   struct wl_pointer *,
                   uint32_t,
                   wl_fixed_t surface_x,
                   wl_fixed_t surface_y)
{
    const auto pimpl = reinterpret_cast<WindowWayland::Impl *>(data);
    pimpl->pointerX = wl_fixed_to_int(surface_x);
    pimpl->pointerY = wl_fixed_to_int(surface_y);
}

void pointerButton(void *data,
                   struct wl_pointer *,
                   uint32_t,
                   uint32_t,
                   uint32_t button,
                   uint32_t state)
{
    const auto pimpl = reinterpret_cast<WindowWayland::Impl *>(data);
    if (state == WL_POINTER_BUTTON_STATE_RELEASED && button == BTN_LEFT)
    {
        pimpl->events.push(Event::createClick(pimpl->pointerX / float(pimpl->displayWidth),
                                              pimpl->pointerY / float(pimpl->displayHeight)));
    }
}

void pointerAxis(void *,
                 struct wl_pointer *,
                 uint32_t,
                 uint32_t,
                 wl_fixed_t)
{
}

constexpr wl_pointer_listener getWlPointerListener()
{
    wl_pointer_listener result{};
    result.enter = pointerEnter;
    result.leave = pointerLeave;
    result.motion = pointerMotion;
    result.button = pointerButton;
    result.axis = pointerAxis;
    return result;
}

// wl_registry_listener

void registryAddObject(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t)
{
    const auto pimpl = reinterpret_cast<WindowWayland::Impl *>(data);
    if (std::strcmp(interface, wl_compositor_interface.name) == 0)
    {
        pimpl->wlCompositor = reinterpret_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, 1));
    }
    else if (std::strcmp(interface, wl_shell_interface.name) == 0)
    {
        pimpl->wlShell = reinterpret_cast<wl_shell *>(wl_registry_bind(registry, name, &wl_shell_interface, 1));
    }
    else if (std::strcmp(interface, wl_seat_interface.name) == 0)
    {
        static constexpr wl_pointer_listener kWlPointerListener = getWlPointerListener();

        pimpl->wlSeat = reinterpret_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, 1));
        pimpl->wlPointer = wl_seat_get_pointer(pimpl->wlSeat);
        wl_pointer_add_listener(pimpl->wlPointer, &kWlPointerListener, data);
    }
}

void registryRemoveObject(void *, struct wl_registry *, uint32_t)
{
}

constexpr wl_registry_listener getRegistryListener()
{
    wl_registry_listener result{};
    result.global = registryAddObject;
    result.global_remove = registryRemoveObject;
    return result;
}

// wl_shell_surface_listener

void shellSurfacePing(void *, struct wl_shell_surface *shellSurface, uint32_t serial)
{
    wl_shell_surface_pong(shellSurface, serial);
}

void shellSurfaceConfigure(void *data, struct wl_shell_surface *, uint32_t, int32_t width, int32_t height)
{
    const auto pimpl = reinterpret_cast<WindowWayland::Impl *>(data);
    pimpl->displayWidth = width;
    pimpl->displayHeight = height;
    wl_egl_window_resize(pimpl->eglWindow, width, height, 0, 0);
}

void shellSurfacePopupDone(void *, struct wl_shell_surface *)
{
}

constexpr wl_shell_surface_listener getShellSurfaceListener()
{
    wl_shell_surface_listener result{};
    result.ping = shellSurfacePing;
    result.configure = shellSurfaceConfigure;
    result.popup_done = shellSurfacePopupDone;
    return result;
}

} // namespace

WindowWayland::WindowWayland(int width, int height)
    : pimpl{std::make_unique<Impl>()}
{
    pimpl->wlDisplay = wl_display_connect(nullptr);
    if (pimpl->wlDisplay == nullptr)
    {
        throw std::runtime_error{"Cannot connect to Wayland display"};
    }
    const auto registry = wl_display_get_registry(pimpl->wlDisplay);
    if (registry == nullptr)
    {
        throw std::runtime_error{"Cannot get Wayland registry"};
    }

    static constexpr wl_registry_listener kRegistryListener = getRegistryListener();
    wl_registry_add_listener(registry, &kRegistryListener, pimpl.get());
    wl_display_roundtrip(pimpl->wlDisplay);

    pimpl->eglDisplay = eglGetDisplay(pimpl->wlDisplay);
    if (pimpl->eglDisplay == nullptr)
    {
        throw EGLError{"Cannot get EGL display"};
    }

    if (eglInitialize(pimpl->eglDisplay, nullptr, nullptr) == EGL_FALSE)
    {
        throw EGLError{"Cannot initialize EGL"};
    }

    static constexpr EGLint attributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_BUFFER_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE};
    EGLConfig eglConfig = nullptr;
    EGLint numConfig = 0;
    if (eglChooseConfig(pimpl->eglDisplay, attributes, &eglConfig, 1, &numConfig) == EGL_FALSE)
    {
        throw EGLError{"Cannot choose EGL config"};
    }

    if (eglBindAPI(EGL_OPENGL_API) == EGL_FALSE)
    {
        throw EGLError{"Cannot bind API to EGL"};
    }

    pimpl->eglContext = eglCreateContext(pimpl->eglDisplay, eglConfig, EGL_NO_CONTEXT, nullptr);
    if (pimpl->eglContext == EGL_NO_CONTEXT)
    {
        throw EGLError{"Cannot create EGL context"};
    }

    pimpl->wlSurface = wl_compositor_create_surface(pimpl->wlCompositor);
    if (pimpl->wlSurface == nullptr)
    {
        throw std::runtime_error{"Cannot create Wayland surface"};
    }

    pimpl->wlShellSurface = wl_shell_get_shell_surface(pimpl->wlShell, pimpl->wlSurface);
    if (pimpl->wlShellSurface == nullptr)
    {
        throw std::runtime_error{"Cannot create Wayland shell surface"};
    }

    static constexpr wl_shell_surface_listener kShellSurfaceListener = getShellSurfaceListener();
    wl_shell_surface_add_listener(pimpl->wlShellSurface, &kShellSurfaceListener, pimpl.get());
    wl_shell_surface_set_toplevel(pimpl->wlShellSurface);
    wl_shell_surface_set_title(pimpl->wlShellSurface, "alarm");

    pimpl->eglWindow = wl_egl_window_create(pimpl->wlSurface, width, height);
    if (pimpl->eglWindow == nullptr)
    {
        throw std::runtime_error{"Cannot create Wayland EGL window"};
    }
    pimpl->eglSurface = eglCreateWindowSurface(pimpl->eglDisplay, eglConfig, pimpl->eglWindow, nullptr);
    if (pimpl->eglSurface == EGL_NO_SURFACE)
    {
        throw EGLError{"Cannot create Wayland EGL window surface"};
    }
    if (eglMakeCurrent(pimpl->eglDisplay, pimpl->eglSurface, pimpl->eglSurface, pimpl->eglContext) == EGL_FALSE)
    {
        throw EGLError{"Cannot make EGL current"};
    }
}

WindowWayland::~WindowWayland() = default;

void WindowWayland::begin()
{
    wl_display_dispatch_pending(pimpl->wlDisplay);
}

void WindowWayland::end()
{
    eglSwapBuffers(pimpl->eglDisplay, pimpl->eglSurface);
}

std::optional<Event> WindowWayland::popEvent()
{
    return pimpl->events.pop();
}

std::ostream &WindowWayland::toStream(std::ostream &str) const
{
    return str << "Wayland window: \nEGL info:\n - EGL_CLIENT_APIS: " << eglQueryString(pimpl->eglDisplay, EGL_CLIENT_APIS)
               << "\n - EGL_VENDOR: " << eglQueryString(pimpl->eglDisplay, EGL_VENDOR)
               << "\n - EGL_VERSION: " << eglQueryString(pimpl->eglDisplay, EGL_VERSION)
               << "\n - EGL_EXTENSIONS: " << eglQueryString(pimpl->eglDisplay, EGL_EXTENSIONS);
}

#endif // PLATFORM_WAYLAND
