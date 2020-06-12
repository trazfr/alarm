#include "window_raspberrypi_tft.hpp"

#ifdef PLATFORM_RASPBERRYPI

#include "event.hpp"
#include "toolbox_io.hpp"

// variadic macros issue... we don't need vcos_logging.h
#define VCOS_LOGGING_H
#include <bcm_host.h>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <ostream>

namespace
{
#define WINDOW_FRAMEBUFFER "/dev/fb1"
constexpr size_t kBitsPerPixel = 16;
} // namespace

struct WindowRaspberryPiTft::Impl
{
    ~Impl()
    {
        if (snapshotResource != DISPMANX_NO_HANDLE)
        {
            vc_dispmanx_resource_delete(snapshotResource);
        }
    }

    MmapFile mmapped;
    DISPMANX_RESOURCE_HANDLE_T snapshotResource = DISPMANX_NO_HANDLE;
    VC_RECT_T rect1 = {};
    uint32_t pitch = 0;
};

WindowRaspberryPiTft::WindowRaspberryPiTft(int width, int height)
    : WindowRaspberryPi{width, height, false},
      pimpl{std::make_unique<Impl>()}
{
    // https://github.com/tasanakorn/rpi-fbcp/blob/master/main.c

    const FileUnix fd{open(WINDOW_FRAMEBUFFER, O_RDONLY)};

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

    if (vinfo.bits_per_pixel != kBitsPerPixel)
    {
        throw std::runtime_error{WINDOW_FRAMEBUFFER " is " + std::to_string(vinfo.bits_per_pixel) + "bpp"};
    }

    pimpl->mmapped.content = mmap(nullptr, finfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd.fd, 0);
    if (pimpl->mmapped.content == MAP_FAILED)
    {
        throw std::runtime_error{"mmap(\"" WINDOW_FRAMEBUFFER "\") failed"};
    }
    pimpl->mmapped.size = finfo.smem_len;

    uint32_t dummy;
    pimpl->snapshotResource = vc_dispmanx_resource_create(VC_IMAGE_RGB565, vinfo.xres, vinfo.yres, &dummy);
    if (pimpl->snapshotResource == DISPMANX_NO_HANDLE)
    {
        throw std::runtime_error{"vc_dispmanx_resource_create() failed"};
    }

    vc_dispmanx_rect_set(&pimpl->rect1, 0, 0, vinfo.xres, vinfo.yres);
    pimpl->pitch = vinfo.xres * (kBitsPerPixel / 8);
}

WindowRaspberryPiTft::~WindowRaspberryPiTft() = default;

void WindowRaspberryPiTft::end()
{
    WindowRaspberryPi::end();

    vc_dispmanx_snapshot(getDispmanxDisplay(), pimpl->snapshotResource, DISPMANX_NO_ROTATE);
    vc_dispmanx_resource_read_data(pimpl->snapshotResource, &pimpl->rect1, pimpl->mmapped.content, pimpl->pitch);
}

std::optional<Event> WindowRaspberryPiTft::popEvent()
{
    std::optional<Event> result;
    return result;
}

std::ostream &WindowRaspberryPiTft::toStream(std::ostream &str) const
{
    str << "To TFT " WINDOW_FRAMEBUFFER ": " << pimpl->rect1.width << 'x' << pimpl->rect1.height << ' ' << kBitsPerPixel << "bpp\n";
    WindowRaspberryPi::toStream(str);

    return str;
}

#endif // PLATFORM_RASPBERRYPI
