#include "window_framebuffer.hpp"

#ifdef USE_WINDOW_FRAMEBUFFER

// Mesa drivers don't like eglCreatePbufferSurface() + glReadPixels()

// on rpi, it silently includes bcm headers
#ifdef USE_WINDOW_DISPMANX
#define VCOS_LOGGING_H
#endif // USE_WINDOW_DISPMANX

#include "windowevent_linux.hpp"

#include "error.hpp"
#include "toolbox_gl.hpp"
#include "toolbox_io.hpp"
#include "toolbox_time.hpp"

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
            throw std::runtime_error{"WindowFramebuffer: too many framebuffers"};
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

GLenum glFormat(uint32_t frameBufferPixelSize)
{
    switch (frameBufferPixelSize)
    {
    case 2:
        return GL_RGB;
    case 3:
        return GL_RGB;
    case 4:
        return GL_RGBA;
    default:
        return 0;
    }
}

size_t glPixelSize(GLenum format)
{
    switch (format)
    {
    case GL_RGB:
        return 3;
    case GL_RGBA:
        return 4;
    default:
        throw std::runtime_error{"Unsupported format: " + std::to_string(format)};
    }
}

class EGLError : public Error
{
public:
    explicit EGLError(const char *description,
                      const char *func = __builtin_FUNCTION(),
                      const char *file = __builtin_FILE(),
                      int line = __builtin_LINE())
        : EGLError{description, eglGetError(), func, file, line}
    {
    }

private:
    explicit EGLError(const char *description,
                      EGLint eglError,
                      const char *func,
                      const char *file,
                      int line)
        : Error{std::string(description) + ". EGL Error: " + std::to_string(eglError) + ": " + eglErrorDescription(eglError), func, file, line}
    {
    }

    // https://registry.khronos.org/EGL/sdk/docs/man/html/eglGetError.xhtml
    static const char *eglErrorDescription(EGLint eglError)
    {
#define EGL_CASE(e) \
    case e:         \
        return #e

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
};

} // namespace

struct WindowFramebuffer::Impl
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
    uint32_t frameBufferPixelSize = 0;
    FramebufferFilename_t framebuffer;
    MmapFile mmaped;

    std::vector<char> glFrame;

    GLenum glFormat = 0;
    GLenum glPixelSize = 0;
    EGLDisplay eglDisplay = nullptr;
    EGLSurface eglSurface = nullptr;
    EGLContext eglContext = nullptr;
};

WindowFramebuffer::WindowFramebuffer(int width, int height)
    : pimpl{std::make_unique<Impl>()}
{
    pimpl->width = width;
    pimpl->height = height;
    pimpl->framebuffer = getLastFb();
    const FileUnix fd{open(pimpl->framebuffer.data(), O_RDWR)};
    if (fd.fd < 0)
    {
        throw std::runtime_error{"Could not open " + std::string{pimpl->framebuffer.data()}};
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
    if (vinfo.bits_per_pixel != 16 && vinfo.bits_per_pixel != 24 && vinfo.bits_per_pixel != 32)
    {
        throw std::runtime_error{std::string{pimpl->framebuffer.data()} + " is " + std::to_string(vinfo.bits_per_pixel) + "bpp"};
    }
    pimpl->frameBufferPixelSize = vinfo.bits_per_pixel >> 3;

    if (vinfo.xres != pimpl->width || vinfo.yres != pimpl->height)
    {
        std::cerr << "Overriding width*height from " << pimpl->width << '*' << pimpl->height << " to " << vinfo.xres << '*' << vinfo.yres << std::endl;
        pimpl->width = vinfo.xres;
        pimpl->height = vinfo.yres;
    }

    pimpl->mmaped = MmapFile{mmap(nullptr, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd.fd, 0),
                             finfo.smem_len};

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
        EGL_WIDTH, static_cast<EGLint>(pimpl->width),
        EGL_HEIGHT, static_cast<EGLint>(pimpl->height),
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

    pimpl->glFormat = glFormat(pimpl->frameBufferPixelSize);
    pimpl->glPixelSize = glPixelSize(pimpl->glFormat);
    pimpl->glFrame.resize(pimpl->width * pimpl->height * pimpl->glPixelSize);
}

WindowFramebuffer::~WindowFramebuffer() = default;

void WindowFramebuffer::begin()
{
}

void WindowFramebuffer::end()
{
    // ~1.3ms in RGB
    glFinish();
    // ~2.5ms in RGB
    glReadPixels(0, 0, pimpl->width, pimpl->height, pimpl->glFormat, GL_UNSIGNED_BYTE, pimpl->glFrame.data());

    if (pimpl->frameBufferPixelSize == pimpl->glPixelSize)
    {
        std::memcpy(pimpl->mmaped.content, pimpl->glFrame.data(), pimpl->glFrame.size());
    }
    else if (pimpl->frameBufferPixelSize == 2 && pimpl->glPixelSize == 3)
    {
        constexpr size_t glPixelSize = 3;
        const auto inputLineLen = pimpl->width * glPixelSize;
        const auto begin = reinterpret_cast<const uint8_t *>(pimpl->glFrame.data());
        auto inputLineEnd = reinterpret_cast<uint8_t *>(pimpl->glFrame.data() + inputLineLen * pimpl->height);
        auto output = reinterpret_cast<uint32_t *>(pimpl->mmaped.content);

        // convert RBG888 -> RGB565 + flip vertical
        // ~5.5ms in RGB
        while (inputLineEnd > begin)
        {
            const auto dataBegin = reinterpret_cast<uint8_t *>(inputLineEnd - inputLineLen);
            auto inputPixel = dataBegin;

            while (inputPixel < inputLineEnd)
            {
                const uint32_t r = inputPixel[0] << 16 | inputPixel[glPixelSize];
                const uint32_t g = inputPixel[1] << 16 | inputPixel[glPixelSize + 1];
                const uint32_t b = inputPixel[2] << 16 | inputPixel[glPixelSize + 2];
                *output = htobe32((r & 0x00f800f8) << 8 | ((g & 0x00fc00fc) << 3) | ((b & 0x00f800f8) >> 3));

                inputPixel += glPixelSize * 2;
                ++output;
            }
            inputLineEnd = dataBegin;
        }
    }
}

std::unique_ptr<WindowEvent> WindowFramebuffer::createDefaultEvent()
{
    return std::make_unique<WindowEventLinux>();
}

std::ostream &WindowFramebuffer::toStream(std::ostream &str) const
{
    str << "\nWindow Framebuffer " << pimpl->framebuffer.data() << ": " << pimpl->width << 'x' << pimpl->height << ' ' << (pimpl->frameBufferPixelSize << 3) << "bpp"
        << "\nEGL info:\n - EGL_CLIENT_APIS: " << eglQueryString(pimpl->eglDisplay, EGL_CLIENT_APIS)
        << "\n - EGL_VENDOR: " << eglQueryString(pimpl->eglDisplay, EGL_VENDOR)
        << "\n - EGL_VERSION: " << eglQueryString(pimpl->eglDisplay, EGL_VERSION)
        << "\n - EGL_EXTENSIONS: " << eglQueryString(pimpl->eglDisplay, EGL_EXTENSIONS);

    return str;
}

#endif // USE_WINDOW_FRAMEBUFFER
