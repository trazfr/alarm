#include <gtest/gtest.h>

#include "sensor_factory.hpp"

class TestSensorFactory : public ::testing::Test
{
};

TEST_F(TestSensorFactory, basic)
{
    // this test is hardware dependent
    SensorFactory factory;
    for (size_t i = 0; i < factory.getThermalSize(); ++i)
    {
        const Sensor *sensor = factory.getThermal(i);
        ASSERT_TRUE(sensor);
    }

    EXPECT_FALSE(factory.getThermal(std::numeric_limits<size_t>::max()));

    std::ostringstream str;
    str << factory;
    EXPECT_FALSE(str.str().empty());
}
