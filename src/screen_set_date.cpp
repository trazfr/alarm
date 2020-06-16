#include "screen_set_date.hpp"

#include "context.hpp"
#include "renderer.hpp"
#include "renderer_sprite.hpp"
#include "renderer_text.hpp"
#include "toolbox_i18n.hpp"

#include <cstring>
#include <iostream>

namespace
{

constexpr char kErrCannotChangeDate[] = "Erreur lors du changement";
using BufferTime_t = char[20];

struct Offset
{
    size_t begin;
    size_t size;
};

enum class Select
{
    Day,
    Month,
    Year,
    Hour,
    Minute,
    Second,
};

constexpr Offset getOffset(Select select)
{
    switch (select)
    {
    case Select::Day:
        return {0, 2};
    case Select::Month:
        return {3, 2};
    case Select::Year:
        return {6, 4};
    case Select::Hour:
        return {11, 2};
    case Select::Minute:
        return {14, 2};
    case Select::Second:
        return {17, 2};
    default:
        return {0, 0};
    }
}

bool setDate(const Clock::time_point &time, Select select, int offset)
{
    const time_t ctime = Clock::to_time_t(time);
    struct tm utc;
    gmtime_r(&ctime, &utc);

    switch (select)
    {
    case Select::Day:
        utc.tm_mday += offset;
        break;
    case Select::Month:
        utc.tm_mon += offset;
        break;
    case Select::Year:
        utc.tm_year += offset;
        break;
    case Select::Hour:
        utc.tm_hour += offset;
        break;
    case Select::Minute:
        utc.tm_min += offset;
        break;
    case Select::Second:
        utc.tm_sec += offset;
        break;
    default:
        break;
    }

    const time_t fixedTime = mktime(&utc);

    if (stime(&fixedTime))
    {
        std::cerr << "Cannot set time to " << fixedTime << std::endl;
        return false;
    }
    return true;
}

} // namespace

struct ScreenSetDate::Impl
{
    explicit Impl(Renderer &renderer)
        : arrowUp{renderer.renderSprite(Asset::Arrow, renderer.getWidth() / 2, renderer.getHeight(), Position::Up)},
          arrowDown{renderer.renderSprite(Asset::Arrow, renderer.getWidth() / 2, 0, Position::Down, 2)},
          arrowLeft{renderer.renderSprite(Asset::Arrow, 0, renderer.getHeight() / 2, Position::Left, 3)},
          arrowRight{renderer.renderSprite(Asset::Arrow, renderer.getWidth(), renderer.getHeight() / 2, Position::Right, 1)},
          previousScreen{renderer.renderStaticText(kMargin, renderer.getHeight() - kMargin, _("Alarm\nFile"), Position::UpLeft, 16)},
          nextScreen{renderer.renderStaticText(renderer.getWidth() - kMargin, renderer.getHeight() - kMargin, _("Sensor"), Position::UpRight, 16)},
          errorText{renderer.renderStaticText(renderer.getWidth() / 2, renderer.getHeight() * 7 / 10, kErrCannotChangeDate, Position::Center, 16)},
          dateText{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() / 2, 19, 1, Position::Center, 16)},
          dateUnderline{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() / 2, 19, 1, Position::Up, 16)}
    {
        changeSelect(Select::Day);
    }

    void refreshDate(const Clock::time_point &time)
    {
        const std::time_t newSecondsSinceEpoch = getTimeSinceEpoch(time);
        if (newSecondsSinceEpoch != secondsSinceEpoch)
        {
            BufferTime_t bufferTime;
            const struct tm localtime = getLocalTime(newSecondsSinceEpoch);
            if (const size_t timeSize = strftime(bufferTime, sizeof(bufferTime), "%d/%m/%Y %H:%M:%S", &localtime))
            {
                dateText.set(bufferTime);
                secondsSinceEpoch = newSecondsSinceEpoch;
            }
        }
    }

    void changeSelect(Select newValue)
    {
        BufferTime_t buffer;
        std::memset(buffer, ' ', 19);
        const auto offsets = getOffset(newValue);
        std::memset(buffer + offsets.begin, '_', offsets.size);
        buffer[sizeof(BufferTime_t) - 1] = '\0';
        dateUnderline.set(buffer);
        select = newValue;
    }

    RendererSprite arrowUp;
    RendererSprite arrowDown;
    RendererSprite arrowLeft;
    RendererSprite arrowRight;

    RendererTextStatic previousScreen;
    RendererTextStatic nextScreen;
    RendererTextStatic errorText;

    RendererText dateText;
    RendererText dateUnderline;

    std::time_t secondsSinceEpoch = 0;
    bool error = false;
    Select select = Select::Day;
    Clock::time_point time;
};

ScreenSetDate::ScreenSetDate(Context &ctx)
    : Screen{ctx}
{
}

ScreenSetDate::~ScreenSetDate() = default;

void ScreenSetDate::enter()
{
    pimpl = std::make_unique<Impl>(ctx.getRenderer());
}

void ScreenSetDate::leave()
{
    pimpl = nullptr;
}

void ScreenSetDate::run(const Clock::time_point &time)
{
    pimpl->refreshDate(time);

    pimpl->previousScreen.print();
    pimpl->nextScreen.print();

    if (pimpl->secondsSinceEpoch)
    {
        pimpl->dateUnderline.print();
        pimpl->dateText.print();
        pimpl->arrowUp.print();
        pimpl->arrowDown.print();
        if (pimpl->select != Select::Day)
        {
            pimpl->arrowLeft.print();
        }
        if (pimpl->select != Select::Second)
        {
            pimpl->arrowRight.print();
        }
    }

    if (pimpl->error)
    {
        pimpl->errorText.print();
    }

    pimpl->time = time;
}

void ScreenSetDate::handleClick(Position position)
{
    switch (position)
    {
    case Position::UpLeft:
        ctx.previousScreen();
        break;
    case Position::UpRight:
        ctx.nextScreen();
        break;

    case Position::Left:
        if (pimpl->select != Select::Day)
        {
            pimpl->changeSelect(Select{static_cast<int>(pimpl->select) - 1});
        }
        break;
    case Position::Right:
        if (pimpl->select != Select::Second)
        {
            pimpl->changeSelect(Select{static_cast<int>(pimpl->select) + 1});
        }
        break;
    case Position::Up:
        if (setDate(pimpl->time, pimpl->select, 1) == false)
        {
            pimpl->error = true;
        }
        break;
    case Position::Down:
        if (setDate(pimpl->time, pimpl->select, -1) == false)
        {
            pimpl->error = true;
        }
        break;
    default:
        break;
    }
}
