#include "gl_vbo.hpp"

#include "toolbox_gl.hpp"

GlVbo::Guard::~Guard()
{
    if (vbo)
    {
        glDeleteBuffers(1, &vbo);
    }
}

GlVbo::GlVbo(const void *data, size_t bufferSize, int glType, int glTarget, int glUsage)
{
    guard.glType = glType;

    glGenBuffers(1, &guard.vbo);
    glBindBuffer(glTarget, get());
    glBufferData(glTarget, bufferSize, data, glUsage);
}

void GlVbo::swap(GlVbo &other)
{
    std::swap(guard, other.guard);
}

GlVbo::~GlVbo() = default;

template <>
int GlVbo::getType<GLubyte>()
{
    return GL_UNSIGNED_BYTE;
}

template <>
int GlVbo::getType<GLushort>()
{
    return GL_UNSIGNED_SHORT;
}

template <>
int GlVbo::getType<GLfloat>()
{
    return GL_FLOAT;
}

unsigned int GlVbo::get() const
{
    return guard.vbo;
}

// GlVboArray

GlVboArray::GlVboArray(GlVboArray &&other)
{
    swap(other);
    std::swap(normalized, other.normalized);
}

GlVboArray &GlVboArray::operator=(GlVboArray &&other)
{
    swap(other);
    std::swap(normalized, other.normalized);
    return *this;
}

void GlVboArray::bind()
{
    glBindBuffer(getTarget(), get());
}

void GlVboArray::draw(int index, int size, int offset, int stride)
{
    glVertexAttribPointer(index, size, guard.glType, normalized, stride, reinterpret_cast<void *>(offset));
}

int GlVboArray::getTarget()
{
    return GL_ARRAY_BUFFER;
}

// GlVboArrayStatic

int GlVboArrayStatic::getUsage()
{
    return GL_STATIC_DRAW;
}

// GlVboArrayDynamic

void GlVboArrayDynamic::set(const void *indices, size_t bufferSize)
{
    glBufferSubData(getTarget(), 0, bufferSize, indices);
}

int GlVboArrayDynamic::getUsage()
{
    return GL_DYNAMIC_DRAW;
}

// GlVboElementArray

GlVboElementArray::GlVboElementArray(GlVboElementArray &&other)
{
    swap(other);
    std::swap(numberVertex, other.numberVertex);
}

GlVboElementArray &GlVboElementArray::operator=(GlVboElementArray &&other)
{
    swap(other);
    std::swap(numberVertex, other.numberVertex);
    return *this;
}

void GlVboElementArray::draw()
{
    glBindBuffer(getTarget(), get());
    glDrawElements(GL_TRIANGLES, numberVertex, guard.glType, nullptr);
}

int GlVboElementArray::getTarget()
{
    return GL_ELEMENT_ARRAY_BUFFER;
}

int GlVboElementArray::getUsage()
{
    return GL_STATIC_DRAW;
}
