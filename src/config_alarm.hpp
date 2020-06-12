#pragma once

#include "serializable.hpp"

#include <memory>

/**
 * @brief Holds the configuration for an alarm
 * 
 * An alarm is:
 * 
 * @arg 1 audio file (mod, mp3, ogg...)
 * @arg flag active / inactive
 * @arg a time of day (hour, minute)
 * @arg a duration (minutes)
 */
class ConfigAlarm : public Serializable
{
public:
    struct Impl;

    /**
     * Create an alarm with default values:
     * 
     * @arg no audio file
     * @arg inactive
     * @arg stars at midnight
     * @arg runs for 59 minutes
     */
    ConfigAlarm();
    ~ConfigAlarm() override;

    void setActive(bool active);
    void setHours(int hours);
    void setMinutes(int minutes);
    void setDurationMinutes(int minutes);
    void setFile(std::string_view filename);

    bool isActive() const;
    int getHours() const;
    int getMinutes() const;
    int getDurationMinutes() const;
    std::string_view getFile() const;

    void load(const Deserializer &deserializer) override;
    void save(Serializer &serializer) const override;

private:
    std::unique_ptr<Impl> pimpl;
};
