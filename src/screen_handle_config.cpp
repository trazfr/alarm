#include "screen_handle_config.hpp"

#include "config.hpp"
#include "context.hpp"
#include "renderer.hpp"
#include "renderer_text.hpp"
#include "toolbox_i18n.hpp"

struct ScreenHandleConfig::Impl
{
    explicit Impl(Renderer &renderer)
        : previousScreen{renderer.renderStaticText(kMargin, renderer.getHeight() - kMargin, _("Sensor"), Position::UpLeft, 16)},
          save{renderer.renderStaticText(kMargin, renderer.getHeight() / 2, _("Save  \nConfig"), Position::Left, 16)},
          load{renderer.renderStaticText(renderer.getWidth() - kMargin, renderer.getHeight() / 2, _("  Load\nConfig"), Position::Right, 16)}
    {
    }

    RendererTextStatic previousScreen;
    RendererTextStatic save;
    RendererTextStatic load;
};

ScreenHandleConfig::ScreenHandleConfig(Context &ctx)
    : Screen{ctx}
{
}

ScreenHandleConfig::~ScreenHandleConfig() = default;

void ScreenHandleConfig::enter()
{
    pimpl = std::make_unique<Impl>(ctx.getRenderer());
}

void ScreenHandleConfig::leave()
{
    pimpl = nullptr;
}

void ScreenHandleConfig::run(const Clock::time_point &)
{
    pimpl->previousScreen.print();
    pimpl->save.print();
    pimpl->load.print();
}

void ScreenHandleConfig::handleClick(Position position)
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
        ctx.saveConfig();
        break;
    case Position::Right:
        ctx.loadConfig();
        break;
    default:
        break;
    }
}
