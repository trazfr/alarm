#include "screen_set_alarm.hpp"

#include "alarm.hpp"
#include "config.hpp"
#include "config_alarm.hpp"
#include "context.hpp"
#include "renderer.hpp"
#include "renderer_sprite.hpp"
#include "renderer_text.hpp"
#include "toolbox_i18n.hpp"

struct ScreenSetAlarm::Impl
{
    enum class Selected
    {
        Hour,
        Minute,
    };

    explicit Impl(Renderer &renderer)
        : arrowUp{renderer.renderSprite(Asset::Arrow, renderer.getWidth() / 2, renderer.getHeight(), Position::Up)},
          arrowDown{renderer.renderSprite(Asset::Arrow, renderer.getWidth() / 2, 0, Position::Down, 2)},
          arrowLeft{renderer.renderSprite(Asset::Arrow, 0, renderer.getHeight() / 2, Position::Left, 3)},
          arrowRight{renderer.renderSprite(Asset::Arrow, renderer.getWidth(), renderer.getHeight() / 2, Position::Right, 1)},
          previousScreen{renderer.renderStaticText(kMargin, renderer.getHeight() - kMargin, _("Hour"), Position::UpLeft, 16)},
          nextScreen{renderer.renderStaticText(renderer.getWidth() - kMargin, renderer.getHeight() - kMargin, _("Alarm\n File"), Position::UpRight, 16)},
          addAlarm{renderer.renderStaticText(renderer.getWidth() - kMargin, kMargin, "+", Position::DownRight)},
          delAlarm{renderer.renderStaticText(kMargin, kMargin, "-", Position::DownLeft)},
          noAlarm{renderer.renderStaticText(renderer.getWidth() / 2, renderer.getHeight() / 2, _("No Alarm"), Position::Center)},
          underline{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() / 2, 5, 1, Position::Up)},
          alarmCounter{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() * 3 / 4, 20, 1, Position::Up, 16)},
          alarmTime{renderer.renderText(renderer.getWidth() / 2, renderer.getHeight() / 2, 5, 1, Position::Center)},
          active{_(" Alarm %d - Disabled"), _(" Alarm %d - Enabled")}
    {
        setSelected(selected);
    }

    RendererSprite arrowUp;
    RendererSprite arrowDown;
    RendererSprite arrowLeft;
    RendererSprite arrowRight;

    RendererTextStatic previousScreen;
    RendererTextStatic nextScreen;
    RendererTextStatic addAlarm;
    RendererTextStatic delAlarm;
    RendererTextStatic noAlarm;

    RendererText underline;
    RendererText alarmCounter;
    RendererText alarmTime;

    size_t alarmIdx = 0;
    Selected selected = Selected::Hour;

    std::array<const char *, 2> active;

    void switchSelected()
    {
        const int selInt = 1 - static_cast<int>(selected);
        setSelected(Selected{selInt});
    }

    void setSelected(Selected sel)
    {
        static constexpr char kUnderline[2][6] = {"__", "   __"};
        underline.set(kUnderline[static_cast<int>(sel)]);
        selected = sel;
    }

    void refreshAlarm(const ConfigAlarm &alarm)
    {
        char buffer[32];
        const int alarmDisplay = alarmIdx >= 100 ? -1 : static_cast<int>(alarmIdx);
        std::sprintf(buffer, active[alarm.isActive()], alarmDisplay);
        alarmCounter.set(buffer);

        std::sprintf(buffer, "%02d:%02d", alarm.getHours(), alarm.getMinutes());
        alarmTime.set(buffer);
    }
};

namespace
{

ConfigAlarm *getAlarm(ScreenSetAlarm::Impl &pimpl, Context &ctx)
{
    if (auto &alarms = ctx.getConfig().getAlarms(); pimpl.alarmIdx < alarms.size())
    {
        return &(*std::next(alarms.begin(), pimpl.alarmIdx));
    }
    return nullptr;
}

} // namespace

ScreenSetAlarm::ScreenSetAlarm(Context &ctx)
    : Screen{ctx}
{
}

ScreenSetAlarm::~ScreenSetAlarm() = default;

void ScreenSetAlarm::enter()
{
    pimpl = std::make_unique<Impl>(ctx.getRenderer());
}

void ScreenSetAlarm::leave()
{
    pimpl = nullptr;
}

void ScreenSetAlarm::run(const Clock::time_point &)
{
    pimpl->previousScreen.print();
    pimpl->nextScreen.print();
    pimpl->addAlarm.print();

    if (const auto *currentAlarm = getAlarm(*pimpl, ctx))
    {
        pimpl->refreshAlarm(*currentAlarm);

        pimpl->delAlarm.print();
        pimpl->arrowUp.print();
        pimpl->arrowDown.print();
        pimpl->arrowLeft.print();
        pimpl->arrowRight.print();
        pimpl->underline.print();
        pimpl->alarmCounter.print();
        pimpl->alarmTime.print();
    }
    else
    {
        pimpl->noAlarm.print();
    }
}

void ScreenSetAlarm::handleClick(Position position)
{
    switch (position)
    {
    case Position::UpLeft:
        ctx.previousScreen();
        break;
    case Position::UpRight:
        ctx.nextScreen();
        break;
    case Position::Right:
        if (pimpl->selected == Impl::Selected::Minute)
        {
            ++(pimpl->alarmIdx);
            if (const auto &alarms = ctx.getConfig().getAlarms(); !alarms.empty())
            {
                pimpl->alarmIdx %= alarms.size();
            }
            else
            {
                pimpl->alarmIdx = 0;
            }
        }
        pimpl->switchSelected();
        break;
    case Position::Left:
        if (pimpl->selected == Impl::Selected::Hour)
        {
            --(pimpl->alarmIdx);
            if (const auto &alarms = ctx.getConfig().getAlarms(); !alarms.empty())
            {
                pimpl->alarmIdx %= alarms.size();
            }
            else
            {
                pimpl->alarmIdx = 0;
            }
        }
        pimpl->switchSelected();
        break;
    case Position::Center:
        if (ConfigAlarm *const alarm = getAlarm(*pimpl, ctx))
        {
            alarm->setActive(!alarm->isActive());
            ctx.getAlarm().reset();
        }
        break;
    case Position::Up:
        if (ConfigAlarm *const alarm = getAlarm(*pimpl, ctx))
        {
            if (pimpl->selected == Impl::Selected::Minute)
            {
                alarm->setMinutes(alarm->getMinutes() + 1);
            }
            else
            {
                alarm->setHours(alarm->getHours() + 1);
            }
            ctx.getAlarm().reset();
        }
        break;
    case Position::Down:
        if (ConfigAlarm *const alarm = getAlarm(*pimpl, ctx))
        {
            if (pimpl->selected == Impl::Selected::Minute)
            {
                alarm->setMinutes(alarm->getMinutes() - 1);
            }
            else
            {
                alarm->setHours(alarm->getHours() - 1);
            }
            ctx.getAlarm().reset();
        }
        break;
    case Position::DownLeft:
        if (ConfigAlarm *const alarm = getAlarm(*pimpl, ctx))
        {
            pimpl->setSelected(Impl::Selected::Hour);
            ctx.deleteAlarm(*alarm);
        }
        ctx.getAlarm().reset();
        break;
    case Position::DownRight:
        pimpl->setSelected(Impl::Selected::Hour);
        ctx.newAlarm();
        pimpl->alarmIdx = ctx.getConfig().getAlarms().size() - 1;
        break;
    default:
        break;
    }
}
