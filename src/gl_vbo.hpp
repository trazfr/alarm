#pragma once

#include <cstddef>

/**
 * @brief Base class for OpenGL Vertex Buffer Object (VBO)
 */
class GlVbo
{
    explicit GlVbo(const GlVbo &) = delete;
    GlVbo &operator=(const GlVbo &) = delete;

public:
    /**
     * Get the OpenGL identifier for the VBO
     */
    unsigned int get() const;

protected:
    explicit GlVbo() = default;
    GlVbo(const void *data, size_t bufferSize, int glType, int glTarget, int glUsage);

    ~GlVbo();

    /**
     * Return the OpenGL type associated to T.
     *
     * @arg GLubyte: GL_UNSIGNED_BYTE
     * @arg GLushort: GL_UNSIGNED_SHORT
     * @arg GLfloat: GL_FLOAT
     */
    template <typename T>
    static int getType();

    void swap(GlVbo &other);

    /**
     * @brief Destroy the VBO on OpenGL side in the destructor
     */
    struct Guard
    {
        ~Guard();
        unsigned int vbo = 0;
        int glType = 0;
    };

    Guard guard;
};

/**
 * @brief Base class for GL_ARRAY_BUFFER VBO
 */
class GlVboArray : public GlVbo
{
public:
    explicit GlVboArray(GlVboArray &&other);
    GlVboArray &operator=(GlVboArray &&other);

    /**
     * Bind the array
     */
    void bind();

    /**
     * Draw the VBO.
     *
     * Most of the time the template variant is a better choice.
     *
     * @param offset in bytes
     * @param stride in bytes
     */
    void draw(int index, int size, int offset, int stride);

    /**
     * Draw the VBO.
     *
     * No offset, there is no "hole", all the data is contiguous in the VBO (stride = 0).
     */
    void draw(int index, int size)
    {
        draw(index, size, 0, 0);
    }

    /**
     * Draw the VBO
     *
     * This variant is used when the data type is the same in the VBO.
     * For instance when all the VBO is filled with GLfloat.
     *
     * The offset and stride is then computed thanks to the size of 1 element
     *
     * @param offset in number of elements
     * @param stride in number of elements
     */
    template <typename T>
    void draw(int index, int size, int offset, int stride)
    {
        return draw(index, size, offset * sizeof(T), stride * sizeof(T));
    }

protected:
    GlVboArray(const void *data, size_t bufferSize, int glType, int glUsage, bool normalized)
        : GlVbo{data, bufferSize, glType, getTarget(), glUsage},
          normalized{normalized}
    {
    }

    /**
     * GL_ARRAY_BUFFER
     */
    static int getTarget();

private:
    bool normalized;
};

/**
 * @brief GL_ARRAY_BUFFER which cannot be updated
 */
class GlVboArrayStatic : public GlVboArray
{
public:
    template <typename T, size_t Size>
    explicit GlVboArrayStatic(const T (&indices)[Size], bool normalized = false)
        : GlVboArrayStatic{&indices[0], Size, normalized} {}
    template <typename T>
    GlVboArrayStatic(const T *indices, size_t numberElements, bool normalized = false)
        : GlVboArray{&indices[0], numberElements * sizeof(T), getType<T>(), getUsage(), normalized}
    {
    }

private:
    /**
     * GL_STATIC_DRAW
     */
    static int getUsage();
};

/**
 * @brief GL_ARRAY_BUFFER which can be updated
 */
class GlVboArrayDynamic : public GlVboArray
{
public:
    template <typename T, size_t Size>
    explicit GlVboArrayDynamic(const T (&indices)[Size], bool normalized = false)
        : GlVboArrayDynamic{&indices[0], Size, normalized} {}
    template <typename T>
    GlVboArrayDynamic(const T *indices, size_t numberElements, bool normalized = false)
        : GlVboArray{&indices[0], numberElements * sizeof(T), getType<T>(), getUsage(), normalized}
    {
    }

    /**
     * Change the value in the VBO
     */
    template <typename T>
    void set(const T *indices, size_t numberElements)
    {
        set(reinterpret_cast<const void *>(indices), numberElements * sizeof(T));
    }

private:
    void set(const void *indices, size_t bufferSize);

    /**
     * GL_DYNAMIC_DRAW
     */
    static int getUsage();
};

/**
 * @brief GL_ELEMENT_ARRAY_BUFFER which cannot be updated
 */
class GlVboElementArray : public GlVbo
{
public:
    template <typename T, size_t Size>
    explicit GlVboElementArray(const T (&indices)[Size])
        : GlVboElementArray{&indices[0], Size} {}

    template <typename T>
    GlVboElementArray(const T *indices, size_t numberVertex)
        : GlVbo{&indices[0], numberVertex * sizeof(T), getType<T>(), getTarget(), getUsage()},
          numberVertex{static_cast<int>(numberVertex)} {}

    explicit GlVboElementArray(GlVboElementArray &&other);
    GlVboElementArray &operator=(GlVboElementArray &&other);

    /**
     * Bind + draw the GL_ELEMENT_ARRAY_BUFFER
     */
    void draw();

    /**
     * As we only support triangles, this returns the number of triangles
     */
    int triangles() const
    {
        return numberVertex / 3;
    }

private:
    /**
     * GL_ELEMENT_ARRAY_BUFFER
     */
    static int getTarget();

    /**
     * GL_STATIC_DRAW
     */
    static int getUsage();
    int numberVertex;
};
