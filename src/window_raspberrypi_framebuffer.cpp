#include "window_raspberrypi_framebuffer.hpp"

// Mesa drivers don't like eglCreatePbufferSurface() + glReadPixels()
#ifndef NO_WINDOW_RASPBERRYPI

#include "windowevent_linux.hpp"

#include "error.hpp"
#include "toolbox_gl.hpp"
#include "toolbox_io.hpp"
#include "toolbox_time.hpp"

// on rpi, it silently includes bcm headers
#define VCOS_LOGGING_H
#include <EGL/egl.h>

#include <endian.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <array>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{

using FramebufferFilename_t = std::array<char, sizeof("/dev/fb99")>;
constexpr size_t kFramebufferBitsPerPixel = 16;
constexpr size_t kPixelSize = 3;

// not very accurate... but should be OK for Raspberry (either /dev/fb1 if HDMI is plugged in or /dev/fb0)
FramebufferFilename_t getLastFb()
{
    FramebufferFilename_t result;
    for (int fbNumber = 0;; ++fbNumber)
    {
        FramebufferFilename_t buffer;
        const int bytesWritten = std::snprintf(buffer.data(), buffer.size(), "/dev/fb%d", fbNumber);
        if (bytesWritten >= static_cast<int>(buffer.size()))
        {
            throw std::runtime_error{"WindowRaspberryPiFramebuffer: too many framebuffers"};
        }

        FileUnix fd{open(buffer.data(), O_WRONLY)};
        if (fd.fd < 0)
        {
            break;
        }
        result = buffer;
    }
    return result;
}

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

} // namespace

struct WindowRaspberryPiFramebuffer::Impl
{
    ~Impl()
    {
        if (eglDisplay)
        {
            eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_DISPLAY);
        }
        if (eglContext)
        {
            eglDestroyContext(eglDisplay, eglContext);
        }
        if (eglSurface)
        {
            eglDestroySurface(eglDisplay, eglSurface);
        }
        if (eglDisplay)
        {
            eglTerminate(eglDisplay);
        }
    }

    uint32_t height = 0;
    uint32_t width = 0;
    FramebufferFilename_t framebuffer;
    MmapFile mmaped;
    std::vector<char> frame;

    EGLDisplay eglDisplay = nullptr;
    EGLSurface eglSurface = nullptr;
    EGLContext eglContext = nullptr;
};

WindowRaspberryPiFramebuffer::WindowRaspberryPiFramebuffer(int width, int height)
    : pimpl{std::make_unique<Impl>()}
{
    pimpl->width = width;
    pimpl->height = height;
    pimpl->framebuffer = getLastFb();
    const FileUnix fd{open(pimpl->framebuffer.data(), O_RDWR)};
    if (fd.fd < 0)
    {
        throw std::runtime_error{"Could not open " + std::string{pimpl->framebuffer.data(), pimpl->framebuffer.size()}};
    }

    struct fb_fix_screeninfo finfo;
    if (ioctl(fd.fd, FBIOGET_FSCREENINFO, &finfo))
    {
        throw std::runtime_error{"ioctl(FBIOGET_FSCREENINFO) failed"};
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fd.fd, FBIOGET_VSCREENINFO, &vinfo))
    {
        throw std::runtime_error{"ioctl(FBIOGET_VSCREENINFO) failed"};
    }

    pimpl->mmaped = MmapFile{mmap(nullptr, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd.fd, 0),
                             finfo.smem_len};

    if (vinfo.bits_per_pixel != kFramebufferBitsPerPixel)
    {
        throw std::runtime_error{std::string{pimpl->framebuffer.data(), pimpl->framebuffer.size()} + " is " + std::to_string(vinfo.bits_per_pixel) + "bpp"};
    }
    if (vinfo.xres != pimpl->width || vinfo.yres != pimpl->height)
    {
        throw std::runtime_error{std::string{pimpl->framebuffer.data(), pimpl->framebuffer.size()} + " is " + std::to_string(vinfo.xres) + "x" + std::to_string(vinfo.yres)};
    }

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
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE};
    EGLConfig eglConfig = nullptr;
    EGLint numConfig = 0;
    if (eglChooseConfig(pimpl->eglDisplay, eglConfigAttributes, &eglConfig, 1, &numConfig) == EGL_FALSE)
    {
        throw EGLError{"eglChooseConfig() failed"};
    }

    const EGLint pbufferAttribs[] = {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_NONE};
    pimpl->eglSurface = eglCreatePbufferSurface(pimpl->eglDisplay, eglConfig, pbufferAttribs);
    if (pimpl->eglSurface == EGL_NO_SURFACE)
    {
        throw EGLError{"eglCreatePbufferSurface() failed"};
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
    if (eglMakeCurrent(pimpl->eglDisplay, pimpl->eglSurface, pimpl->eglSurface, pimpl->eglContext) == EGL_FALSE)
    {
        throw EGLError{"eglMakeCurrent() failed"};
    }
    pimpl->frame.resize(pimpl->width * pimpl->height * kPixelSize + 1);
}

WindowRaspberryPiFramebuffer::~WindowRaspberryPiFramebuffer() = default;

void WindowRaspberryPiFramebuffer::begin()
{
}

void WindowRaspberryPiFramebuffer::end()
{
    // ~1.3ms in RGB
    glFinish();
    // ~2.5ms in RGB
    glReadPixels(0, 0, pimpl->width, pimpl->height, GL_RGB, GL_UNSIGNED_BYTE, pimpl->frame.data());

    const auto inputLineLen = pimpl->width * kPixelSize;
    const auto begin = reinterpret_cast<const uint8_t *>(pimpl->frame.data());
    auto inputLineEnd = reinterpret_cast<uint8_t *>(pimpl->frame.data() + inputLineLen * pimpl->height);
    auto output = reinterpret_cast<uint16_t *>(pimpl->mmaped.content);

    // convert RBG888 -> RGB565 + flip vertical
    // ~5.5ms in RGB
    while (inputLineEnd > begin)
    {
        const auto dataBegin = reinterpret_cast<uint8_t *>(inputLineEnd - inputLineLen);
        auto inputPixel = dataBegin;

        while (inputPixel < inputLineEnd)
        {
            const uint32_t r = inputPixel[0];
            const uint32_t g = inputPixel[1];
            const uint32_t b = inputPixel[2];

            *output = htobe16(((r << 8) & 0xf800) | ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f));

            inputPixel += kPixelSize;
            ++output;
        }
        inputLineEnd = dataBegin;
    }
}

std::unique_ptr<WindowEvent> WindowRaspberryPiFramebuffer::createDefaultEvent()
{
    return std::make_unique<WindowEventLinux>();
}

std::ostream &WindowRaspberryPiFramebuffer::toStream(std::ostream &str) const
{
    str << "\nWindow Framebuffer " << pimpl->framebuffer.data() << ": " << pimpl->width << 'x' << pimpl->height << ' ' << kFramebufferBitsPerPixel << "bpp"
        << "\nEGL info:\n - EGL_CLIENT_APIS: " << eglQueryString(pimpl->eglDisplay, EGL_CLIENT_APIS)
        << "\n - EGL_VENDOR: " << eglQueryString(pimpl->eglDisplay, EGL_VENDOR)
        << "\n - EGL_VERSION: " << eglQueryString(pimpl->eglDisplay, EGL_VERSION)
        << "\n - EGL_EXTENSIONS: " << eglQueryString(pimpl->eglDisplay, EGL_EXTENSIONS);

    return str;
}

#endif // NO_WINDOW_RASPBERRYPI
