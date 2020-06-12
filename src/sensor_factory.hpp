#pragma once

#include <iosfwd>
#include <memory>

class Sensor;

/**
 * @brief instantiate and stores the sensors
 */
class SensorFactory
{
public:
    struct Impl;

    SensorFactory();
    ~SensorFactory();

    /**
     * Number of /sys/class/thermal sensors
     * 
     * The valid sensorNumber values are from 0 to getThermalSize()
     */
    size_t getThermalSize() const;

    /**
     * /sys/class/thermal sensor instance which can be refreshed
     * 
     * @return a valid Sensor if sensorNumber < getThermalSize(), otherwise nullptr
     */
    Sensor *getThermal(size_t sensorNumber);

    /**
     * /sys/class/thermal sensor where we can only get the cached data
     */
    const Sensor *getThermal(size_t sensorNumber) const;

    friend std::ostream &operator<<(std::ostream &str, const SensorFactory &factory)
    {
        return factory.toStream(str);
    }

private:
    std::ostream &toStream(std::ostream &str) const;

    std::unique_ptr<Impl> pimpl;
};
