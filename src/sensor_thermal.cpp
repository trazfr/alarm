#include "sensor_thermal.hpp"

#include "toolbox_filesystem.hpp"
#include "toolbox_io.hpp"

#include <array>
#include <cmath>
#include <cstring>
#include <random>

namespace
{

using Buffer_t = std::array<char, 65>;

} // namespace

struct SensorThermal::Impl
{
    std::mt19937 rand{};
    Clock::time_point nextRefresh = Clock::time_point::min();
    float value = 0;
    Buffer_t type{};
    Buffer_t temperaturePath{};
};

SensorThermal::SensorThermal(std::string_view path)
    : pimpl{std::make_unique<Impl>()}
{
    const fs::path fsPath{path};

    const fs::path fileType = fsPath / "type";
    const fs::path fileTemp = fsPath / "temp";

    if (fs::is_regular_file(fileTemp) && fs::is_regular_file(fileType))
    {
        copyBuffer(pimpl->temperaturePath, fileTemp.native());

        Buffer_t buf;
        const auto tempSize = readFile(fileTemp.c_str(), buf.data(), buf.size());
        if (tempSize > 0)
        {
            readFile(fileType.c_str(), pimpl->type.data(), pimpl->type.size());
            if (char *lf = std::strchr(pimpl->type.data(), '\n'))
            {
                *lf = '\0';
            }
        }
    }
}

SensorThermal::~SensorThermal() = default;

std::vector<std::unique_ptr<Sensor>> SensorThermal::create()
{
    std::vector<std::unique_ptr<Sensor>> sensors;
    const fs::path thermal = "/sys/class/thermal";
    if (fs::is_directory(thermal))
    {
        for (auto &dirEntry : fs::directory_iterator{thermal})
        {
            if (fs::is_directory(dirEntry) && std::strstr(dirEntry.path().c_str(), "thermal_zone") != nullptr)
            {
                auto sensor = std::make_unique<SensorThermal>(dirEntry.path().native());
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

bool SensorThermal::refresh(const Clock::time_point &time)
{
    if (time >= pimpl->nextRefresh)
    {
        Buffer_t value;
        if (readFile(pimpl->temperaturePath.data(), value.data(), value.size()))
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
