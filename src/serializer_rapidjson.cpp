#include "serializer_rapidjson.hpp"

#include "serializable.hpp"
#include "toolbox_io.hpp"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/prettywriter.h>

namespace
{

constexpr size_t kBufferSize = 65536;

rapidjson::Value &getOrAddArray(rapidjson::Value &object, const char *key,
                                rapidjson::Value::AllocatorType &allocator)
{
    const auto keyRef = rapidjson::StringRef(key);
    if (const auto it = object.FindMember(keyRef); it != object.MemberEnd())
    {
        if (it->value.IsArray())
        {
            return it->value;
        }
        // not an array... best effort
        object.RemoveMember(it);
    }
    object.AddMember(keyRef, rapidjson::Type::kArrayType, allocator);
    return getOrAddArray(object, key, allocator);
}

const rapidjson::Value *getValueFromArrayAt(const rapidjson::Value &object, const char *key, size_t idx)
{
    const auto it = object.FindMember(key);
    if (it == object.MemberEnd() || !it->value.IsArray())
    {
        return nullptr;
    }
    const auto &array = it->value;
    if (idx <= array.Size())
    {
        return &array[idx];
    }
    return nullptr;
}

} // namespace

SerializerRapidJSON::SerializerRapidJSON(rapidjson::Value &object, rapidjson::Value::AllocatorType &allocator)
    : object{object},
      allocator{allocator}
{
}

SerializerRapidJSON::~SerializerRapidJSON() = default;

void SerializerRapidJSON::setBool(const char *key, bool value)
{
    object.AddMember(rapidjson::StringRef(key), rapidjson::Value{value}, allocator);
}

void SerializerRapidJSON::setFloat(const char *key, double value)
{
    object.AddMember(rapidjson::StringRef(key), rapidjson::Value{value}, allocator);
}

void SerializerRapidJSON::setInt(const char *key, int64_t value)
{
    object.AddMember(rapidjson::StringRef(key), rapidjson::Value{value}, allocator);
}

void SerializerRapidJSON::setString(const char *key, std::string_view value)
{
    object.AddMember(rapidjson::StringRef(key),
                     rapidjson::Value{value.data(), static_cast<rapidjson::SizeType>(value.size())},
                     allocator);
}

void SerializerRapidJSON::setObject(const char *key, const Serializable &serializable)
{
    rapidjson::Value newObject{rapidjson::Type::kObjectType};
    SerializerRapidJSON child{newObject, allocator};
    serializable.save(child);

    object.AddMember(rapidjson::StringRef(key), newObject, allocator);
}

void SerializerRapidJSON::appendFloatToArray(const char *key, double value)
{
    getOrAddArray(object, key, allocator).PushBack(rapidjson::Value{value}, allocator);
}

void SerializerRapidJSON::appendIntToArray(const char *key, int64_t value)
{
    getOrAddArray(object, key, allocator).PushBack(rapidjson::Value{value}, allocator);
}

void SerializerRapidJSON::appendObjectToArray(const char *key, const Serializable &serializable)
{
    rapidjson::Value newObject{rapidjson::Type::kObjectType};
    SerializerRapidJSON child{newObject, allocator};
    serializable.save(child);

    getOrAddArray(object, key, allocator).PushBack(newObject, allocator);
}

DeserializerRapidJSON::DeserializerRapidJSON(const rapidjson::Value &object)
    : object(object)
{
}

DeserializerRapidJSON::~DeserializerRapidJSON() = default;

std::optional<bool> DeserializerRapidJSON::getBool(const char *key) const
{
    if (const auto it = object.FindMember(key); it != object.MemberEnd() && it->value.IsBool())
    {
        return {{it->value.GetBool()}};
    }
    return {};
}

std::optional<double> DeserializerRapidJSON::getFloat(const char *key) const
{
    if (const auto it = object.FindMember(key); it != object.MemberEnd() && it->value.IsDouble())
    {
        return {{it->value.GetDouble()}};
    }
    return {};
}

std::optional<int64_t> DeserializerRapidJSON::getInt(const char *key) const
{
    if (const auto it = object.FindMember(key); it != object.MemberEnd() && it->value.IsInt64())
    {
        return {{it->value.GetInt64()}};
    }
    return {};
}

std::optional<std::string_view> DeserializerRapidJSON::getString(const char *key) const
{
    if (const auto it = object.FindMember(key); it != object.MemberEnd() && it->value.IsString())
    {
        return {{it->value.GetString(), it->value.GetStringLength()}};
    }
    return {};
}

bool DeserializerRapidJSON::getObject(const char *key, Serializable &serializable) const
{
    if (const auto it = object.FindMember(key); it != object.MemberEnd() && it->value.IsObject())
    {
        const DeserializerRapidJSON child{it->value};
        serializable.load(child);
        return true;
    }
    return false;
}

size_t DeserializerRapidJSON::getArraySize(const char *key) const
{
    if (const auto it = object.FindMember(key); it != object.MemberEnd() && it->value.IsArray())
    {
        return it->value.Size();
    }
    return 0;
}

std::optional<double> DeserializerRapidJSON::getFloatFromArrayAt(const char *key, size_t idx) const
{
    if (const auto *value = getValueFromArrayAt(object, key, idx); value != nullptr && value->IsDouble())
    {
        return {value->GetDouble()};
    }
    return {};
}

std::optional<int64_t> DeserializerRapidJSON::getIntFromArrayAt(const char *key, size_t idx) const
{
    if (const auto *value = getValueFromArrayAt(object, key, idx); value != nullptr && value->IsInt64())
    {
        return {value->GetInt64()};
    }
    return {};
}

bool DeserializerRapidJSON::getObjectFromArrayAt(const char *key, size_t idx, Serializable &serializable) const
{
    if (const auto *value = getValueFromArrayAt(object, key, idx); value != nullptr && value->IsObject())
    {
        const DeserializerRapidJSON child{*value};
        serializable.load(child);
        return true;
    }
    return false;
}

FileSerializationHandlerRapidJSON::FileSerializationHandlerRapidJSON(const char *filename)
    : filename{filename}
{
}

FileSerializationHandlerRapidJSON::~FileSerializationHandlerRapidJSON() = default;

bool FileSerializationHandlerRapidJSON::load(Serializable &serializable)
{
    const FILEUnique fd{std::fopen(filename, "rb")};
    if (!fd)
    {
        return false;
    }

    char buffer[kBufferSize];
    rapidjson::FileReadStream stream{fd.get(), buffer, sizeof(buffer)};
    rapidjson::Document doc;
    doc.ParseStream(stream);

    if (doc.HasParseError())
    {
        return false;
    }

    serializable.load(DeserializerRapidJSON{doc});
    return true;
}

void FileSerializationHandlerRapidJSON::save(const Serializable &serializable)
{
    std::string tmpFile = filename;
    tmpFile += ".tmp";

    {
        const FILEUnique fd{std::fopen(tmpFile.c_str(), "wb+")};

        rapidjson::Document doc{rapidjson::Type::kObjectType};
        SerializerRapidJSON serial{doc, doc.GetAllocator()};
        serializable.save(serial);

        char buffer[kBufferSize];
        rapidjson::FileWriteStream stream{fd.get(), buffer, sizeof(buffer)};

        rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer{stream};
        doc.Accept(writer);
    }

    std::rename(tmpFile.c_str(), filename);
}
