#pragma once

#include "toolbox_time.hpp"

#include <memory>

class Alarm;
class Audio;
class Config;
class ConfigAlarm;
class Renderer;
class SerializationHandler;
class Screen;
class Sensor;
class SensorFactory;

/**
 * @brief Holds the whole context of the application (GL rendering, audio, config...)
 */
class Context
{
public:
    struct Impl;

    /**
     * @attention the object keeps a reference to constructor's arguments. Make sure they stay valid
     */
    Context(Config &config, SerializationHandler &configPersistence, Renderer &renderer);
    ~Context();

    /**
     * Get the screen renderer
     */
    Renderer &getRenderer();

    /**
     * Get the audio system
     */
    Audio &getAudio();

    Config &getConfig();
    const Config &getConfig() const;
    void saveConfig();
    void loadConfig();

    Alarm &getAlarm();
    void newAlarm();
    void deleteAlarm(ConfigAlarm &alarm);

    /**
     * Get the Screen system (Raspberry PI's Dispman, SDL2, Wayland...)
     */
    Screen &getScreen();
    void previousScreen();
    void nextScreen();

    /**
     * Refresh the sensor used. To be called when the configuration has changed
     */
    void resetSensors();
    SensorFactory &getSensorFactory();
    Sensor *getTemperatureSensor();

    // handle asynchronous events
    void handleClick(float x, float y);

    /**
     * To be called at each loop. It takes care in to call the run() method on all subsystems
     *
     * @arg Alarm to refresh the alarm
     * @arg Screen to refresh the display
     * @arg refresh the active Sensor if any
     */
    void run(const Clock::time_point &time);

private:
    std::unique_ptr<Impl> pimpl;
};
