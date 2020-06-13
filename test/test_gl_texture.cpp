// correct testing of OpenGL is very hardware dependent... this is not really a unittest

#include <gtest/gtest.h>

#include "gl_shader.hpp"
#include "gl_texture.hpp"
#include "toolbox_gl.hpp"
#include "window.hpp"
#include "window_factory.hpp"

namespace
{

constexpr char kFilename[] = "test.dds";

constexpr char vertexShader[] = R"(\
#version 100

attribute vec4 a_position;
attribute vec2 a_texCoord;
varying vec2 v_texCoord;

void main()
{
    gl_Position = a_position;
    v_texCoord = a_texCoord;
})";

constexpr char fragmentShader[] = R"(\
#version 100

precision mediump float;
varying vec2 v_texCoord;
uniform sampler2D s_texture;

void main()
{
    gl_FragColor = texture2D(s_texture, v_texCoord);
})";

constexpr int width = 320;
constexpr int height = 240;
constexpr GLfloat vertices[] = {
    -0.5f, 0.5f, 0.0f,  // Position 0
    0.0f, 0.0f,         // TexCoord 0
    -0.5f, -0.5f, 0.0f, // Position 1
    0.0f, 1.0f,         // TexCoord 1
    0.5f, -0.5f, 0.0f,  // Position 2
    1.0f, 1.0f,         // TexCoord 2
    0.5f, 0.5f, 0.0f,   // Position 3
    1.0f, 0.0f          // TexCoord 3
};
constexpr GLushort indices[] = {0, 1, 2, 0, 2, 3};

} // namespace

class TestGlTexture : public ::testing::Test
{
protected:
    WindowFactory factory;

    void SetUp() override
    {
        factory.create(factory.getDriver(0), width, height).begin();
        glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
        program = std::make_unique<GlProgram>(vertexShader, fragmentShader);

        positionLoc = program->getAttribLocation("a_position");
        texCoordLoc = program->getAttribLocation("a_texCoord");
        samplerLoc = program->getUniformLocation("s_texture");

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, width, height);

        // Clear the color buffer
        glClear(GL_COLOR_BUFFER_BIT);

        // Use the program object
        glUseProgram(program->get());

        // Load the vertex position
        glVertexAttribPointer(positionLoc, 3, GL_FLOAT,
                              GL_FALSE, 5 * sizeof(*vertices), vertices);
        // Load the texture coordinate
        glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT,
                              GL_FALSE, 5 * sizeof(*vertices), vertices + 3);

        glEnableVertexAttribArray(positionLoc);
        glEnableVertexAttribArray(texCoordLoc);

        glActiveTexture(GL_TEXTURE0);
    }

    void TearDown() override
    {
        ASSERT_TRUE(texture);
        glBindTexture(GL_TEXTURE_2D, texture->get());

        glUniform1i(samplerLoc, 0);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
        factory.get().end();
        factory.clear();

        unlink(kFilename);
    }

    std::unique_ptr<GlProgram> program;
    std::unique_ptr<GlTexture> texture;
    GLint positionLoc;
    GLint texCoordLoc;
    GLint samplerLoc;
};

TEST_F(TestGlTexture, RGB)
{
    system("convert -size 128x128 -define gradient:angle=45 gradient:red-blue -format dds -define dds:compression=none -define dds:mipmaps=7 test.dds");
    texture = std::make_unique<GlTexture>(kFilename);
}

TEST_F(TestGlTexture, BGRA)
{
    system("convert -size 128x128 -background none gradient:none-red -flatten -format dds -define dds:compression=none -define dds:mipmaps=7 test.dds");
    texture = std::make_unique<GlTexture>(kFilename);
}

// not supported by Raspberry Pi
TEST_F(TestGlTexture, DISABLED_DXT1)
{
    system("convert -size 128x128 -define gradient:angle=45 gradient:red-blue -format dds -define dds:compression=dxt1 -define dds:mipmaps=7 test.dds");
    texture = std::make_unique<GlTexture>(kFilename);
}

// not supported by Raspberry Pi
TEST_F(TestGlTexture, DISABLED_DXT3)
{
    system("convert -size 128x128 -background none gradient:none-red -flatten -format dds -define dds:compression=dxt3 -define dds:mipmaps=7 test.dds");
    texture = std::make_unique<GlTexture>(kFilename);
}

// not supported by Raspberry Pi
TEST_F(TestGlTexture, DISABLED_DXT5)
{
    system("convert -size 128x128 -background none gradient:none-red -flatten -format dds -define dds:compression=dxt5 -define dds:mipmaps=7 test.dds");
    texture = std::make_unique<GlTexture>(kFilename);
}
