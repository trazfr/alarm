#pragma once

#include "sensor.hpp"

#include <memory>
#include <string>
#include <vector>

/**
 * @brief sensor for path /sys/bus/iio/devices in Linux (Industrial IO)
 *
 * @arg refresh rate is random between 2min and 3min
 * @arg the unit is Celcius degree (temperature) or percent (relative humidity)
 */
class SensorIio : public Sensor
{
public:
    struct Impl;

    explicit SensorIio(std::string_view path, std::string_view type);
    ~SensorIio() override;

    /**
     * Read from /sys/bus/iio/devices/iio:deviceXXX/in_${type}_(raw|scale)
     */
    static std::vector<std::unique_ptr<Sensor>> create(std::string_view type);

    bool refresh(const Clock::time_point &time) override;
    float get() const override;
    const char *getName() const override;

private:
    std::unique_ptr<Impl> pimpl;
};
