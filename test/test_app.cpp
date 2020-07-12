// this is a bit hacky
#define private public
#include "app.hpp"
#include "window_factory.hpp"
#undef private

#include "config.hpp"
#include "config_alarm.hpp"
#include "event.hpp"
#include "serializer_rapidjson.hpp"
#include "window.hpp"
#include "windowevent.hpp"

#include <gtest/gtest.h>

#include <list>

// these tests must be disabled in release mode due to a wrong assets default path
#ifndef RELEASE_MODE
#define ONLY_DEBUG_MODE(x) x
#else
#define ONLY_DEBUG_MODE(x) DISABLED_##x
#endif

namespace
{
constexpr char kFilename[] = "test_config.json";

class WindowEventMock : public WindowEvent
{
public:
    using Storage = std::list<std::optional<Event>>;
    explicit WindowEventMock(Storage &events) : events{events} {}

    std::optional<Event> popEvent()
    {
        assert(events.empty() == false);

        auto result = events.front();
        events.pop_front();
        return result;
    }

private:
    virtual std::ostream &toStream(std::ostream &str) const { return str << "/!\\ dummy"; }

    Storage &events;
};

/**
 * Override the events of the window passed in the constructor
 */
class MockWindow : public Window
{
public:
    explicit MockWindow(std::unique_ptr<Window> window)
        : window{std::move(window)},
          defaultEvent{this->window->createDefaultEvent()}
    {
    }
    ~MockWindow() override
    {
        while (defaultEvent->popEvent())
        {
        }
    }

    void begin() override
    {
        ++numberCallsBegin;
        window->begin();
    }

    void end() override
    {
        ++numberCallsEnd;
        window->end();
        // avoid saturation
        while (defaultEvent->popEvent())
        {
        }
    }

    std::unique_ptr<WindowEvent> createDefaultEvent()
    {
        return std::make_unique<WindowEventMock>(events);
    }

    std::ostream &toStream(std::ostream &str) const override
    {
        return str << "/!\\ WRAPPER /!\\\n"
                   << *window;
    }

    /**
     * Create 1 Event per frame
     */
    template <size_t N>
    void setEvents(const Event (&array)[N])
    {
        setEvents(&array[0], &array[N]);
    }
    template <typename Iterator>
    void setEvents(Iterator begin, Iterator end)
    {
        events.clear();
        for (Iterator it = begin; it != end; ++it)
        {
            events.emplace_back(*it);
            events.emplace_back();
        }
    }

    unsigned int numberCallsBegin = 0;
    unsigned int numberCallsEnd = 0;

private:
    std::unique_ptr<Window> window;
    std::unique_ptr<WindowEvent> defaultEvent;
    WindowEventMock::Storage events;
};

} // namespace

class TestApp : public ::testing::Test
{
protected:
    void SetUp() override
    {
        unlink(kFilename);

        app = std::make_unique<App>(kFilename);
        /// @attention due to this, the WindowFactory must be at the 1st position of App::Impl
        const auto windowFactory = reinterpret_cast<WindowFactory *>(app->pimpl.get());
        auto mockTmp = std::make_unique<MockWindow>(std::move(windowFactory->window));

        mockWindow = mockTmp.get();
        windowFactory->window = std::move(mockTmp);
        windowFactory->windowEvent = windowFactory->window->createDefaultEvent();
    }

    void TearDown() override
    {
        unlink(kFilename);
    }

    static Event createClick(Position position)
    {
        switch (position)
        {
        case Position::DownLeft:
            return Event::createClick(0, 1);
        case Position::Down:
            return Event::createClick(.5, 1);
        case Position::DownRight:
            return Event::createClick(1, 1);
        case Position::Left:
            return Event::createClick(0, .5);
        case Position::Center:
            return Event::createClick(.5, .5);
        case Position::Right:
            return Event::createClick(1, .5);
        case Position::UpLeft:
            return Event::createClick(0, 0);
        case Position::Up:
            return Event::createClick(.5, 0);
        case Position::UpRight:
        default:
            return Event::createClick(1, 0);
        }
    }

    FileSerializationHandlerRapidJSON serial{kFilename};
    std::unique_ptr<App> app;
    MockWindow *mockWindow = nullptr;
};

TEST_F(TestApp, ONLY_DEBUG_MODE(onlyQuit))
{
    const Event events[] = {
        Event::createQuit(),
    };
    mockWindow->setEvents(events);
    app->run();

    EXPECT_EQ(1, mockWindow->numberCallsBegin);
    EXPECT_EQ(1, mockWindow->numberCallsEnd);

    Config config;
    serial.load(config);
    EXPECT_TRUE(config.getAlarms().empty());
}

TEST_F(TestApp, ONLY_DEBUG_MODE(fillAlarms))
{
    const Event events[] = {
        // go to screen_set_alarm
        createClick(Position::UpRight),
        // create active alarm0 to (22:59)
        createClick(Position::DownRight), // create alarm0 00:00 inactive
        createClick(Position::Down),      // alarm0 23:00 inactive
        createClick(Position::Left),      // alarm0 23:00 inactive (there is only 1 alarms so left is OK too)
        createClick(Position::Down),      // alarm0 22:59 inactive
        createClick(Position::Center),    // alarm0 22:59 active
        // create inactive alarm1 to (01:01)
        createClick(Position::DownRight), // alarm1 00:00 inactive
        createClick(Position::Up),        // alarm1 01:00 inactive
        createClick(Position::Right),     // alarm1 01:00 inactive
        createClick(Position::Up),        // alarm1 01:01 inactive
        createClick(Position::Center),    // alarm1 01:01 active
        createClick(Position::Center),    // alarm1 01:01 inactive
        // create inactive alarm2 + delete it
        createClick(Position::DownRight),
        createClick(Position::DownLeft),

        // go to screen_set_sensor (skip screen_set_alarm_file, screen_set_date)
        createClick(Position::UpRight), // now at screen_set_alarm_file
        createClick(Position::UpRight), // now at screen_set_date
        createClick(Position::UpRight),
        createClick(Position::Up), // navigate through sensors (if any...)
        createClick(Position::Up),
        createClick(Position::Down),

        // go to screen_handle_config
        createClick(Position::UpRight),
        createClick(Position::Left),  // save
        createClick(Position::Right), // load

        // go back to screen_main
        createClick(Position::UpLeft), // now at screen_set_sensor
        createClick(Position::UpLeft), // now at screen_set_date
        createClick(Position::UpLeft), // now at screen_set_alarm_file
        createClick(Position::UpLeft), // now at screen_set_alarm
        createClick(Position::UpLeft), // now at screen_main

        Event::createQuit(),
    };

    const size_t numberEvents = sizeof(events) / sizeof(*events);

    mockWindow->setEvents(events);
    app->run();

    // there is 1 begin/end per frame, 1 frame per empty event, and we build 1 empty event per click to refresh the screen
    EXPECT_EQ(numberEvents, mockWindow->numberCallsBegin);
    EXPECT_EQ(numberEvents, mockWindow->numberCallsEnd);

    Config config;
    serial.load(config);
    EXPECT_EQ(2, config.getAlarms().size());

    const ConfigAlarm &alarm0 = config.getAlarms().front();
    EXPECT_TRUE(alarm0.isActive());
    EXPECT_EQ(59, alarm0.getDurationMinutes()); // cannot be changed in the UI
    EXPECT_EQ(22, alarm0.getHours());
    EXPECT_EQ(59, alarm0.getMinutes());

    const ConfigAlarm &alarm1 = config.getAlarms().back();
    EXPECT_FALSE(alarm1.isActive());
    EXPECT_EQ(59, alarm1.getDurationMinutes()); // cannot be changed in the UI
    EXPECT_EQ(1, alarm1.getHours());
    EXPECT_EQ(1, alarm1.getMinutes());
}
