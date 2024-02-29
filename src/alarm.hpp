#pragma once

#include "toolbox_time.hpp"

#include <memory>
#include <optional>

class Audio;
class Config;

/**
 * @brief Handle which alarm to run and when to start and stop it
 */
class Alarm
{
public:
    struct Impl;

    /**
     * @attention the object keeps a reference to constructor's arguments. Make sure they stay valid
     */
    Alarm(const Config &config, Audio &audio);
    ~Alarm();

    bool isActive() const;

    /**
     * @return the next time an alarm will start
     */
    std::optional<Clock::time_point> getNextRun() const;

    /**
     * Clear the internal state so that the next call to run() will reinitialize the state
     *
     * This has to be run each time the configuration is updated
     */
    void reset();

    /**
     * Method to be run at each loop to ensure the audio buffers are filled and the alarm is started/stopped according
     * to the configuration.
     *
     * This is done by the Context under normal operation
     */
    void run(const Clock::time_point &time);

private:
    std::unique_ptr<Impl> pimpl;
};
