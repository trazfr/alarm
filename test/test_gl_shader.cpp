// correct testing of OpenGL is very hardware dependent... this is not really a unittest

#include <gtest/gtest.h>

#include "error.hpp"
#include "gl_shader.hpp"
#include "toolbox_gl.hpp"
#include "window.hpp"
#include "window_factory.hpp"

namespace
{
constexpr char kValidFragment[] = R"(\
#version 100

precision mediump float;

uniform vec4 u_color;
void main()
{
    gl_FragColor = u_color;
})";

constexpr char kValidVertex[] = R"(\
#version 100

attribute vec4 a_position;
void main()
{
    gl_Position = a_position;
})";

constexpr char kInvalidShader[] = "#pragma once";

} // namespace

class TestGlShader : public ::testing::Test
{
public:
    WindowFactory factory;

    void SetUp() override
    {
        factory.create(factory.getDriver(0), 320, 240).begin();
    }

    void TearDown() override
    {
        factory.get().end();
        factory.clear();
    }
};

TEST_F(TestGlShader, BasicVertex)
{
    GlVertexShader shader{kValidVertex};
    (void)shader;
}

TEST_F(TestGlShader, BasicFragment)
{
    GlFragmentShader shader{kValidFragment};
    (void)shader;
}

TEST_F(TestGlShader, BasicProgram)
{
    const std::string vertex = kValidVertex;
    const std::string fragment = kValidFragment;
    GlProgram program{vertex, fragment};

    EXPECT_NE(-1, program.get());
    EXPECT_NE(-1, program.getUniformLocation("u_color"));
    EXPECT_NE(-1, program.getAttribLocation("a_position"));

    std::ostringstream str;
    str << program;
    EXPECT_FALSE(str.str().empty());
}

TEST_F(TestGlShader, InvalidVertex)
{
    try
    {
        GlVertexShader shader{kInvalidShader};
        FAIL() << "Should have thrown";
    }
    catch (const Error &)
    {
    }
}

TEST_F(TestGlShader, InvalidFragment)
{
    try
    {
        GlFragmentShader shader{kInvalidShader};
        FAIL() << "Should have thrown";
    }
    catch (const Error &)
    {
    }
}

TEST_F(TestGlShader, NoLinkProgram)
{
    static constexpr char fragment[] = R"(\
#version 100

precision mediump float;

varying vec2 v_some_offset;
uniform vec4 u_color;
void main()
{
    gl_FragColor = vec4(u_color[0] + v_some_offset[0], u_color[1] + v_some_offset[1], u_color[2], u_color[3]);
})";
    // v_some_offset not present in vertex shader
    try
    {
        GlProgram program{kValidVertex, fragment};
        FAIL() << "Should have thrown";
    }
    catch (const Error &)
    {
    }
}
