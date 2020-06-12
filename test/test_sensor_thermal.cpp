#include <gtest/gtest.h>

#include "sensor_thermal.hpp"
#include "toolbox_io.hpp"

class TestSensorThermal : public ::testing::Test
{
protected:
    void write(const char *filename, const char *content)
    {
        FILEUnique fd{std::fopen(filename, "wb+")};
        ASSERT_TRUE(fd);
        std::fputs(content, fd.get());
    }

    void TearDown() override
    {
        unlink("temp");
        unlink("type");
    }
};

TEST_F(TestSensorThermal, basic)
{
    write("temp", "23000");
    write("type", "mytype");

    SensorTermal thermal{"."};

    EXPECT_FLOAT_EQ(0., thermal.get());
    EXPECT_STREQ("mytype", thermal.getName());

    const auto now = Clock::now();

    const bool refreshed = thermal.refresh(now);
    EXPECT_TRUE(refreshed);
    EXPECT_FLOAT_EQ(23., thermal.get());
    EXPECT_STREQ("mytype", thermal.getName());

    const bool refreshed2 = thermal.refresh(now + std::chrono::seconds{10});
    EXPECT_TRUE(refreshed2);
}
