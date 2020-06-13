#include "window_sdl.hpp"

#ifdef PLATFORM_SDL2

#include "error.hpp"
#include "event.hpp"
#include "renderer.hpp"

#include <SDL.h>

#include <iostream>

namespace
{

constexpr uint32_t kSdlFlags = SDL_INIT_VIDEO;
constexpr char kWindowName[] = "Alarm";
constexpr char kUnavailable[] = "unavailable";

class SDLError : public Error
{
public:
    explicit SDLError(const char *description,
                      const char *func = __builtin_FUNCTION(),
                      const char *file = __builtin_FILE(),
                      int line = __builtin_LINE())
        : Error{description + std::string{". SDL Error: "} + SDL_GetError(), func, file, line}
    {
    }
};

struct LogSDLString
{
    explicit LogSDLString(const char *data) : data{data} {}
    friend std::ostream &operator<<(std::ostream &str, const LogSDLString &obj)
    {
        if (obj.data)
        {
            return str << obj.data;
        }
        return str << kUnavailable;
    }

    const char *const data;
};

template <typename T>
struct LogSDLObj
{
    LogSDLObj(int sdlResult, const T &obj)
        : sdlResult{sdlResult}, obj{obj}
    {
    }
    friend std::ostream &operator<<(std::ostream &str, const LogSDLObj &obj)
    {
        if (obj.sdlResult == 0)
        {
            return str << obj.obj;
        }
        return str << kUnavailable;
    }
    const int sdlResult;
    const T &obj;
};

LogSDLString logSDL(const char *data)
{
    return LogSDLString{data};
}

template <typename T>
LogSDLObj<T> logSDL(int sdlResult, const T &obj)
{
    return {sdlResult, obj};
}

std::ostream &operator<<(std::ostream &str, const SDL_DisplayMode &mode)
{
    return str << mode.w << 'x' << mode.h << '@' << mode.refresh_rate << "Hz " << SDL_GetPixelFormatName(mode.format);
}

std::ostream &operator<<(std::ostream &str, const SDL_version &sdl)
{
    return str << static_cast<int>(sdl.major)
               << '.' << static_cast<int>(sdl.minor)
               << '.' << static_cast<int>(sdl.patch);
}

std::ostream &operator<<(std::ostream &str, const SDL_Window &sdl)
{
    const auto windowPtr = const_cast<SDL_Window *>(&sdl);
    int x = -1;
    int y = -1;
    SDL_GetWindowPosition(windowPtr, &x, &y);
    str << "position=" << x << 'x' << y;

    if (const int i = SDL_GetWindowDisplayIndex(windowPtr); i != -1)
    {
        SDL_DisplayMode mode = {};
        str << " display_index=" << i
            << "\n - current_mode=" << logSDL(SDL_GetCurrentDisplayMode(i, &mode), mode)
            << "\n - desktop_mode=" << logSDL(SDL_GetDesktopDisplayMode(i, &mode), mode);
    }
    else
    {
        str << " could not get the window";
    }

    return str;
}

std::optional<Event> handleEvent(SDL_Window &window, const SDL_Event &sdlEvent)
{
    std::optional<Event> event;
    switch (sdlEvent.type)
    {
    case SDL_QUIT:
        event = Event::createQuit();
        break;

    case SDL_FINGERUP:
        event = Event::createClick(sdlEvent.tfinger.x, sdlEvent.tfinger.y);
        break;

    case SDL_MOUSEBUTTONUP:
        if (sdlEvent.button.button == SDL_BUTTON_LEFT && sdlEvent.button.which != SDL_TOUCH_MOUSEID)
        {
            int w = -1;
            int h = -1;
            SDL_GetWindowSize(&window, &w, &h);
            event = Event::createClick(sdlEvent.button.x / static_cast<float>(w),
                                       sdlEvent.button.y / static_cast<float>(h));
        }
        break;

    default:
        break;
    }
    return event;
}

} // namespace

struct WindowSDL::Impl
{
    ~Impl()
    {
        if (sdlGlContext)
        {
            SDL_GL_DeleteContext(sdlGlContext);
        }
        if (sdlWindow)
        {
            SDL_DestroyWindow(sdlWindow);
        }
        SDL_Quit();
    }
    SDL_Window *sdlWindow = nullptr;
    SDL_GLContext sdlGlContext = nullptr;
};

WindowSDL::WindowSDL(int width, int height)
    : pimpl{std::make_unique<Impl>()}
{
    if (SDL_Init(kSdlFlags) < 0)
    {
        throw SDLError{"Could not initialize SDL"};
    }

    // Linear filtering
    if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1") == SDL_FALSE)
    {
        std::cerr << "Warning: Linear texture filtering not enabled!" << std::endl;
    }

    pimpl->sdlWindow = SDL_CreateWindow(kWindowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (pimpl->sdlWindow == nullptr)
    {
        throw SDLError{"Could not create the window"};
    }

    pimpl->sdlGlContext = SDL_GL_CreateContext(pimpl->sdlWindow);
    if (pimpl->sdlGlContext == nullptr)
    {
        throw SDLError{"Renderer could not be created"};
    }
}

WindowSDL::~WindowSDL() = default;

void WindowSDL::begin()
{
}

void WindowSDL::end()
{
    SDL_GL_SwapWindow(pimpl->sdlWindow);
}

std::optional<Event> WindowSDL::popEvent()
{
    std::optional<Event> result;
    SDL_Event event;
    while (!result && SDL_PollEvent(&event))
    {
        result = handleEvent(*pimpl->sdlWindow, event);
    }
    return result;
}

std::ostream &WindowSDL::toStream(std::ostream &str) const
{
    str << "WindowSDL" << '\n';
    SDL_version version;
    SDL_VERSION(&version);
    str << "Compiled against SDL: " << version << '\n';
    SDL_GetVersion(&version);
    str << "Running on SDL: " << version << '\n';
    str << "SDL revision: " << SDL_GetRevisionNumber() << '\n';

    str << "Number of touch devices: " << SDL_GetNumTouchDevices() << '\n';
    str << "Available Video Drivers:\n";
    for (int i = 0, imax = SDL_GetNumVideoDrivers(); i < imax; ++i)
    {
        str << " - " << i << ": " << logSDL(SDL_GetVideoDriver(i)) << '\n';
    }

    str << "Available Video Displays:\n";
    for (int i = 0, imax = SDL_GetNumVideoDisplays(); i < imax; ++i)
    {
        str << " - " << i << ": " << logSDL(SDL_GetDisplayName(i)) << '\n';
        for (int j = 0, jmax = SDL_GetNumDisplayModes(i); j < jmax; ++j)
        {
            SDL_DisplayMode mode;
            str << "   - " << logSDL(SDL_GetDisplayMode(i, j, &mode), mode) << '\n';
        }
    }
    str << "Using Video Driver: " << logSDL(SDL_GetCurrentVideoDriver()) << '\n';
    str << "Window: " << *pimpl->sdlWindow;

    return str;
}

#endif // PLATFORM_SDL2
