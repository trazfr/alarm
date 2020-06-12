#include <gtest/gtest.h>

#include "sensor_thermal.hpp"
#include "toolbox_io.hpp"

#include <fstream>

class TestSensorThermal : public ::testing::Test
{
protected:
    void write(const char *filename, const char *content)
    {
        std::ofstream{filename} << content;
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

TEST_F(TestSensorThermal, carriageReturn)
{
    write("temp", "24000");
    write("type", "myType\n");

    SensorTermal thermal{"."};
    thermal.refresh(Clock::now());

    EXPECT_FLOAT_EQ(24., thermal.get());
    EXPECT_STREQ("myType", thermal.getName());
}

TEST_F(TestSensorThermal, longName)
{
    write("temp", "23500");
    write("type", "my_very_long_name_which_will_be_truncated_at_some_point_during_a_buffer_overflow");

    SensorTermal thermal{"."};
    thermal.refresh(Clock::now());

    EXPECT_FLOAT_EQ(23.5, thermal.get());
    EXPECT_STREQ("my_very_long_name_which_will_be_truncated_at_some_point_during_a", thermal.getName());
}

TEST_F(TestSensorThermal, missingFile)
{
    // this does not throw (in case of disconnection of the sensor from I2C)
    SensorTermal thermal{"/my/non/existing/path"};
    thermal.refresh(Clock::now());

    EXPECT_STREQ("", thermal.getName());
    EXPECT_FLOAT_EQ(0, thermal.get());
}
