#include "toolbox_gl.hpp"

#include <ostream>

namespace
{
template <typename T>
const T *getValidString(const T *c)
{
    if (c)
    {
        return c;
    }
    return reinterpret_cast<const T *>("<INVALID>");
}
} // namespace

void glDebug(std::ostream &str)
{
    str << "OpenGL info:" << std::endl;
    str << " - GL_VENDOR: " << getValidString(glGetString(GL_VENDOR)) << std::endl;
    str << " - GL_RENDERER: " << getValidString(glGetString(GL_RENDERER)) << std::endl;
    str << " - GL_VERSION: " << getValidString(glGetString(GL_VERSION)) << std::endl;
    str << " - GL_SHADING_LANGUAGE_VERSION: " << getValidString(glGetString(GL_SHADING_LANGUAGE_VERSION)) << std::endl;
    str << " - GL_EXTENSIONS: " << getValidString(glGetString(GL_EXTENSIONS)) << std::endl;
}

void glCheckError(const char *func,
                  const char *file,
                  const int line)
{
    if (const GLenum error = glGetError())
    {
        throw GLError("Got error", error, func, file, line);
    }
}

GLError::GLError(const char *description,
                 GLenum error,
                 const char *func,
                 const char *file,
                 int line)
    : Error{description + std::string{". GL Error: "} + std::to_string(error), func, file, line}
{
}
