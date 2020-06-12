#include "renderer.hpp"

#include "config.hpp"
#include "error.hpp"
#include "gl_shader.hpp"
#include "gl_texture.hpp"
#include "gl_texture_loader.hpp"
#include "gl_vbo.hpp"
#include "renderer_sprite.hpp"
#include "renderer_text.hpp"
#include "toolbox_filesystem.hpp"
#include "toolbox_gl.hpp"
#include "toolbox_io.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <ostream>
#include <vector>

namespace
{

/**
 * @brief Texture + position on screen
 */
struct GraphicalAsset
{
    GraphicalAsset(const std::string &filename, unsigned int width, unsigned int height, unsigned textureSize)
        : texture{filename.c_str()},
          screenWidth{static_cast<GLfloat>(width * (2. / Renderer::getWidth()))},
          screenHeight{static_cast<GLfloat>(height * (2. / Renderer::getHeight()))},
          cropWidth{static_cast<GLfloat>(1. * width / textureSize)},
          cropHeight{static_cast<GLfloat>(1. * height / textureSize)}
    {
    }
    GlTexture texture;
    GLfloat screenWidth = 0;
    GLfloat screenHeight = 0;
    GLfloat cropWidth = 0;
    GLfloat cropHeight = 0;
};

/**
 * Get the horizontal alignment from the Position enum
 * @return 0 for left, 1 for center, 2 for right
 */
constexpr int getHAlign(Position p)
{
    return static_cast<int>(p) % 3;
}

/**
 * Get the vertical alignment from the Position enum
 * @return 0 for down, 1 for center, 2 for up
 */
constexpr int getVAlign(Position p)
{
    return static_cast<int>(p) / 3;
}

/**
 * @brief Get the OpenGL vertices to display the asset
 */
constexpr std::array<GLfloat, 20> getVertices2D(const GraphicalAsset &asset, GLfloat x, GLfloat y, int rotation90Degree = 0)
{
    const GLfloat textCoord[] = {
        0, 0,                              // TexCoord 0
        0, asset.cropWidth,                // TexCoord 1
        asset.cropHeight, asset.cropWidth, // TexCoord 2
        asset.cropHeight, 0,               // TexCoord 3
        // rotation
        0, 0,                              // TexCoord=0 (rotation)
        0, asset.cropWidth,                // TexCoord=1 (rotation)
        asset.cropHeight, asset.cropWidth, // TexCoord=2 (rotation)
    };
    int texI = (rotation90Degree % 4) * 2;

    return {
        x, y + asset.screenHeight,                     // Position 0
        textCoord[texI++], textCoord[texI++],          // TexCoord 0
        x, y,                                          // Position 1
        textCoord[texI++], textCoord[texI++],          // TexCoord 1
        x + asset.screenWidth, y,                      // Position 2
        textCoord[texI++], textCoord[texI++],          // TexCoord 2
        x + asset.screenWidth, y + asset.screenHeight, // Position 3
        textCoord[texI++], textCoord[texI++],          // TexCoord 3
    };
}

static_assert(getHAlign(Position::UpLeft) == 0 && getHAlign(Position::Up) == 1 && getHAlign(Position::UpRight) == 2);
static_assert(getHAlign(Position::Left) == 0 && getHAlign(Position::Center) == 1 && getHAlign(Position::Right) == 2);
static_assert(getHAlign(Position::DownLeft) == 0 && getHAlign(Position::Down) == 1 && getHAlign(Position::DownRight) == 2);

static_assert(getVAlign(Position::DownLeft) == 0 && getVAlign(Position::Down) == 0 && getVAlign(Position::DownRight) == 0);
static_assert(getVAlign(Position::Left) == 1 && getVAlign(Position::Center) == 1 && getVAlign(Position::Right) == 1);
static_assert(getVAlign(Position::UpLeft) == 2 && getVAlign(Position::Up) == 2 && getVAlign(Position::UpRight) == 2);

constexpr GLushort kDrawSquareIndices[] = {0, 1, 2, 0, 2, 3};
constexpr int kFontWidth = 18;
constexpr int kFontHeight = 32;
constexpr GLfloat kFontHeightToWidth = static_cast<GLfloat>(kFontWidth) / static_cast<GLfloat>(kFontHeight);
constexpr int kGlyphsPerLine = 256 / kFontWidth;

} // namespace

struct Renderer::Impl
{
    explicit Impl(const Config &config)
        : printTextureElementArray{kDrawSquareIndices},
          printTexture{readFile(config.getShader("print_texture.vert")), readFile(config.getShader("print_texture.frag"))},
          printText{readFile(config.getShader("print_text.vert")), readFile(config.getShader("print_texture.frag"))},
          analogClockTexture{config.getTexture("clock.dds"), 240, 240, 256},
          arrowTexture{config.getTexture("arrow.dds"), 50, 50, 64},
          fontTexture{config.getTexture("font.dds").c_str()}
    {
        fontTexture.bind();
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    GlVboElementArray printTextureElementArray;

    // GlProgram printTexture
    GlProgram printTexture;
    GLint printTexturePosition = -1;
    GLint printTextureCoord = -1;

    // GlProgram printText
    GlProgram printText;
    GLint printTextPosition = -1;
    GLint printTextIndices = -1;

    // textures
    GraphicalAsset analogClockTexture;
    GraphicalAsset arrowTexture;
    GlTexture fontTexture;

    GraphicalAsset *getAsset(Asset asset)
    {
        switch (asset)
        {
        case Asset::Clock:
            return &analogClockTexture;
        case Asset::Arrow:
            return &arrowTexture;
        default:
            break;
        }
        return nullptr;
    }
};

Renderer::Renderer(const Config &config)
    : pimpl{std::make_unique<Impl>(config)}
{
    glClearColor(0., 0., 0., 1.);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, config.getDisplayWidth(), config.getDisplayHeight());

    // shader printTexture
    pimpl->printTexture.use();
    pimpl->printTexturePosition = pimpl->printTexture.getAttribLocation("a_positionScreen");
    pimpl->printTextureCoord = pimpl->printTexture.getAttribLocation("a_texCoord");
    glUniform1i(pimpl->printTexture.getUniformLocation("s_texture"), 0);
    glEnableVertexAttribArray(pimpl->printTexturePosition);
    glEnableVertexAttribArray(pimpl->printTextureCoord);

    // shader printText
    pimpl->printText.use();
    pimpl->printTextPosition = pimpl->printText.getAttribLocation("a_positionScreen");
    pimpl->printTextIndices = pimpl->printText.getAttribLocation("a_textIndice");
    glUniform1i(pimpl->printText.getUniformLocation("s_texture"), 0);
    glEnableVertexAttribArray(pimpl->printTextPosition);
    glEnableVertexAttribArray(pimpl->printTextIndices);
}

Renderer::~Renderer() = default;

void Renderer::begin()
{
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::end()
{
    glCheckError();
}

RendererSprite Renderer::renderSprite(Asset asset, int x, int y, Position align, int rotation90Degree)
{
    const auto graphicalAsset = pimpl->getAsset(asset);
    if (graphicalAsset == nullptr)
    {
        throw std::runtime_error{"No such asset"};
    }

    const GLfloat printX = x * (2. / getWidth()) - getHAlign(align) * graphicalAsset->screenWidth * .5 - 1;
    const GLfloat printY = y * (2. / getHeight()) - getVAlign(align) * graphicalAsset->screenHeight * .5 - 1;

    const auto vertices = getVertices2D(*graphicalAsset, printX, printY, rotation90Degree);
    return RendererSprite{pimpl->printTexture,
                          graphicalAsset->texture,
                          pimpl->printTextureElementArray,
                          pimpl->printTexturePosition,
                          pimpl->printTextureCoord,
                          GlVboArrayStatic{vertices.data(), vertices.size()}};
}

RendererText Renderer::renderText(int x, int y, int numCol, int numRow, Position align, int size)
{
    const GLfloat fontWidth = size * kFontHeightToWidth;
    const GLfloat fontHeight = size;

    static constexpr auto kXFactor = (2. / getWidth());
    static constexpr auto kYFactor = (2. / getHeight());

    const GLfloat glPrintX = (x - getHAlign(align) * numCol * fontWidth * .5) * kXFactor - 1;
    const GLfloat glPrintY = (y - getVAlign(align) * numRow * fontHeight * .5) * kYFactor - 1;
    const GLfloat glGlyphW = fontWidth * kXFactor;
    const GLfloat glGlyphH = fontHeight * kYFactor;

    std::vector<GLfloat> vertices;
    std::vector<GLushort> indices;
    vertices.reserve(numCol * numRow * 8);
    indices.reserve(numCol * numRow * (sizeof(kDrawSquareIndices) / sizeof(*kDrawSquareIndices)));
    unsigned int indiceOffset = 0;
    // start top top to bottom
    for (int row = numRow; row--;)
    {
        const GLfloat yf = glPrintY + row * glGlyphH;
        // left to right
        for (int col = 0; col < numCol; ++col)
        {
            const GLfloat xf = glPrintX + col * glGlyphW;

            const GLfloat glyph[] = {
                xf, yf + glGlyphH,            // Position 0
                xf, yf,                       // Position 1
                xf + glGlyphW, yf,            // Position 2
                xf + glGlyphW, yf + glGlyphH, // Position 3
            };
            const GLushort newIndices[] = {
                static_cast<GLushort>(indiceOffset), static_cast<GLushort>(indiceOffset + 1), static_cast<GLushort>(indiceOffset + 2), // 1st triangle
                static_cast<GLushort>(indiceOffset), static_cast<GLushort>(indiceOffset + 2), static_cast<GLushort>(indiceOffset + 3), // 2nd triangle
            };
            vertices.insert(vertices.end(), &glyph[0], &glyph[sizeof(glyph) / sizeof(*glyph)]);
            indices.insert(indices.end(), &newIndices[0], &newIndices[sizeof(newIndices) / sizeof(*newIndices)]);
            indiceOffset += 4;
        }
    }

    return RendererText{pimpl->printText,
                        pimpl->fontTexture,
                        pimpl->printTextPosition,
                        pimpl->printTextIndices,
                        kGlyphsPerLine,
                        GlVboArrayStatic{vertices.data(), vertices.size()},
                        GlVboElementArray{indices.data(), indices.size()}};
}

RendererTextStatic Renderer::renderStaticText(int x, int y, const char *text, Position align, int size)
{
    const GLfloat fontWidth = size * kFontHeightToWidth;
    const GLfloat fontHeight = size;
    const size_t textLen = std::strlen(text);

    const int numRow = std::count(text, text + textLen, '\n') + 1;
    const int numCol = numRow > 1 ? (std::strchr(text, '\n') - text) : textLen;

    static constexpr auto kXFactor = (2. / getWidth());
    static constexpr auto kYFactor = (2. / getHeight());

    const GLfloat glPrintX = (x - getHAlign(align) * numCol * fontWidth * .5) * kXFactor - 1;
    const GLfloat glPrintY = (y - getVAlign(align) * numRow * fontHeight * .5) * kYFactor - 1;
    const GLfloat glGlyphW = fontWidth * kXFactor;
    const GLfloat glGlyphH = fontHeight * kYFactor;

    std::vector<GLfloat> vertices;
    std::vector<GLushort> indices;
    vertices.reserve(textLen * 8);
    indices.reserve(textLen * (sizeof(kDrawSquareIndices) / sizeof(*kDrawSquareIndices)));
    unsigned int indiceOffset = 0;

    // start top top to bottom
    GLfloat yf = glPrintY + (numRow - 1) * glGlyphH;
    GLfloat xf = glPrintX;
    for (auto c = reinterpret_cast<const unsigned char *>(text); *c; ++c)
    {
        if (*c == '\n')
        {
            yf -= glGlyphH;
            xf = glPrintX;
        }
        else
        {
            const int glyphNumber = *c - 0x20;
            const int fontIndex = glyphNumber + glyphNumber / kGlyphsPerLine;

            const GLfloat glyph[] = {
                xf, yf + glGlyphH,                                      // Position 0
                static_cast<GLfloat>(fontIndex),                        // Font index 0
                xf, yf,                                                 // Position 1
                static_cast<GLfloat>(fontIndex + (kGlyphsPerLine + 1)), // Font index 1
                xf + glGlyphW, yf,                                      // Position 2
                static_cast<GLfloat>(fontIndex + (kGlyphsPerLine + 2)), // Font index 2
                xf + glGlyphW, yf + glGlyphH,                           // Position 3
                static_cast<GLfloat>(fontIndex + 1),                    // Font index 3
            };
            const GLushort newIndices[] = {
                static_cast<GLushort>(indiceOffset), static_cast<GLushort>(indiceOffset + 1), static_cast<GLushort>(indiceOffset + 2), // 1st triangle
                static_cast<GLushort>(indiceOffset), static_cast<GLushort>(indiceOffset + 2), static_cast<GLushort>(indiceOffset + 3), // 2nd triangle
            };
            vertices.insert(vertices.end(), &glyph[0], &glyph[sizeof(glyph) / sizeof(*glyph)]);
            indices.insert(indices.end(), &newIndices[0], &newIndices[sizeof(newIndices) / sizeof(*newIndices)]);

            indiceOffset += 4;
            xf += glGlyphW;
        }
    }

    return RendererTextStatic{pimpl->printText,
                              pimpl->fontTexture,
                              pimpl->printTextPosition,
                              pimpl->printTextIndices,
                              GlVboArrayStatic{vertices.data(), vertices.size()},
                              GlVboElementArray{indices.data(), indices.size()}};
}

std::ostream &Renderer::toStream(std::ostream &str) const
{
    str << "OpenGL\n - Vendor: " << glGetString(GL_VENDOR)
        << "\n - Renderer: " << glGetString(GL_RENDERER)
        << "\n - Version: " << glGetString(GL_VERSION)
        << "\n - Shading language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
        << "\n - Extensions: " << glGetString(GL_EXTENSIONS)
        << "\nGL Programs:\n - printTexture: " << pimpl->printTexture
        << " - printText: " << pimpl->printText;
    return str;
}
