#pragma once

#include <string>

/**
 * Type of shader
 */
enum class GlShaderType
{
    Vertex,   ///< Vertex shader
    Fragment, ///< Fragment shader
};

std::ostream &operator<<(std::ostream &str, GlShaderType type);

/**
 * @brief Base class to store shader
 *
 * The shader remains valid till the destruction of the object.
 * No copy is allowed as we are keeping a reference to the shader on the OpenGL side
 */
class GlShader
{
    explicit GlShader(const GlShader &) = delete;
    GlShader &operator=(const GlShader &) = delete;

public:
    /**
     * Get the OpenGL identifier for the shader
     */
    unsigned int get();

protected:
    explicit GlShader();
    GlShader(const char *source, GlShaderType type);
    ~GlShader();

    /**
     * @brief Destroy the shader on OpenGL side in the destructor
     */
    struct Guard
    {
        ~Guard();
        unsigned int shader = 0;
    };

    Guard guard;
};

/**
 * @brief Store a Vertex shader
 */
class GlVertexShader : public GlShader
{
public:
    explicit GlVertexShader(const char *source);
    GlVertexShader(GlVertexShader &&other);
    GlVertexShader &operator=(GlVertexShader &&other);
};

/**
 * @brief Store a Fragment shader
 */
class GlFragmentShader : public GlShader
{
public:
    explicit GlFragmentShader(const char *source);
    GlFragmentShader(GlFragmentShader &&other);
    GlFragmentShader &operator=(GlFragmentShader &&other);
};

/**
 * @brief Store a shader Program (vertex + fragment shaders linked)
 *
 * As it is a link to OpenGL resource, there is no copy, only move
 */
class GlProgram
{
    explicit GlProgram(const GlProgram &) = delete;
    GlProgram &operator=(const GlProgram &) = delete;

public:
    explicit GlProgram(const char *vertexSource,
                       const char *fragmentSource);
    explicit GlProgram(const std::string &vertexSource,
                       const std::string &fragmentSource)
        : GlProgram{vertexSource.c_str(), fragmentSource.c_str()}
    {
    }

    explicit GlProgram(GlProgram &&other);
    GlProgram &operator=(GlProgram &&other);

    ~GlProgram();

    /**
     * Use the GL program
     */
    void use();

    /**
     * Get the OpenGL identifier for the program
     */
    unsigned int get();

    /**
     * Get the OpenGL location for the attrib given during link time
     * Needed due to OpenGLES 2
     */
    int getAttribLocation(const char *name);

    /**
     * Get the OpenGL location for the uniform given during link time
     * Needed due to OpenGLES 2
     */
    int getUniformLocation(const char *name);
    friend std::ostream &operator<<(std::ostream &str, const GlProgram &program)
    {
        return program.toStream(str);
    }

private:
    std::ostream &toStream(std::ostream &str) const;

    /**
     * Destroy the program on OpenGL side in the destructor
     */
    struct Guard
    {
        ~Guard();
        unsigned int program = 0;
    };

    Guard guard;
    GlVertexShader vertexShader;
    GlFragmentShader fragmentShader;
};
