#include "sensor_thermal.hpp"

#include "toolbox_filesystem.hpp"
#include "toolbox_io.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <random>

namespace
{

using Buffer_t = std::array<char, 65>;

size_t readFile(const char *filename, Buffer_t &buffer)
{
    const FILEUnique file{std::fopen(filename, "rb")};
    if (file)
    {
        // omit the last byte when reading, so we can safely use buffer[size]
        if (size_t size = std::fread(buffer.data(), 1, buffer.size() - 1, file.get()))
        {
            buffer[size] = '\0';
            if (const char *end = std::strchr(buffer.data(), '\n'))
            {
                size = end - buffer.data();
                buffer[size] = '\0';
            }
            return size;
        }
    }
    return 0;
}

} // namespace

struct SensorThermal::Impl
{
    std::mt19937 rand{};
    Clock::time_point nextRefresh = Clock::time_point::min();
    float value = 0;
    Buffer_t type;
    Buffer_t temperaturePath;
};

SensorThermal::SensorThermal(std::string_view path)
    : pimpl{std::make_unique<Impl>()}
{
    const fs::path fsPath{path};
    const fs::path fileType = fsPath / "type";
    const size_t typeSize = readFile(fileType.c_str(), pimpl->type);
    pimpl->temperaturePath[0] = '\0';

    if (typeSize > 0)
    {
        const fs::path fileTemp = fsPath / "temp";
        std::strncpy(pimpl->temperaturePath.data(), fileTemp.c_str(), pimpl->temperaturePath.size());
        pimpl->temperaturePath.back() = '\0';
    }
}

SensorThermal::~SensorThermal() = default;

std::vector<std::unique_ptr<Sensor>> SensorThermal::create()
{
    std::vector<std::unique_ptr<Sensor>> sensors;
    const fs::path thermal = "/sys/class/thermal";
    for (auto &dirEntry : fs::directory_iterator{thermal})
    {
        if (fs::is_directory(dirEntry) && std::strstr(dirEntry.path().c_str(), "thermal_zone"))
        {
            auto sensor = std::make_unique<SensorThermal>(dirEntry.path().native());
            if (const auto name = sensor->getName();
                name && name[0] && sensor->refresh(Clock::time_point::min()))
            {
                sensors.push_back(std::move(sensor));
            }
        }
    }
    std::sort(sensors.begin(), sensors.end(),
              [](const auto &first, const auto &second) { return first->getName() < second->getName(); });

    return sensors;
}

bool SensorThermal::refresh(const Clock::time_point &time)
{
    if (time >= pimpl->nextRefresh)
    {
        Buffer_t value;
        if (readFile(pimpl->temperaturePath.data(), value) > 0)
        {
            char *end = nullptr;
            const long intValue = std::strtol(value.data(), &end, 10);
            if (end > value.data())
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

float SensorThermal::get() const
{
    return pimpl->value;
}

const char *SensorThermal::getName() const
{
    return pimpl->type.data();
}
