#include "sensor_iio.hpp"

#include "toolbox_filesystem.hpp"
#include "toolbox_io.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <random>

namespace
{

using Buffer_t = std::array<char, 65>;

long readLong(const char *filename)
{
    Buffer_t value;
    if (readFile(filename, value.data(), value.size()))
    {
        char *end = nullptr;
        const long longValue = std::strtol(value.data(), &end, 10);
        if (end > value.data())
        {
            return longValue;
        }
    }
    return std::numeric_limits<long>::min();
}

} // namespace

struct SensorIio::Impl
{
    std::mt19937 rand{};
    Clock::time_point nextRefresh = Clock::time_point::min();
    float value = 0;
    float scale = 1000.;
    Buffer_t name{};
    Buffer_t sensorPath{};
};

SensorIio::SensorIio(std::string_view path, std::string_view type)
    : pimpl{std::make_unique<Impl>()}
{
    const fs::path fsPath{path};

    std::string filenameBase = "in_";
    filenameBase += type;

    const fs::path filenameScale = fsPath / (filenameBase + "_scale");
    if (const long scale = readLong(filenameScale.c_str()); scale > std::numeric_limits<long>::min())
    {
        pimpl->scale = 1000. / scale;
    }

    const fs::path filenameValue = fsPath / (filenameBase + "_raw");
    copyBuffer(pimpl->sensorPath, filenameValue.native());

    if (const long value = readLong(pimpl->sensorPath.data()); value > std::numeric_limits<long>::min())
    {
        pimpl->value = value / pimpl->scale;
        const fs::path pathSensorName = fsPath / "name";
        readFile(pathSensorName.c_str(), pimpl->name.data(), pimpl->name.size());
        if (char *lf = std::strchr(pimpl->name.data(), '\n'))
        {
            *lf = '\0';
        }
    }
}

SensorIio::~SensorIio() = default;

std::vector<std::unique_ptr<Sensor>> SensorIio::create(std::string_view type)
{
    std::vector<std::unique_ptr<Sensor>> sensors;
    const fs::path devices = "/sys/bus/iio/devices";
    if (fs::is_directory(devices))
    {
        for (auto &dirEntry : fs::directory_iterator{devices})
        {
            if (fs::is_directory(dirEntry) && std::strstr(dirEntry.path().c_str(), "device") != nullptr)
            {
                auto sensor = std::make_unique<SensorIio>(dirEntry.path().c_str(), type);
                if (const auto name = sensor->getName();
                    name && name[0] && sensor->refresh(Clock::time_point::min()))
                {
                    sensors.push_back(std::move(sensor));
                }
            }
        }
    }

    return sensors;
}

bool SensorIio::refresh(const Clock::time_point &time)
{
    if (time >= pimpl->nextRefresh)
    {
        if (const long value = readLong(pimpl->sensorPath.data()); value > std::numeric_limits<long>::min())
        {
            pimpl->value = value / pimpl->scale;
            pimpl->nextRefresh = time + std::chrono::milliseconds{(pimpl->rand() % 120000) + 60000};
            return true;
        }
        return false;
    }
    return pimpl->nextRefresh > Clock::time_point::min();
}

float SensorIio::get() const
{
    return pimpl->value;
}

const char *SensorIio::getName() const
{
    return pimpl->name.data();
}
