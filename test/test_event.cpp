#include <gtest/gtest.h>

#include "event.hpp"

class TestEvent : public ::testing::Test
{
};

TEST_F(TestEvent, quit)
{
    const Event event = Event::createQuit();
    EXPECT_EQ(EventType::Quit, event.type);
}

TEST_F(TestEvent, click)
{
    const Event event = Event::createClick(.5, 1);
    EXPECT_EQ(EventType::Click, event.type);
    EXPECT_FLOAT_EQ(.5, event.click.x);
    EXPECT_FLOAT_EQ(1, event.click.y);
}
