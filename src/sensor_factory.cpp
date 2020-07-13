#include "sensor_factory.hpp"

#include "sensor.hpp"
#include "sensor_thermal.hpp"

#include <ostream>
#include <vector>

struct SensorFactory::Impl
{
    std::vector<std::unique_ptr<Sensor>> thermal;
};

SensorFactory::SensorFactory()
    : pimpl{std::make_unique<Impl>()}
{
    pimpl->thermal = SensorThermal::create();
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
