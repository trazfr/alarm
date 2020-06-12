#include "alarm.hpp"

#include "audio.hpp"
#include "config.hpp"
#include "config_alarm.hpp"
#include "toolbox_time.hpp"

#include <iostream>

struct Alarm::Impl
{
    static constexpr auto kInvalidTime = Clock::time_point::max();

    explicit Impl(const Config &config, Audio &audio)
        : config{config},
          audio{audio}
    {
    }
    const Config &config;
    Audio &audio;

    const ConfigAlarm *nextAlarm = nullptr;
    Clock::time_point timeStartNextAlarm = kInvalidTime;
    Clock::time_point timeStopMusic = kInvalidTime;
};

namespace
{

Clock::time_point getNextAlarm(const ConfigAlarm &alarm, const struct tm &now)
{
    const std::chrono::seconds requestedTimeOfDay = std::chrono::hours(alarm.getHours()) +
                                                    std::chrono::minutes(alarm.getMinutes());
    const std::chrono::seconds nowTimeOfDay = std::chrono::hours(now.tm_hour) +
                                              std::chrono::minutes(now.tm_min) +
                                              std::chrono::seconds(now.tm_sec);
    struct tm alarmTime = now;

    // update time of day
    alarmTime.tm_hour = alarm.getHours();
    alarmTime.tm_min = alarm.getMinutes();
    alarmTime.tm_sec = 0;

    // useless
    alarmTime.tm_wday = 0;
    alarmTime.tm_yday = 0;
    alarmTime.tm_isdst = -1;

    if (nowTimeOfDay >= requestedTimeOfDay)
    {
        ++alarmTime.tm_mday;
    }

    std::cerr << "Next alarm: " << 1900 + alarmTime.tm_year << '-' << 1 + alarmTime.tm_mon << '-' << alarmTime.tm_mday
              << ' ' << alarmTime.tm_hour << ':' << alarmTime.tm_min << ':' << alarmTime.tm_sec
              << std::endl;

    return Clock::from_time_t(mktime(&alarmTime));
}

std::pair<const ConfigAlarm *, Clock::time_point> getNextAlarm(const std::list<ConfigAlarm> &alarms, const Clock::time_point &now)
{
    std::pair<const ConfigAlarm *, Clock::time_point> result{
        nullptr,
        Clock::time_point::max(),
    };
    const auto nowTm = getLocalTime(now);

    for (const ConfigAlarm &alarm : alarms)
    {
        if (alarm.isActive())
        {
            const auto alarmTime = getNextAlarm(alarm, nowTm);
            if (alarmTime < result.second)
            {
                result = {&alarm, alarmTime};
            }
        }
    }
    return result;
}

} // namespace

Alarm::Alarm(const Config &config, Audio &audio)
    : pimpl{std::make_unique<Impl>(config, audio)}
{
}

Alarm::~Alarm() = default;

bool Alarm::isActive() const
{
    return pimpl->timeStartNextAlarm != Impl::kInvalidTime ||
           pimpl->timeStopMusic != Impl::kInvalidTime;
}

std::optional<Clock::time_point> Alarm::getNextRun() const
{
    if (pimpl->timeStartNextAlarm != Impl::kInvalidTime)
    {
        return {pimpl->timeStartNextAlarm};
    }
    return {};
}

void Alarm::reset()
{
    pimpl->nextAlarm = nullptr;
    pimpl->timeStartNextAlarm = Impl::kInvalidTime;
    pimpl->timeStopMusic = Clock::time_point::min();
}

void Alarm::run(const Clock::time_point &time)
{
    // the time to start the alarm has been reached
    if (pimpl->nextAlarm && time >= pimpl->timeStartNextAlarm)
    {
        if (const auto configFilename = pimpl->nextAlarm->getFile(); configFilename.empty() == false)
        {
            const auto filename = pimpl->config.getMusic(configFilename);

            std::cerr << "Start music: " << filename << std::endl;
            pimpl->audio.stopStream();
            if (pimpl->audio.loadStream(filename.c_str()) == false)
            {
                std::cerr << "Could not load the stream" << std::endl;
            }
            pimpl->audio.playStream();

            pimpl->timeStopMusic = pimpl->timeStartNextAlarm + std::chrono::minutes(pimpl->nextAlarm->getDurationMinutes());
            pimpl->timeStartNextAlarm = Impl::kInvalidTime;
            pimpl->nextAlarm = nullptr;
        }
    }

    // the time to stop the alarm has been reached
    if (time >= pimpl->timeStopMusic)
    {
        pimpl->audio.stopStream();
        pimpl->timeStopMusic = Impl::kInvalidTime;
    }

    // not running and next alarm not programmed
    if (pimpl->audio.run() == false && isActive() == false)
    {
        if (const auto &alarms = pimpl->config.getAlarms(); alarms.empty() == false)
        {
            std::tie(pimpl->nextAlarm, pimpl->timeStartNextAlarm) = getNextAlarm(alarms, time);
        }
    }
}
