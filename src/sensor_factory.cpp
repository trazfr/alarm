#include "sensor_factory.hpp"

#include "sensor.hpp"
#include "sensor_thermal.hpp"
#include "toolbox_filesystem.hpp"

#include <algorithm>
#include <cstring>
#include <ostream>
#include <vector>

namespace
{

std::vector<std::unique_ptr<Sensor>> createThermalSensors()
{
    std::vector<std::unique_ptr<Sensor>> sensors;
    const fs::path thermal = "/sys/class/thermal";
    for (auto &dirEntry : fs::directory_iterator{thermal})
    {
        if (fs::is_directory(dirEntry) && std::strstr(dirEntry.path().c_str(), "thermal_zone"))
        {
            auto sensor = std::make_unique<SensorTermal>(dirEntry.path().native());
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

} // namespace

struct SensorFactory::Impl
{
    std::vector<std::unique_ptr<Sensor>> thermal;
};

SensorFactory::SensorFactory()
    : pimpl{std::make_unique<Impl>()}
{
    pimpl->thermal = createThermalSensors();
}

SensorFactory::~SensorFactory() = default;

size_t SensorFactory::getThermalSize() const
{
    return pimpl->thermal.size();
}

Sensor *SensorFactory::getThermal(size_t sensorNumber)
{
    if (sensorNumber < pimpl->thermal.size())
    {
        return pimpl->thermal[sensorNumber].get();
    }
    return nullptr;
}

const Sensor *SensorFactory::getThermal(size_t sensorNumber) const
{
    return const_cast<SensorFactory *>(this)->getThermal(sensorNumber);
}

std::ostream &SensorFactory::toStream(std::ostream &str) const
{
    str << getThermalSize() << " thermal sensors:\n";
    for (size_t i = 0; i < getThermalSize(); ++i)
    {
        str << " - " << getThermal(i)->getName() << '\n';
    }
    return str;
}
