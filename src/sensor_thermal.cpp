#include "sensor_thermal.hpp"

#include "toolbox_filesystem.hpp"
#include "toolbox_io.hpp"

#include <cmath>
#include <cstring>
#include <random>

namespace
{

constexpr size_t kBufferSize = 65;
using Buffer_t = char[kBufferSize];

size_t readFile(const char *filename, Buffer_t &buffer)
{
    const FILEUnique file{std::fopen(filename, "r")};
    if (file)
    {
        // omit the last byte when reading, so we can safely use buffer[size]
        if (size_t size = std::fread(buffer, sizeof(*buffer), sizeof(buffer) / sizeof(*buffer) - 1, file.get()))
        {
            buffer[size] = '\0';
            if (const char *end = std::strchr(buffer, '\n'))
            {
                size = end - buffer;
                buffer[size] = '\0';
            }
            return size;
        }
    }
    return 0;
}

} // namespace

struct SensorTermal::Impl
{
    std::mt19937 rand{};
    Clock::time_point nextRefresh = Clock::time_point::min();
    float value = 0;
    Buffer_t type;
    Buffer_t temperaturePath;
};

SensorTermal::SensorTermal(std::string_view path)
    : pimpl{std::make_unique<Impl>()}
{
    const fs::path fsPath{path};
    const fs::path fileType = fsPath / "type";
    const size_t typeSize = readFile(fileType.c_str(), pimpl->type);
    pimpl->temperaturePath[0] = '\0';

    if (typeSize > 0)
    {
        const fs::path fileTemp = fsPath / "temp";
        std::strncpy(pimpl->temperaturePath, fileTemp.c_str(), sizeof(pimpl->temperaturePath));
        pimpl->temperaturePath[sizeof(pimpl->temperaturePath) - 1] = '\0';
    }
}

SensorTermal::~SensorTermal() = default;

bool SensorTermal::refresh(const Clock::time_point &time)
{
    if (time >= pimpl->nextRefresh)
    {
        Buffer_t value;
        if (readFile(pimpl->temperaturePath, value) > 0)
        {
            char *end = nullptr;
            const long intValue = std::strtol(value, &end, 10);
            if (end > value)
            {
                pimpl->value = intValue / 1000.;
                pimpl->nextRefresh = time + std::chrono::milliseconds{(pimpl->rand() % 120000) + 60000};
                return true;
            }
        }
        return false;
    }
    return pimpl->nextRefresh > Clock::time_point::min();
}

float SensorTermal::get() const
{
    return pimpl->value;
}

const char *SensorTermal::getName() const
{
    return pimpl->type;
}
