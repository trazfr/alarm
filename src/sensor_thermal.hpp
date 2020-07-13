#pragma once

#include "sensor.hpp"

#include <memory>
#include <string>
#include <vector>

/**
 * @brief sensor for path /sys/class/thermal in Linux
 * 
 * @arg refresh rate is random between 2min and 3min
 * @arg the unit is Celcius degree
 */
class SensorThermal : public Sensor
{
public:
    struct Impl;

    explicit SensorThermal(std::string_view path);
    ~SensorThermal() override;

    static std::vector<std::unique_ptr<Sensor>> create();

    bool refresh(const Clock::time_point &time) override;
    float get() const override;
    const char *getName() const override;

private:
    std::unique_ptr<Impl> pimpl;
};
