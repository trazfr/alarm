#include <gtest/gtest.h>

#include "serializable.hpp"
#include "serializer_rapidjson.hpp"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <fstream>
#include <string>
#include <vector>

class DummyChild : public Serializable
{
public:
    void save(Serializer &serializer) const override
    {
        if (value)
        {
            serializer.setInt("value", *value);
        }
    }

    void load(const Deserializer &deserializer) override
    {
        value = deserializer.getInt("value");
    }

    std::optional<int64_t> value;
};

class DummySerializable : public Serializable
{
public:
    ~DummySerializable() override = default;

    void save(Serializer &serializer) const override
    {
        for (auto &kv : booleans)
        {
            if (kv.second)
            {
                serializer.setBool(kv.first.data(), *kv.second);
            }
        }
        for (auto &kv : floats)
        {
            if (kv.second)
            {
                serializer.setFloat(kv.first.data(), *kv.second);
            }
        }
        for (auto &kv : integers)
        {
            if (kv.second)
            {
                serializer.setInt(kv.first.data(), *kv.second);
            }
        }
        for (auto &kv : strings)
        {
            if (kv.second)
            {
                serializer.setString(kv.first.data(), *kv.second);
            }
        }
        if (child.value)
        {
            serializer.setObject("CHILD", child);
        }
        for (const double f : arrayFloats)
        {
            serializer.appendFloatToArray("ARRAY_FLOATS", f);
        }
        for (const int64_t i : arrayIntegers)
        {
            serializer.appendIntToArray("ARRAY_INTS", i);
        }
        for (const DummyChild &o : arrayObjects)
        {
            serializer.appendObjectToArray("ARRAY_OBJECTS", o);
        }
    }

    void load(const Deserializer &deserializer) override
    {
        for (auto &kv : booleans)
        {
            kv.second = deserializer.getBool(kv.first.data());
        }
        for (auto &kv : floats)
        {
            kv.second = deserializer.getFloat(kv.first.data());
        }
        for (auto &kv : integers)
        {
            kv.second = deserializer.getInt(kv.first.data());
        }
        for (auto &kv : strings)
        {
            kv.second = deserializer.getString(kv.first.data());
        }
        deserializer.getObject("CHILD", child);
        for (size_t i = 0; i < deserializer.getArraySize("ARRAY_FLOATS"); ++i)
        {
            arrayFloats.push_back(*deserializer.getFloatFromArrayAt("ARRAY_FLOATS", i));
        }
        for (size_t i = 0; i < deserializer.getArraySize("ARRAY_INTS"); ++i)
        {
            arrayIntegers.push_back(*deserializer.getIntFromArrayAt("ARRAY_INTS", i));
        }
        for (size_t i = 0; i < deserializer.getArraySize("ARRAY_OBJECTS"); ++i)
        {
            arrayObjects.emplace_back();
            deserializer.getObjectFromArrayAt("ARRAY_OBJECTS", i, arrayObjects.back());
        }
    }

    void clearValues()
    {
        for (auto &kv : booleans)
        {
            kv.second.reset();
        }
        for (auto &kv : floats)
        {
            kv.second.reset();
        }
        for (auto &kv : integers)
        {
            kv.second.reset();
        }
        for (auto &kv : strings)
        {
            kv.second.reset();
        }
        child = {};
        arrayFloats.clear();
        arrayIntegers.clear();
        arrayObjects.clear();
    }

    std::vector<std::pair<std::string, std::optional<bool>>> booleans;
    std::vector<std::pair<std::string, std::optional<int>>> integers;
    std::vector<std::pair<std::string, std::optional<double>>> floats;
    std::vector<std::pair<std::string, std::optional<std::string>>> strings;
    DummyChild child;
    std::vector<double> arrayFloats;
    std::vector<int64_t> arrayIntegers;
    std::vector<DummyChild> arrayObjects;
};

class TestSerializerRapidJSON : public ::testing::Test
{
protected:
    void test(const char *result)
    {
        rapidjson::Document doc{rapidjson::Type::kObjectType};
        SerializerRapidJSON serial{doc, doc.GetAllocator()};
        ser.save(serial);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        doc.Accept(writer);
        const std::string serialized{buffer.GetString(), buffer.GetSize()};

        EXPECT_EQ(result, serialized);

        DummySerializable ser2 = ser;
        ser2.clearValues();
        ser2.load(DeserializerRapidJSON{doc});

        EXPECT_EQ(ser.booleans, ser2.booleans);
        EXPECT_EQ(ser.integers, ser2.integers);
        EXPECT_EQ(ser.strings, ser2.strings);
    }

    DummySerializable ser;
};

TEST_F(TestSerializerRapidJSON, booleans)
{
    ser.booleans = {
        {"true", true},
        {"false", false},
        {"true2", true},
    };
    static constexpr char result[] = R"({"true":true,"false":false,"true2":true})";
    test(result);
}

TEST_F(TestSerializerRapidJSON, integers)
{
    ser.integers = {
        {"one", 1},
        {"two", 2},
        {"three", 3},
    };
    static constexpr char result[] = R"({"one":1,"two":2,"three":3})";
    test(result);
}

TEST_F(TestSerializerRapidJSON, floats)
{
    ser.floats = {
        {"one", .1},
        {"two", .2},
        {"three", .3},
    };
    static constexpr char result[] = R"({"one":0.1,"two":0.2,"three":0.3})";
    test(result);
}

TEST_F(TestSerializerRapidJSON, strings)
{
    ser.strings = {
        {"one", "1"},
        {"two", "2"},
        {"three", "3"},
    };
    static constexpr char result[] = R"({"one":"1","two":"2","three":"3"})";
    test(result);
}

TEST_F(TestSerializerRapidJSON, object)
{
    ser.child.value = 1;
    static constexpr char result[] = R"({"CHILD":{"value":1}})";
    test(result);
}

TEST_F(TestSerializerRapidJSON, arrayFloats)
{
    ser.arrayFloats = {.1, .2, .3};
    static constexpr char result[] = R"({"ARRAY_FLOATS":[0.1,0.2,0.3]})";
    test(result);
}

TEST_F(TestSerializerRapidJSON, arrayIntegers)
{
    ser.arrayIntegers = {1, 2, 3};
    static constexpr char result[] = R"({"ARRAY_INTS":[1,2,3]})";
    test(result);
}

TEST_F(TestSerializerRapidJSON, arrayObjects)
{
    ser.arrayObjects = std::vector<DummyChild>(3);
    ser.arrayObjects[0].value = 1;
    ser.arrayObjects[1].value = 2;
    ser.arrayObjects[2].value = 3;
    static constexpr char result[] = R"({"ARRAY_OBJECTS":[{"value":1},{"value":2},{"value":3}]})";
    test(result);
}

TEST_F(TestSerializerRapidJSON, Serializer_AddSeveralTimes)
{
    ser.integers = {{"ARRAY_INTS", 1}};
    ser.booleans = {{"ARRAY_INTS", true}};
    ser.arrayIntegers = {1, 2, 3};

    rapidjson::Document doc{rapidjson::Type::kObjectType};
    SerializerRapidJSON serial{doc, doc.GetAllocator()};
    // see DummySerializable::save() the last one is the array
    ser.save(serial);

    // best effort, only the last is kept
    EXPECT_TRUE(doc["ARRAY_INTS"].IsArray());
}

TEST_F(TestSerializerRapidJSON, Deserializer_missingKeys)
{
    // empty document: all keys are missing
    const rapidjson::Document doc{rapidjson::Type::kObjectType};
    const DeserializerRapidJSON deserializer{doc};

    EXPECT_FALSE(deserializer.getBool("MISSING"));
    EXPECT_FALSE(deserializer.getFloat("MISSING"));
    EXPECT_FALSE(deserializer.getInt("MISSING"));
    EXPECT_FALSE(deserializer.getString("MISSING"));
    EXPECT_FALSE(deserializer.getObject("MISSING", ser));

    EXPECT_EQ(0, deserializer.getArraySize("MISSING"));
    EXPECT_FALSE(deserializer.getFloatFromArrayAt("MISSING", 0));
    EXPECT_FALSE(deserializer.getIntFromArrayAt("MISSING", 0));
    EXPECT_FALSE(deserializer.getObjectFromArrayAt("MISSING", 0, ser));
}

TEST_F(TestSerializerRapidJSON, Deserializer_wrongType)
{
    // empty document: all keys are missing
    rapidjson::Document doc{rapidjson::Type::kObjectType};
    doc.AddMember(rapidjson::StringRef("bool"), rapidjson::Value{true}, doc.GetAllocator());
    doc.AddMember(rapidjson::StringRef("int"), rapidjson::Value{42}, doc.GetAllocator());

    const DeserializerRapidJSON deserializer{doc};

    EXPECT_FALSE(deserializer.getBool("int"));
    EXPECT_FALSE(deserializer.getFloat("bool"));
    EXPECT_FALSE(deserializer.getInt("bool"));
    EXPECT_FALSE(deserializer.getString("bool"));
    EXPECT_FALSE(deserializer.getObject("bool", ser));

    EXPECT_EQ(0, deserializer.getArraySize("int"));
    EXPECT_FALSE(deserializer.getFloatFromArrayAt("int", 0));
    EXPECT_FALSE(deserializer.getIntFromArrayAt("int", 0));
    EXPECT_FALSE(deserializer.getObjectFromArrayAt("int", 0, ser));
}

TEST_F(TestSerializerRapidJSON, Deserializer_missingArrayIndex)
{
    // empty document: all keys are missing
    rapidjson::Document doc{rapidjson::Type::kObjectType};
    doc.AddMember(rapidjson::StringRef("array"), rapidjson::Type::kArrayType, doc.GetAllocator());

    const DeserializerRapidJSON deserializer{doc};

    EXPECT_FALSE(deserializer.getFloatFromArrayAt("array", 100));
    EXPECT_FALSE(deserializer.getIntFromArrayAt("array", 100));
    EXPECT_FALSE(deserializer.getObjectFromArrayAt("array", 100, ser));
}

TEST_F(TestSerializerRapidJSON, fileSerialization_Basic)
{
    ser.integers = {
        {"one", 1},
        {"two", 2},
        {"three", 3},
    };
    FileSerializationHandlerRapidJSON fileSerial{"dummy.json"};
    fileSerial.save(ser);

    DummySerializable ser2;
    ser2.integers = {
        {"one", {}},
        {"two", {}},
        {"three", {}},
    };
    const bool result = fileSerial.load(ser2);

    EXPECT_TRUE(result);
    EXPECT_EQ(ser.integers, ser2.integers);

    unlink("dummy.json");
}

TEST_F(TestSerializerRapidJSON, fileSerialization_MissingFile)
{
    unlink("dummy.json");
    FileSerializationHandlerRapidJSON fileSerial{"dummy.json"};
    const bool result = fileSerial.load(ser);

    EXPECT_FALSE(result);
}

TEST_F(TestSerializerRapidJSON, fileSerialization_NotAJson)
{
    std::ofstream{"dummy.json"} << "Hello, I am not a JSON file!";

    FileSerializationHandlerRapidJSON fileSerial{"dummy.json"};
    const bool result = fileSerial.load(ser);

    EXPECT_FALSE(result);

    unlink("dummy.json");
}
