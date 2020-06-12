#pragma once

#include "serializer.hpp"

namespace rapidjson
{
class CrtAllocator;

template <typename T>
class UTF8;

template <typename T>
class MemoryPoolAllocator;

template <typename Encoding, typename Allocator>
class GenericValue;
} // namespace rapidjson

using RapidJSONAllocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;
using RapidJSONObject = rapidjson::GenericValue<rapidjson::UTF8<char>, RapidJSONAllocator>;

/**
 * @brief serialize Objects into JSON
 */
class SerializerRapidJSON : public Serializer
{
public:
    SerializerRapidJSON(RapidJSONObject &object, RapidJSONAllocator &allocator);
    ~SerializerRapidJSON() override;

    void setBool(const char *key, bool value) override;
    void setFloat(const char *key, double value) override;
    void setInt(const char *key, int64_t value) override;
    void setString(const char *key, std::string_view value) override;
    void setObject(const char *key, const Serializable &serializable) override;

    void appendFloatToArray(const char *key, double value) override;
    void appendIntToArray(const char *key, int64_t value) override;
    void appendObjectToArray(const char *key, const Serializable &serializable) override;

private:
    RapidJSONObject &object;
    RapidJSONAllocator &allocator;
};

/**
 * @brief deserialize JSON into Objects
 */
class DeserializerRapidJSON : public Deserializer
{
public:
    explicit DeserializerRapidJSON(const RapidJSONObject &object);
    ~DeserializerRapidJSON() override;

    std::optional<bool> getBool(const char *key) const override;
    std::optional<double> getFloat(const char *key) const override;
    std::optional<int64_t> getInt(const char *key) const override;
    std::optional<std::string_view> getString(const char *key) const override;
    bool getObject(const char *key, Serializable &serializable) const override;

    size_t getArraySize(const char *key) const override;

    std::optional<double> getFloatFromArrayAt(const char *key, size_t idx) const override;
    std::optional<int64_t> getIntFromArrayAt(const char *key, size_t idx) const override;
    bool getObjectFromArrayAt(const char *key, size_t idx, Serializable &serializable) const override;

private:
    const RapidJSONObject &object;
};

/**
 * @brief Handle JSON serialization / deserialization into a fiel
 */
class FileSerializationHandlerRapidJSON : public SerializationHandler
{
public:
    explicit FileSerializationHandlerRapidJSON(const char *filename);
    ~FileSerializationHandlerRapidJSON() override;

    bool load(Serializable &serializable) override;
    void save(const Serializable &serializable) override;

private:
    const char *const filename;
};
