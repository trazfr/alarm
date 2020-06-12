#include "screen_main.hpp"

#include "alarm.hpp"
#include "config.hpp"
#include "context.hpp"
#include "renderer.hpp"
#include "renderer_clock.hpp"
#include "renderer_sprite.hpp"
#include "renderer_text.hpp"
#include "sensor.hpp"
#include "toolbox_time.hpp"

#include <cstring>

namespace
{
using Buffer_t = char[32];

constexpr int kClockOffsetX = 0;
constexpr int kClockOffsetY = 0;
constexpr int kClockPosX = 120;
constexpr int kClockPosY = 120;
constexpr int kTextMargin = 10;
constexpr int kThermalSize = 7;

constexpr char kDow[][4] = {
    "Dim",
    "Lun",
    "Mar",
    "Mer",
    "Jeu",
    "Ven",
    "Sam",
};
constexpr char kMon[][4] = {
    "Jan",
    "Fev",
    "Mar",
    "Avr",
    "Mai",
    "Jui",
    "Jul",
    "Aou",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
};

} // namespace

struct ScreenMain::Impl
{
    explicit Impl(const Config &config, Renderer &renderer)
        : clock{config, Renderer::getWidth(), Renderer::getHeight(), 120, 120,
                .7, .04,
                .87, .025,
                .87, .015},
          clockSprite{renderer.renderSprite(Asset::Clock, 0, 0, Position::DownLeft, 0)},
          dateText{renderer.renderText(kClockPosX, kClockPosY + 25, 15, 1, Position::Down, 16)},
          timeText{renderer.renderText(kClockPosX, kClockPosY - 25, config.displaySeconds() ? 8 : 5, 1, Position::Up)},
          alarmText{renderer.renderText(Renderer::getWidth() - kTextMargin, kTextMargin, 8, 2, Position::DownRight, 16)},
          thermalText{renderer.renderText(Renderer::getWidth() - kTextMargin, Renderer::getHeight() - kTextMargin, kThermalSize, 1, Position::UpRight, 16)}
    {
    }

    void refreshTime(time_t timeSinceEpoch, bool displaySeconds)
    {
        if (timeSinceEpoch != savedTimeSinceEpoch)
        {
            Buffer_t buffer;
            const struct tm localTime = getLocalTime(timeSinceEpoch);

            std::sprintf(buffer, "%s %02d %s %04d", kDow[localTime.tm_wday], localTime.tm_mday, kMon[localTime.tm_mon], localTime.tm_year + 1900);
            dateText.set(buffer);

            std::sprintf(buffer, "%02d:%02d:%02d", localTime.tm_hour, localTime.tm_min, localTime.tm_sec);
            if (!displaySeconds)
            {
                buffer[5] = '\0';
            }
            timeText.set(buffer);
            savedTimeSinceEpoch = timeSinceEpoch;
        }
    }

    void refreshAlarm(const Alarm &alarm)
    {
        if (const auto nextAlarm = alarm.getNextRun(); nextAlarm != savedNextAlarm)
        {
            if (nextAlarm)
            {
                Buffer_t buffer;
                const auto alarmLocalTime = getLocalTime(*nextAlarm);
                std::sprintf(buffer, "  Alarme   %02d:%02d", alarmLocalTime.tm_hour, alarmLocalTime.tm_min);
                alarmText.set(buffer);
            }
            else
            {
                alarmText.set("  Alarmeen cours");
            }
            savedNextAlarm = nextAlarm;
        }
    }

    void refreshThermal(float value)
    {
        if (std::abs(value - savedThermalValue) > 0.1)
        {
            Buffer_t buffer;
            std::sprintf(buffer, "%5.1f" DEGREE "C", value);
            if (std::strlen(buffer) == kThermalSize)
            {
                thermalText.set(buffer);
            }
            else
            {
                thermalText.set(" ?TEMP?");
            }
            savedThermalValue = value;
        }
    }

    RendererClock clock;
    RendererSprite clockSprite;
    RendererText dateText;
    RendererText timeText;
    RendererText alarmText;
    RendererText thermalText;
    time_t savedTimeSinceEpoch = 0;
    std::optional<Clock::time_point> savedNextAlarm = Clock::from_time_t(0);
    float savedThermalValue = -1000;
};

ScreenMain::ScreenMain(Context &ctx)
    : Screen{ctx}
{
}

ScreenMain::~ScreenMain() = default;

void ScreenMain::enter()
{
    pimpl = std::make_unique<Impl>(ctx.getConfig(),
                                   ctx.getRenderer());
}

void ScreenMain::leave()
{
    pimpl = nullptr;
}

void ScreenMain::run(const Clock::time_point &time)
{

    pimpl->refreshTime(getTimeSinceEpoch(time), ctx.getConfig().displaySeconds());

    if (const auto &alarm = ctx.getAlarm(); alarm.isActive())
    {
        pimpl->refreshAlarm(alarm);
        pimpl->alarmText.print();
    }

    if (const auto thermal = ctx.getThermalSensor())
    {
        pimpl->refreshThermal(thermal->get());
        pimpl->thermalText.print();
    }

    pimpl->clockSprite.print();
    pimpl->dateText.print();
    pimpl->timeText.print();

    const struct tm localTime = getLocalTime(time);
    pimpl->clock.draw(localTime.tm_hour,
                      localTime.tm_min,
                      localTime.tm_sec,
                      std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count() % 1000);
}

void ScreenMain::handleClick(Position position)
{
    switch (position)
    {
    case Position::UpRight:
        ctx.nextScreen();
        break;

    default:
        if (const auto &alarm = ctx.getAlarm(); alarm.isActive() && !alarm.getNextRun())
        {
            ctx.getAlarm().reset();
        }
        break;
    }
}
