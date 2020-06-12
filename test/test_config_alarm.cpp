#include <gtest/gtest.h>

#include "config_alarm.hpp"

#include "serializer_rapidjson.hpp"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

class TestConfigAlarm : public ::testing::Test
{
protected:
    ConfigAlarm configAlarm;

    std::string toString(const rapidjson::Document &doc)
    {
        rapidjson::StringBuffer buffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        return {buffer.GetString(), buffer.GetSize()};
    }

    void test(const char *reference)
    {
        rapidjson::Document doc{rapidjson::Type::kObjectType};
        SerializerRapidJSON serial{doc, doc.GetAllocator()};
        configAlarm.save(serial);
        EXPECT_EQ(reference, toString(doc));

        ConfigAlarm alarm2;
        alarm2.load(DeserializerRapidJSON{doc});

        rapidjson::Document doc2{rapidjson::Type::kObjectType};
        SerializerRapidJSON serial2{doc2, doc2.GetAllocator()};
        alarm2.save(serial2);
        EXPECT_EQ(reference, toString(doc2));
    }
};

TEST_F(TestConfigAlarm, empty)
{
    test(R"({
    "active": false,
    "hours": 0,
    "minutes": 0,
    "duration_minutes": 59
})");
}

TEST_F(TestConfigAlarm, inactive)
{
    configAlarm.setActive(false);
    test(R"({
    "active": false,
    "hours": 0,
    "minutes": 0,
    "duration_minutes": 59
})");
}

TEST_F(TestConfigAlarm, active)
{
    configAlarm.setActive(true);
    test(R"({
    "active": true,
    "hours": 0,
    "minutes": 0,
    "duration_minutes": 59
})");
}

TEST_F(TestConfigAlarm, minutes)
{
    configAlarm.setMinutes(10);
    test(R"({
    "active": false,
    "hours": 0,
    "minutes": 10,
    "duration_minutes": 59
})");

    configAlarm.setMinutes(11);
    test(R"({
    "active": false,
    "hours": 0,
    "minutes": 11,
    "duration_minutes": 59
})");
}

TEST_F(TestConfigAlarm, hours)
{
    configAlarm.setHours(10);
    test(R"({
    "active": false,
    "hours": 10,
    "minutes": 0,
    "duration_minutes": 59
})");
}

TEST_F(TestConfigAlarm, overflow)
{
    configAlarm.setHours(25);
    test(R"({
    "active": false,
    "hours": 1,
    "minutes": 0,
    "duration_minutes": 59
})");
}

TEST_F(TestConfigAlarm, underflow)
{
    configAlarm.setMinutes(-1);
    test(R"({
    "active": false,
    "hours": 23,
    "minutes": 59,
    "duration_minutes": 59
})");
}

TEST_F(TestConfigAlarm, time)
{
    configAlarm.setHours(13);
    configAlarm.setMinutes(37);
    test(R"({
    "active": false,
    "hours": 13,
    "minutes": 37,
    "duration_minutes": 59
})");

    configAlarm.setHours(12);
    configAlarm.setMinutes(42);
    test(R"({
    "active": false,
    "hours": 12,
    "minutes": 42,
    "duration_minutes": 59
})");
}

TEST_F(TestConfigAlarm, duration)
{
    configAlarm.setDurationMinutes(10);
    test(R"({
    "active": false,
    "hours": 0,
    "minutes": 0,
    "duration_minutes": 10
})");

    configAlarm.setDurationMinutes(11);
    test(R"({
    "active": false,
    "hours": 0,
    "minutes": 0,
    "duration_minutes": 11
})");
}

TEST_F(TestConfigAlarm, file)
{
    configAlarm.setFile("filename");
    test(R"({
    "active": false,
    "file": "filename",
    "hours": 0,
    "minutes": 0,
    "duration_minutes": 59
})");
}
