#include <gtest/gtest.h>

#include "config.hpp"
#include "config_alarm.hpp"

#include "serializer_rapidjson.hpp"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

class TestConfig : public ::testing::Test
{
protected:
    Config config;

    void SetUp() override
    {
        config.setAssetsFolder("folder");
    }

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
        config.save(serial);
        EXPECT_EQ(reference, toString(doc));

        Config config2;
        config2.load(DeserializerRapidJSON{doc});

        rapidjson::Document doc2{rapidjson::Type::kObjectType};
        SerializerRapidJSON serial2{doc2, doc2.GetAllocator()};
        config2.save(serial2);
        EXPECT_EQ(reference, toString(doc2));
    }
};

TEST_F(TestConfig, empty)
{
    test(R"({
    "alsa_device": "default",
    "assets_folder": "folder",
    "display_driver": "",
    "display_width": 320,
    "display_height": 240,
    "display_seconds": true,
    "frames_per_second": 25,
    "hand_clock_color": [
        255,
        0,
        0
    ]
})");
}

TEST_F(TestConfig, full)
{
    config.setSensorThermal("/dev/null");
    config.getAlarms().emplace_back();
    config.getAlarms().emplace_back();
    test(R"({
    "alsa_device": "default",
    "assets_folder": "folder",
    "display_driver": "",
    "display_width": 320,
    "display_height": 240,
    "display_seconds": true,
    "frames_per_second": 25,
    "sensor_thermal": "/dev/null",
    "hand_clock_color": [
        255,
        0,
        0
    ],
    "alarms": [
        {
            "active": false,
            "hours": 0,
            "minutes": 0,
            "duration_minutes": 59
        },
        {
            "active": false,
            "hours": 0,
            "minutes": 0,
            "duration_minutes": 59
        }
    ]
})");
}

TEST_F(TestConfig, helpers)
{
    EXPECT_EQ("folder/music/bla.mp3", config.getMusic("bla.mp3"));
    EXPECT_EQ("folder/shader/shader.vert", config.getShader("shader.vert"));
    EXPECT_EQ("folder/textures/blip.dds", config.getTexture("blip.dds"));
}
