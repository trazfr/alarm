#include <gtest/gtest.h>

#include "window.hpp"
#include "window_factory.hpp"

#include <sstream>
#include <stdexcept>

class TestWindowFactory : public ::testing::Test
{
protected:
    WindowFactory factory;
};

TEST_F(TestWindowFactory, getAnyDriver)
{
    // there is at least 1 driver (otherwise we cannot compile)
    const auto &window = factory.create(factory.getDriver(0), 320, 240);

    std::ostringstream str;
    str << window;
    EXPECT_FALSE(str.str().empty());
}

TEST_F(TestWindowFactory, getDebug)
{
    std::ostringstream str;
    str << factory;
    EXPECT_FALSE(str.str().empty());
}

TEST_F(TestWindowFactory, getNonInitialized)
{
    try
    {
        factory.get();
        FAIL() << "Should have thrown";
    }
    catch (const std::runtime_error &)
    {
    }
}

TEST_F(TestWindowFactory, createInvalidDriverName)
{
    try
    {
        factory.create("INVALID_DRIVER", 320, 240);
        FAIL() << "Should have thrown";
    }
    catch (const std::runtime_error &)
    {
    }
}

TEST_F(TestWindowFactory, createInvalidScreenSize)
{
    try
    {
        factory.create(factory.getDriver(0), 320, 100000);
        FAIL() << "Should have thrown";
    }
    catch (const std::runtime_error &)
    {
    }
}

TEST_F(TestWindowFactory, getInvalidDriverIndex)
{
    try
    {
        factory.getDriver(factory.getDriverSize());
        FAIL() << "Should have thrown";
    }
    catch (const std::runtime_error &)
    {
    }
}
