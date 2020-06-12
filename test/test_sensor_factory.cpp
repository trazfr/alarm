#include <gtest/gtest.h>

#include "sensor_factory.hpp"

class TestSensorFactory : public ::testing::Test
{
protected:
    SensorFactory factory;
};

TEST_F(TestSensorFactory, debug)
{
    std::ostringstream str;
    str << factory;
    EXPECT_FALSE(str.str().empty());
}

TEST_F(TestSensorFactory, basic)
{
    // this test is hardware dependent
    for (size_t i = 0; i < factory.getThermalSize(); ++i)
    {
        const Sensor *sensor = factory.getThermal(i);
        EXPECT_TRUE(sensor);
    }
}

TEST_F(TestSensorFactory, overflow)
{
    EXPECT_FALSE(factory.getThermal(std::numeric_limits<size_t>::max()));
}
