#include "gl_shader.hpp"

#include "error.hpp"
#include "toolbox_gl.hpp"

#include <sstream>

namespace
{

const char *toString(GlShaderType type)
{
    switch (type)
    {
    case GlShaderType::Vertex:
        return "Vertex";
    case GlShaderType::Fragment:
        return "Fragment";
    default:
        return "Unknown";
    }
}

class GlCompileShaderError : public Error
{
public:
    explicit GlCompileShaderError(const char *description,
                                  const char *source,
                                  GlShaderType type,
                                  GLuint shader,
                                  const char *func = __builtin_FUNCTION(),
                                  const char *file = __builtin_FILE(),
                                  int line = __builtin_LINE())
        : Error{getErrorDescription(description, source, type, shader), func, file, line}
    {
    }

    static std::string getErrorDescription(const char *description,
                                           const char *source,
                                           GlShaderType type,
                                           GLuint shader)
    {
        std::ostringstream str;
        str << description
            << ". GL "
            << type
            << " Shader Error: ";

        GLint size = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &size);
        if (size > 1)
        {
            const auto infoLog = reinterpret_cast<GLchar *>(alloca(size * sizeof(GLchar)));
            glGetShaderInfoLog(shader, size, nullptr, infoLog);
            str << infoLog;
        }
        else
        {
            str << "no infoLog";
        }

        str << "Source:\n";
        str << source;

        return str.str();
    }
};

class GlProgramLinkError : public Error
{
public:
    explicit GlProgramLinkError(const char *description,
                                GLuint program,
                                const char *func = __builtin_FUNCTION(),
                                const char *file = __builtin_FILE(),
                                int line = __builtin_LINE())
        : Error{getErrorDescription(description, program), func, file, line}
    {
    }

    static std::string getErrorDescription(const char *description,
                                           GLuint program)
    {
        std::string result = description;
        result += ". Program link Error: ";

        GLint size = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &size);
        if (size > 1)
        {
            const auto infoLog = reinterpret_cast<GLchar *>(alloca(size * sizeof(GLchar)));
            glGetProgramInfoLog(program, size, nullptr, infoLog);
            result += infoLog;
        }
        else
        {
            result += "no infoLog";
        }

        return result;
    }
};

constexpr GLenum getShaderType(GlShaderType type)
{
    switch (type)
    {
    case GlShaderType::Vertex:
        return GL_VERTEX_SHADER;
    case GlShaderType::Fragment:
        return GL_FRAGMENT_SHADER;
    default:
        return 0;
    }
}

} // namespace

std::ostream &operator<<(std::ostream &str, GlShaderType type)
{
    return str << toString(type);
}

// GlShader

GlShader::Guard::~Guard()
{
    if (shader)
    {
        glDeleteShader(shader);
    }
}

GlShader::GlShader() = default;

GlShader::GlShader(const char *source, GlShaderType type)
    : guard{glCreateShader(getShaderType(type))}
{
    static_assert(std::is_same_v<decltype(glCreateShader(0)), decltype(guard.shader)>);

    if (get() == 0)
    {
        throw GLError{("Cannot create shader " + std::string(toString(type))).c_str()};
    }

    glShaderSource(get(), 1, &source, nullptr);
    glCompileShader(get());

    GLint compiled;
    glGetShaderiv(get(), GL_COMPILE_STATUS, &compiled);

    if (!compiled)
    {
        throw GlCompileShaderError{"Could not compile the shader", source, type, get()};
    }
}

GlShader::~GlShader() = default;

unsigned int GlShader::get()
{
    static_assert(std::is_same_v<decltype(guard.shader), decltype(get())>);
    return guard.shader;
}

// GlVertexShader

GlVertexShader::GlVertexShader(const char *source)
    : GlShader{source, GlShaderType::Vertex}
{
}

GlVertexShader::GlVertexShader(GlVertexShader &&other)
{
    std::swap(guard, other.guard);
}

GlVertexShader &GlVertexShader::operator=(GlVertexShader &&other)
{
    std::swap(guard, other.guard);
    return *this;
}

// GlFragmentShader

GlFragmentShader::GlFragmentShader(const char *source)
    : GlShader{source, GlShaderType::Fragment}
{
}

GlFragmentShader::GlFragmentShader(GlFragmentShader &&other)
{
    std::swap(guard, other.guard);
}

GlFragmentShader &GlFragmentShader::operator=(GlFragmentShader &&other)
{
    std::swap(guard, other.guard);
    return *this;
}

// GlProgram

GlProgram::Guard::~Guard()
{
    if (program)
    {
        glDeleteProgram(program);
    }
}

GlProgram::GlProgram(const char *vertexSource,
                     const char *fragmentSource)
    : guard{glCreateProgram()},
      vertexShader{vertexSource},
      fragmentShader{fragmentSource}
{
    static_assert(std::is_same_v<decltype(glCreateProgram()), decltype(guard.program)>);

    if (get() == 0)
    {
        throw GLError("Cannot create GL program");
    }
    glAttachShader(get(), vertexShader.get());
    glAttachShader(get(), fragmentShader.get());

    glLinkProgram(get());
    GLint linked;
    glGetProgramiv(get(), GL_LINK_STATUS, &linked);
    if (!linked)
    {
        throw GlProgramLinkError{"Could not link the program", get()};
    }
}

GlProgram::GlProgram(GlProgram &&other)
    : vertexShader{std::move(other.vertexShader)},
      fragmentShader{std::move(fragmentShader)}
{
    std::swap(guard, other.guard);
}

GlProgram &GlProgram::operator=(GlProgram &&other)
{
    vertexShader = std::move(other.vertexShader);
    fragmentShader = std::move(fragmentShader);
    std::swap(guard, other.guard);
    return *this;
}

GlProgram::~GlProgram() = default;

void GlProgram::use()
{
    glUseProgram(get());
}

unsigned int GlProgram::get()
{
    static_assert(std::is_same_v<decltype(guard.program), decltype(get())>);
    return guard.program;
}

int GlProgram::getAttribLocation(const char *name)
{
    static_assert(std::is_same_v<decltype(glGetAttribLocation(0, nullptr)), decltype(getAttribLocation(nullptr))>);
    const GLint result = glGetAttribLocation(get(), name);
    if (result < 0)
    {
        throw GLError{("Could not glGetAttribLocation(" + std::string(name) + ')').c_str()};
    }
    return result;
}

int GlProgram::getUniformLocation(const char *name)
{
    static_assert(std::is_same_v<decltype(glGetUniformLocation(0, nullptr)), decltype(getUniformLocation(nullptr))>);
    const GLint result = glGetUniformLocation(get(), name);
    if (result < 0)
    {
        throw GLError{("Could not glGetUniformLocation(" + std::string(name) + ')').c_str()};
    }
    return result;
}

std::ostream &GlProgram::toStream(std::ostream &str) const
{
    char buf[256];
    str << "GL Program\n";
    GLint param = 0;
    GLsizei bufLen = 0;
    GLint size = 0;
    GLenum type = 0;

    glGetProgramiv(guard.program, GL_ACTIVE_ATTRIBUTES, &param);
    str << "  Attributes:\n";
    for (GLint i = 0; i < param; ++i)
    {
        glGetActiveAttrib(guard.program, i, sizeof(buf), &bufLen, &size, &type, buf);
        str << "    - " << std::string_view{buf, static_cast<size_t>(bufLen)} << ' ' << size << '\n';
    }

    glGetProgramiv(guard.program, GL_ACTIVE_UNIFORMS, &param);
    str << "  Uniforms:\n";
    for (GLint i = 0; i < param; ++i)
    {
        glGetActiveUniform(guard.program, i, sizeof(buf), &bufLen, nullptr, &type, buf);
        str << "    - " << std::string_view{buf, static_cast<size_t>(bufLen)} << ' ' << size << '\n';
    }
    return str;
}
