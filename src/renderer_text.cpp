#include "renderer_text.hpp"

#include "gl_shader.hpp"
#include "gl_texture.hpp"
#include "gl_vbo.hpp"
#include "toolbox_gl.hpp"

#include <vector>

namespace
{
struct RendererTextBase
{
    RendererTextBase(GlProgram &program,
                     GlTexture &texture,
                     GLint attribPositionOnScreen,
                     GLint attribTextIndice,
                     GlVboArrayStatic &&vboVertices,
                     GlVboElementArray &&vboIndices)
        : program{program},
          texture{texture},
          attribPositionOnScreen{attribPositionOnScreen},
          attribTextIndice{attribTextIndice},
          vboVertices{std::move(vboVertices)},
          vboIndices{std::move(vboIndices)}
    {
    }

    GlProgram &program;
    GlTexture &texture;
    GLint attribPositionOnScreen;
    GLint attribTextIndice;

    // owned
    GlVboArrayStatic vboVertices;
    GlVboElementArray vboIndices;
};

void addGlyph(unsigned char *textIndices, int index, int nextLine)
{
    textIndices[0] = index;
    textIndices[1] = index + nextLine;
    textIndices[2] = index + nextLine + 1;
    textIndices[3] = index + 1;
}

} // namespace

struct RendererText::Impl : RendererTextBase
{
    Impl(GlProgram &program,
         GlTexture &texture,
         GLint attribPositionOnScreen,
         GLint attribTextIndice,
         int glyphsPerLine,
         GlVboArrayStatic &&vboVertices,
         GlVboElementArray &&vboIndices)
        : RendererTextBase{program, texture, attribPositionOnScreen, attribTextIndice, std::move(vboVertices), std::move(vboIndices)},
          glyphsPerLine{glyphsPerLine},
          textIndices(this->vboIndices.triangles() * 2),
          vboTextIndices{textIndices.data(), textIndices.size()}
    {
        textIndices.resize(this->vboIndices.triangles() * 2);
    }

    // pimpl->vertices == 8x size of text (2 coord * 4 points)
    // pimpl->indices == 6x size of text (2 triangles)
    // pimpl->textIndices == 4x size of text  (4 points)

    const int glyphsPerLine;

    // to avoid rebuilding
    std::string text;
    std::vector<unsigned char> textIndices;

    // owned
    GlVboArrayDynamic vboTextIndices;
};

// RendererText

RendererText::RendererText(GlProgram &program,
                           GlTexture &texture,
                           int attribPositionOnScreen,
                           int attribTextIndice,
                           int glyphsPerLine,
                           GlVboArrayStatic vboVertices,
                           GlVboElementArray vboIndices)
    : pimpl{std::make_unique<Impl>(program, texture, attribPositionOnScreen, attribTextIndice, glyphsPerLine, std::move(vboVertices), std::move(vboIndices))}
{
}

RendererText::~RendererText() = default;

void RendererText::set(const char *text)
{
    pimpl->vboTextIndices.bind();
    if (const std::string_view textView{text}; textView != pimpl->text)
    {
        unsigned char *const textIndices = &pimpl->textIndices.front();

        const int glyphsPerLine = pimpl->glyphsPerLine;
        const int nextLine = glyphsPerLine + 1;
        const auto uText = reinterpret_cast<const unsigned char *>(text);

        unsigned int i = 0;
        for (unsigned int imax = std::min<unsigned>(textView.size(), pimpl->textIndices.size() / 4); i < imax; ++i)
        {
            const int glyphNumber = uText[i] - 0x20;
            const int index = glyphNumber + glyphNumber / glyphsPerLine;
            addGlyph(textIndices + (i << 2), index, nextLine);
        }

        for (unsigned int imax = pimpl->textIndices.size() / 4; i < imax; ++i)
        {
            addGlyph(textIndices + (i << 2), 0, nextLine);
        }

        pimpl->text = textView;
        pimpl->vboTextIndices.set(pimpl->textIndices.data(), pimpl->textIndices.size());
    }
}

void RendererText::print()
{
    pimpl->program.use();

    pimpl->vboTextIndices.bind();
    pimpl->vboTextIndices.draw(pimpl->attribTextIndice, 1);

    pimpl->vboVertices.bind();
    pimpl->vboVertices.draw(pimpl->attribPositionOnScreen, 2);

    pimpl->texture.bind();
    pimpl->vboIndices.draw();
}

// RendererTextStatic

struct RendererTextStatic::Impl : RendererTextBase
{
    using RendererTextBase::RendererTextBase;
};

RendererTextStatic::RendererTextStatic(GlProgram &program,
                                       GlTexture &texture,
                                       int attribPositionOnScreen,
                                       int attribTextIndice,
                                       GlVboArrayStatic vboVertices,
                                       GlVboElementArray vboIndices)
    : pimpl{std::make_unique<Impl>(program, texture, attribPositionOnScreen, attribTextIndice, std::move(vboVertices), std::move(vboIndices))}
{
}

RendererTextStatic::~RendererTextStatic() = default;

void RendererTextStatic::print()
{
    pimpl->program.use();

    pimpl->vboVertices.bind();
    pimpl->vboVertices.draw<GLfloat>(pimpl->attribPositionOnScreen, 2, 0, 3);
    pimpl->vboVertices.draw<GLfloat>(pimpl->attribTextIndice, 1, 2, 3);

    pimpl->texture.bind();
    pimpl->vboIndices.draw();
}
