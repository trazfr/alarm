#pragma once

#include "serializable.hpp"

#include <list>
#include <memory>

class ConfigAlarm;

/**
 * @brief holds the configuration
 * 
 * It needs a Deserializer / Serializer save it
 */
class Config : public Serializable
{
public:
    struct Impl;

    /**
     * Create a config with default values
     * 
     * @arg alsa_device is default
     * @arg assets_folder is taken from ALARM_ASSETS_DIR ($PWD in debug, /opt/local/alarm/assets in release)
     * @arg display_driver is not defined
     * @arg display_width is 320
     * @arg display_height is 240
     * @arg frames_per_second is 25 (main screen consumes ~2% CPU on a Raspberry PI 1B)
     * @arg sensor_thermal is not defined
     * @arg display_seconds is true (display second hand on the clock)
     * @arg hand_clock_color is red
     * @arg alarms is empty (no default alarm)
     */
    Config();
    ~Config() override;

    const char *getAlsaDevice() const;
    void setAlsaDevice(std::string_view device);

    std::string_view getAssetsFolder() const;
    void setAssetsFolder(std::string_view folder);

    bool displaySeconds() const;
    void setDisplaySeconds(bool d);

    std::string_view getDisplayDriver() const;
    void setDisplayDriver(std::string_view driver);

    int getDisplayWidth() const;
    void setDisplayWidth(int w);

    int getDisplayHeight() const;
    void setDisplayHeight(int h);

    const uint8_t (&getClockHandColor() const)[3];
    uint8_t (&getClockHandColor())[3];

    int getFramesPerSecond() const;
    void setFramesPerSecond(int fps);

    std::string_view getSensorThermal() const;
    void setSensorThermal(std::string_view name);

    const std::list<ConfigAlarm> &getAlarms() const;
    std::list<ConfigAlarm> &getAlarms();

    void load(const Deserializer &deserializer) override;
    void save(Serializer &serializer) const override;

    // helpers
    std::string getMusic(std::string_view filename = {}) const;
    std::string getShader(std::string_view filename = {}) const;
    std::string getTexture(std::string_view filename = {}) const;

private:
    std::unique_ptr<Impl> pimpl;
};
