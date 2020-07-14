#include "windowevent_linux.hpp"

#include "event.hpp"
#include "toolbox_io.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>

#include <array>
#include <iostream>

struct WindowEventLinux::Impl
{
    struct EventInfo
    {
        using EventFilename_t = std::array<char, sizeof("/dev/input/event99")>;
        using EventName_t = std::array<char, 128>;

        FileUnix fd;

        EventFilename_t filename;
        EventName_t name;

        uint16_t btn = 0;
        int32_t x = 0;
        int32_t y = 0;

        int32_t xMin = 0;
        int32_t xMax = 0;
        int32_t yMin = 0;
        int32_t yMax = 0;
    };

    EventInfo info;
};

namespace
{
constexpr char kEvent[] = "/dev/input/event%d";
using EventInfo = WindowEventLinux::Impl::EventInfo;

template <typename T>
constexpr size_t getBitSize()
{
    return 8 * sizeof(T);
}

template <typename T>
constexpr size_t getNumIntegers(unsigned int max)
{
    return (max + (getBitSize<T>() - 1)) / getBitSize<T>();
}

template <typename T>
bool getBit(const T *c, unsigned int bitNum)
{
    return (c[bitNum / getBitSize<T>()] >> (bitNum % getBitSize<T>())) & 0x01;
}

bool isEvOk(int fd)
{
    long bits[getNumIntegers<long>(EV_CNT)] = {};
    if (ioctl(fd, EVIOCGBIT(0, sizeof(bits)), bits) < 0)
    {
        return false;
    }

    // we need ABS_n
    return getBit(bits, EV_ABS);
}

bool isEvAbsOk(int fd)
{
    long bitsAbs[getNumIntegers<long>(ABS_CNT)] = {};
    if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(bitsAbs)), bitsAbs) < 0)
    {
        return false;
    }

    // we need ABS_X + ABS_Y
    return getBit(bitsAbs, ABS_X) && getBit(bitsAbs, ABS_Y);
}

uint16_t getEvKey(int fd)
{
    long bitsKey[getNumIntegers<long>(KEY_CNT)] = {};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bitsKey)), bitsKey) < 0)
    {
        return 0;
    }

    // we need either BTN_LEFT or BTN_TOUCH
    for (const unsigned int btn : {BTN_LEFT, BTN_TOUCH})
    {
        if (getBit(bitsKey, btn))
        {
            return btn;
        }
    }
    return 0;
}

std::tuple<int32_t, int32_t, int32_t> getEvAbsBoundaries(int fd, int abs)
{
    struct input_absinfo absinfo;
    const auto len = ioctl(fd, EVIOCGABS(abs), &absinfo);
    if (len < 0)
    {
        throw std::runtime_error{"cannot get input_absinfo"};
    }
    return {absinfo.value, absinfo.minimum, absinfo.maximum};
}

int openFile(const char *filename)
{
    const int fd = open(filename, O_RDONLY | O_NONBLOCK, 0);
    if (fd < 0)
    {
        throw std::runtime_error{"Could not open " + std::string{filename}};
    }
    return fd;
}

EventInfo findEvent()
{
    EventInfo result;
    for (int i = 0;; ++i)
    {
        const auto size = std::snprintf(result.filename.data(), result.filename.size(), kEvent, i);
        if (size >= static_cast<int>(sizeof(result.filename)))
        {
            throw std::runtime_error{"Reached the limit"};
        }

        FileUnix fd{openFile(result.filename.data())};
        if (ioctl(fd.fd, EVIOCGNAME(result.name.size() - 1), result.name.data()) < 0)
        {
            continue;
        }
        result.name.back() = '\0';

        if ((isEvOk(fd.fd) && isEvAbsOk(fd.fd)) == false)
        {
            continue;
        }

        result.btn = getEvKey(fd.fd);
        if (result.btn == 0)
        {
            continue;
        }

        std::tie(result.x, result.xMin, result.xMax) = getEvAbsBoundaries(fd.fd, ABS_X);
        std::tie(result.y, result.yMin, result.yMax) = getEvAbsBoundaries(fd.fd, ABS_Y);
        result.fd = std::move(fd);
        break;
    }
    return result;
}

float getPosition(int32_t val, int32_t valMin, int32_t valMax)
{
    return static_cast<double>(val - valMin) / (valMax - valMin);
}

bool processEvent(EventInfo &info, struct input_event event)
{
    switch (event.type)
    {
    case EV_ABS:
        if (event.code == ABS_X)
        {
            info.x = event.value;
        }
        else if (event.code == ABS_Y)
        {
            info.y = event.value;
        }
        break;
    case EV_KEY:
        if (event.code == info.btn && event.value == 1)
        {
            return true;
        }
        break;
    default:
        break;
    }
    return false;
}

} // namespace

WindowEventLinux::WindowEventLinux()
    : pimpl{std::make_unique<Impl>()}
{
    pimpl->info = findEvent();
    if (pimpl->info.fd.fd < 0)
    {
        throw std::runtime_error{"Cannot find suitable event in /dev/input/event*"};
    }
}

WindowEventLinux::~WindowEventLinux() = default;

std::optional<Event> WindowEventLinux::popEvent()
{
    for (bool loop = true; loop;)
    {
        struct input_event event;
        const auto readBytes = read(pimpl->info.fd.fd, &event, sizeof(event));

        if (readBytes == sizeof(event))
        {
            if (processEvent(pimpl->info, event))
            {
                return Event::createClick(getPosition(pimpl->info.x, pimpl->info.xMin, pimpl->info.xMax),
                                          getPosition(pimpl->info.y, pimpl->info.yMin, pimpl->info.yMax));
            }
        }
        else if (readBytes == 0 || (readBytes < 0 && errno == EAGAIN))
        {
            loop = false;
        }
        else
        {
            std::cerr << "Error while reading from " << pimpl->info.filename.data() << std::endl;
            pimpl->info = findEvent();
        }
    }
    return {};
}

std::ostream &WindowEventLinux::toStream(std::ostream &str) const
{
    return str << "WindowEventLinux: " << pimpl->info.filename.data()
               << "\n - name: " << pimpl->info.name.data()
               << "\n - X: " << pimpl->info.x << " limits: " << pimpl->info.xMin << " to " << pimpl->info.xMax
               << "\n - Y: " << pimpl->info.y << " limits: " << pimpl->info.yMin << " to " << pimpl->info.yMax;
}
