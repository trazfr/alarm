#include "config_alarm.hpp"

#include "serializer.hpp"

#include <chrono>

namespace
{
using TimeUnit = std::chrono::seconds;

constexpr char kKeyActive[] = "active";
constexpr char kKeyFile[] = "file";
constexpr char kKeyHours[] = "hours";
constexpr char kKeyMinutes[] = "minutes";
constexpr char kKeyDurationMinutes[] = "duration_minutes";

} // namespace

struct ConfigAlarm::Impl
{
    bool active = false;
    std::string file;
    TimeUnit alarmTime;
    TimeUnit duration = std::chrono::minutes{59};

    void fixAlarmTime()
    {
        alarmTime %= std::chrono::hours{24};
        if (alarmTime.count() < 0)
        {
            alarmTime += std::chrono::hours{24};
        }
    }
};

ConfigAlarm::ConfigAlarm() : pimpl{std::make_unique<Impl>()} {}

ConfigAlarm::~ConfigAlarm() = default;

void ConfigAlarm::setActive(bool active)
{
    pimpl->active = active;
}

void ConfigAlarm::setHours(int hours)
{
    pimpl->alarmTime += std::chrono::hours{hours - getHours()};
    pimpl->fixAlarmTime();
}

void ConfigAlarm::setMinutes(int minutes)
{
    pimpl->alarmTime += std::chrono::minutes{minutes - getMinutes()};
    pimpl->fixAlarmTime();
}

void ConfigAlarm::setDurationMinutes(int minutes)
{
    pimpl->duration = std::max(std::chrono::minutes{minutes}, std::chrono::minutes{1});
}

void ConfigAlarm::setFile(std::string_view filename)
{
    pimpl->file = filename;
}

bool ConfigAlarm::isActive() const
{
    return pimpl->active;
}

int ConfigAlarm::getHours() const
{
    return std::chrono::duration_cast<std::chrono::hours>(pimpl->alarmTime).count();
}

int ConfigAlarm::getMinutes() const
{
    return std::chrono::duration_cast<std::chrono::minutes>(pimpl->alarmTime % std::chrono::hours(1)).count();
}

int ConfigAlarm::getDurationMinutes() const
{
    return std::chrono::duration_cast<std::chrono::minutes>(pimpl->duration).count();
}

std::string_view ConfigAlarm::getFile() const
{
    return pimpl->file;
}

void ConfigAlarm::save(Serializer &serializer) const
{
    serializer.setBool(kKeyActive, isActive());

    if (const auto file = getFile(); file.empty() == false)
    {
        serializer.setString(kKeyFile, file);
    }
    serializer.setInt(kKeyHours, getHours());
    serializer.setInt(kKeyMinutes, getMinutes());
    serializer.setInt(kKeyDurationMinutes, getDurationMinutes());
}

void ConfigAlarm::load(const Deserializer &deserializer)
{
    if (const auto val = deserializer.getBool(kKeyActive))
    {
        setActive(*val);
    }
    if (const auto val = deserializer.getInt(kKeyMinutes))
    {
        setMinutes(*val);
    }
    if (const auto val = deserializer.getInt(kKeyHours))
    {
        setHours(*val);
    }
    if (const auto val = deserializer.getInt(kKeyDurationMinutes))
    {
        setDurationMinutes(*val);
    }
    if (const auto val = deserializer.getString(kKeyFile))
    {
        setFile(*val);
    }
}
