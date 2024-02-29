#pragma once

#include "toolbox_time.hpp"

/**
 * @brief base class for sensors (temperature / humidity...)
 */
class Sensor
{
public:
    virtual ~Sensor() = 0;

    /**
     * Refresh the sensor value. Due the physical constraints, the sensor may cache its value and refresh only from time
     * to time
     *
     * @return false in case of error
     */
    virtual bool refresh(const Clock::time_point &time) = 0;

    /**
     * Get the current value of the sensor (in degree, percent...)
     */
    virtual float get() const = 0;

    /**
     * Get the pretty name of the sensor
     */
    virtual const char *getName() const = 0;
};
