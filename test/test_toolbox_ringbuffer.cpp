#include <gtest/gtest.h>

#include "toolbox_ringbuffer.hpp"

class TestToolboxRingBuffer : public ::testing::Test
{
};

TEST_F(TestToolboxRingBuffer, Basic)
{
    RingBuffer<int, 4> buffer;
    EXPECT_EQ(4, buffer.capacity());
    EXPECT_TRUE(buffer.empty());

    // push buffer content: {0,1,2,3}
    for (int i = 0; i < 4; ++i)
    {
        EXPECT_EQ(i, buffer.size());
        EXPECT_FALSE(buffer.full());

        const bool pushed = buffer.push(i);
        EXPECT_TRUE(pushed);
        EXPECT_FALSE(buffer.empty());

        EXPECT_EQ(i + 1, buffer.size());
    }

    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(4, buffer.size());
    EXPECT_FALSE(buffer.empty());

    // full, do not push
    {
        const bool pushed = buffer.push(1);

        EXPECT_FALSE(pushed);
    }

    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(4, buffer.size());
    EXPECT_FALSE(buffer.empty());

    // {0,1,2,3} => {3}
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(4 - i, buffer.size());

        const auto pop = buffer.pop();
        ASSERT_TRUE(pop);

        EXPECT_EQ(i, *pop);
        EXPECT_EQ(4 - i - 1, buffer.size());
        EXPECT_FALSE(buffer.full());
        EXPECT_FALSE(buffer.empty());
    }

    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(1, buffer.size());
    EXPECT_FALSE(buffer.empty());

    // {3} => {3,4,5,6}
    for (int i = 4; i < 7; ++i)
    {
        EXPECT_EQ(i - 3, buffer.size());
        const bool pushed = buffer.push(i);

        EXPECT_TRUE(pushed);

        EXPECT_EQ(i - 2, buffer.size());
    }

    EXPECT_TRUE(buffer.full());
    EXPECT_EQ(4, buffer.size());
    EXPECT_FALSE(buffer.empty());

    // {3,4,5,6} => {}
    for (int i = 3; i < 7; ++i)
    {
        EXPECT_EQ(7 - i, buffer.size());

        const auto pop = buffer.pop();
        ASSERT_TRUE(pop);

        EXPECT_EQ(i, *pop);
        EXPECT_EQ(7 - i - 1, buffer.size());
        EXPECT_FALSE(buffer.full());
    }

    EXPECT_FALSE(buffer.full());
    EXPECT_EQ(0, buffer.size());
    EXPECT_TRUE(buffer.empty());
    const auto pop = buffer.pop();
    EXPECT_FALSE(pop);
}

TEST_F(TestToolboxRingBuffer, NoCopy)
{
    RingBuffer<std::unique_ptr<int>, 4> buffer;
    EXPECT_TRUE(buffer.empty());

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FALSE(buffer.full());
        const bool pushed = buffer.push(std::make_unique<int>(i));
        EXPECT_TRUE(pushed);
        EXPECT_FALSE(buffer.empty());
    }
    EXPECT_EQ(4, buffer.size());
    EXPECT_TRUE(buffer.full());
    EXPECT_FALSE(buffer.empty());

    {
        const bool pushed = buffer.push(std::make_unique<int>(123));
        EXPECT_FALSE(pushed);
    }

    for (int i = 0; i < 4; ++i)
    {
        EXPECT_FALSE(buffer.empty());
        const auto pop = buffer.pop();
        ASSERT_TRUE(pop);
        ASSERT_TRUE(*pop);
        EXPECT_EQ(i, **pop);
        EXPECT_FALSE(buffer.full());
    }

    EXPECT_TRUE(buffer.empty());
    const auto pop = buffer.pop();
    EXPECT_FALSE(pop);
}
