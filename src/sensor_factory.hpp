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

    enum class Type
    {
        Temperature,
        Humidity,
    };

    SensorFactory();
    ~SensorFactory();

    /**
     * Get the number of sensors for the given type
     * 
     * The valid sensorNumber values are from 0 to getSize(Type)
     */
    size_t getSize(Type type) const;

    /**
     * Get the sensor instance which can be refreshed
     * 
     * @return a valid Sensor if sensorNumber < getSize(Type), otherwise nullptr
     */
    Sensor *get(Type type, size_t sensorNumber);

    /**
     * Temperature sensor where we can only get the cached data
     */
    const Sensor *get(Type type, size_t sensorNumber) const;

    friend std::ostream &operator<<(std::ostream &str, const SensorFactory &factory)
    {
        return factory.toStream(str);
    }

private:
    std::ostream &toStream(std::ostream &str) const;

    std::unique_ptr<Impl> pimpl;
};
