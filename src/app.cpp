#include "app.hpp"

#include "config.hpp"
#include "context.hpp"
#include "event.hpp"
#include "renderer.hpp"
#include "screen.hpp"
#include "serializer_rapidjson.hpp"
#include "toolbox_time.hpp"
#include "window.hpp"
#include "window_factory.hpp"

#include <iostream>
#include <thread>

struct App::Impl
{
    explicit Impl(const char *configurationFile)
        : configPersistence{configurationFile}
    {
    }

    Config config;
    FileSerializationHandlerRapidJSON configPersistence;
    WindowFactory windowFactory;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Context> context;
};

namespace
{

bool handleEvent(App::Impl &pimpl, const Event &event)
{
    switch (event.type)
    {
    case EventType::Quit:
        return false;

    case EventType::Click:
        pimpl.context->handleClick(event.click.x, event.click.y);
        break;

    default:
        break;
    }
    return true;
}

Clock::time_point getNextComputedLoop(const Clock::time_point &time, int fps)
{
    const auto loopDuration = std::chrono::nanoseconds(static_cast<int64_t>(1000000000. / fps));
    return time - (time.time_since_epoch() % loopDuration) + loopDuration;
}

} // namespace

App::App(const char *configurationFile)
    : pimpl{std::make_unique<Impl>(configurationFile)}
{
    std::cerr << "Windows: " << pimpl->windowFactory << std::endl;

    if (!pimpl->configPersistence.load(pimpl->config))
    {
        std::cerr << "Configuration file doesn't exist. Creating it" << std::endl;
        if (pimpl->windowFactory.getDriverSize() > 0)
        {
            pimpl->config.setDisplayDriver(pimpl->windowFactory.getDriver(0));
        }
        pimpl->configPersistence.save(pimpl->config);
    }

    auto &window = pimpl->windowFactory.create(pimpl->config.getDisplayDriver(), pimpl->config.getDisplayWidth(), pimpl->config.getDisplayHeight());
    std::cerr << "Created window: " << window << std::endl;
    pimpl->renderer = std::make_unique<Renderer>(pimpl->config);
    std::cerr << "Created renderer: " << *pimpl->renderer;
    pimpl->context = std::make_unique<Context>(pimpl->config, pimpl->configPersistence, *pimpl->renderer);

    std::cerr << "Initialization OK" << std::endl;
}

App::~App() = default;

void App::run()
{
    for (bool loop = true; loop;)
    {
        const auto startLoop = Clock::now();
        Window &window = pimpl->windowFactory.get();

        while (const auto event = window.popEvent())
        {
            loop &= handleEvent(*pimpl, *event);
        }

        window.begin();

        pimpl->renderer->begin();
        pimpl->context->run(startLoop);
        pimpl->renderer->end();

        window.end();

        if (const auto fps = pimpl->config.getFramesPerSecond())
        {
            std::this_thread::sleep_until(getNextComputedLoop(startLoop, fps));
        }
    }
}
