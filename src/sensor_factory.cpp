#include "sensor_factory.hpp"

#include "sensor.hpp"
#include "sensor_iio.hpp"
#include "sensor_thermal.hpp"

#include <algorithm>
#include <cstring>
#include <numeric>
#include <ostream>
#include <vector>

namespace
{
constexpr size_t sumSize()
{
    return 0;
}

template <typename... Params>
size_t sumSize(const std::vector<std::unique_ptr<Sensor>> &sensors, const Params &... others)
{
    return sensors.size() + sumSize(others...);
}

void pushBack(std::vector<std::unique_ptr<Sensor>> &)
{
}

template <typename... Params>
void pushBack(std::vector<std::unique_ptr<Sensor>> &result, std::vector<std::unique_ptr<Sensor>> &&sensors, Params &&... others)
{
    for (auto &sensor : sensors)
    {
        result.push_back(std::move(sensor));
    }
    pushBack(result, std::forward<Params>(others)...);
}

/**
 * Concatenate all the next std::vector<> into the 1st one
 */
template <typename... Params>
std::vector<std::unique_ptr<Sensor>> concat(std::vector<std::unique_ptr<Sensor>> result, Params &&... params)
{
    result.reserve(result.size() + sumSize(params...));
    pushBack(result, std::forward<Params>(params)...);
    std::sort(result.begin(), result.end(),
              [](const auto &first, const auto &second) { return std::strcmp(first->getName(), second->getName()) < 0; });
    return result;
}

void print(std::ostream &str, const std::vector<std::unique_ptr<Sensor>> &sensors, const char *name)
{
    str << " - " << name << " sensors (" << sensors.size() << "):\n";
    for (const auto &sensor : sensors)
    {
        str << "   - " << sensor->getName() << '\n';
    }
}

} // namespace

struct SensorFactory::Impl
{
    std::vector<std::unique_ptr<Sensor>> temperature;
    std::vector<std::unique_ptr<Sensor>> humidity;

    std::vector<std::unique_ptr<Sensor>> *get(SensorFactory::Type type)
    {
        switch (type)
        {
        case SensorFactory::Type::Temperature:
            return &temperature;
        case SensorFactory::Type::Humidity:
            return &humidity;
        default:
            break;
        }
        return nullptr;
    }
};

SensorFactory::SensorFactory()
    : pimpl{std::make_unique<Impl>()}
{
    pimpl->temperature = concat(SensorThermal::create(), SensorIio::create("temp"));
    pimpl->humidity = concat(SensorIio::create("humidityrelative"));
}

SensorFactory::~SensorFactory() = default;

size_t SensorFactory::getSize(Type type) const
{
    if (const auto sensors = pimpl->get(type))
    {
        return sensors->size();
    }
    return 0;
}

Sensor *SensorFactory::get(Type type, size_t sensorNumber)
{
    if (const auto sensors = pimpl->get(type))
    {
        if (sensorNumber < sensors->size())
        {
            return (*sensors)[sensorNumber].get();
        }
    }
    return nullptr;
}

const Sensor *SensorFactory::get(Type type, size_t sensorNumber) const
{
    return const_cast<SensorFactory *>(this)->get(type, sensorNumber);
}

std::ostream &SensorFactory::toStream(std::ostream &str) const
{
    str << "Sensors found:\n";
    print(str, pimpl->temperature, "temperature");
    print(str, pimpl->humidity, "humidity");
    return str;
}
