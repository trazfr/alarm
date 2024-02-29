#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

class Serializable;

/**
 * @brief base class for serialization
 */
class Serializer
{
public:
    virtual ~Serializer() = 0;

    virtual void setBool(const char *key, bool value) = 0;
    virtual void setFloat(const char *key, double value) = 0;
    virtual void setInt(const char *key, int64_t value) = 0;
    virtual void setString(const char *key, std::string_view value) = 0;
    virtual void setObject(const char *key, const Serializable &serializable) = 0;

    virtual void appendFloatToArray(const char *key, double value) = 0;
    virtual void appendIntToArray(const char *key, int64_t value) = 0;
    virtual void appendObjectToArray(const char *key, const Serializable &serializable) = 0;
};

/**
 * @brief base class for deserialization
 */
class Deserializer
{
public:
    virtual ~Deserializer() = 0;

    virtual std::optional<bool> getBool(const char *key) const = 0;
    virtual std::optional<double> getFloat(const char *key) const = 0;
    virtual std::optional<int64_t> getInt(const char *key) const = 0;
    virtual std::optional<std::string_view> getString(const char *key) const = 0;
    virtual bool getObject(const char *key, Serializable &serializable) const = 0;

    virtual size_t getArraySize(const char *key) const = 0;

    virtual std::optional<double> getFloatFromArrayAt(const char *key, size_t idx) const = 0;
    virtual std::optional<int64_t> getIntFromArrayAt(const char *key, size_t idx) const = 0;
    virtual bool getObjectFromArrayAt(const char *key, size_t idx, Serializable &serializable) const = 0;
};

/**
 * @brief base class to load / save serializable objects
 */
class SerializationHandler
{
public:
    virtual ~SerializationHandler() = 0;

    virtual bool load(Serializable &serializable) = 0;
    virtual void save(const Serializable &serializable) = 0;
};
