#pragma once

class Deserializer;
class Serializer;

/**
 * @brief base class for objects which can be serializable / deserializable
 */
class Serializable
{
public:
    virtual ~Serializable() = 0;

    virtual void load(const Deserializer &deserializer) = 0;
    virtual void save(Serializer &serializer) const = 0;
};
