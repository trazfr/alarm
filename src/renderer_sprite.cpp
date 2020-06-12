#include "renderer_sprite.hpp"

#include "gl_shader.hpp"
#include "gl_texture.hpp"
#include "gl_vbo.hpp"
#include "toolbox_gl.hpp"

struct RendererSprite::Impl
{
    Impl(GlProgram &program,
         GlTexture &texture,
         GlVboElementArray &vboIndices,
         GLint position,
         GLint textureCoord,
         GlVboArrayStatic &&vboVertices)
        : program{program},
          texture{texture},
          vboIndices{vboIndices},
          position{position},
          textureCoord{textureCoord},
          vboVertices{std::move(vboVertices)}
    {
    }
    // not owned
    GlProgram &program;
    GlTexture &texture;
    GlVboElementArray &vboIndices;
    GLint position;
    GLint textureCoord;

    // owned
    GlVboArrayStatic vboVertices;
};

RendererSprite::RendererSprite(GlProgram &program,
                               GlTexture &texture,
                               GlVboElementArray &vboIndices,
                               int position,
                               int textureCoord,
                               GlVboArrayStatic &&vboVertices)
    : pimpl{std::make_unique<Impl>(program, texture, vboIndices, position, textureCoord, std::move(vboVertices))}
{
}

RendererSprite::~RendererSprite() = default;

void RendererSprite::print()
{
    pimpl->program.use();

    pimpl->vboVertices.bind();
    pimpl->vboVertices.draw<GLfloat>(pimpl->position, 2, 0, 4);
    pimpl->vboVertices.draw<GLfloat>(pimpl->textureCoord, 2, 2, 4);

    pimpl->texture.bind();
    pimpl->vboIndices.draw();
}
