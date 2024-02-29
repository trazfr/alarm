#pragma once

#include <memory>

class GlProgram;
class GlTexture;
class GlVboArrayStatic;
class GlVboElementArray;

/**
 * @brief Sprite to be displayed
 *
 * created from Renderer
 *
 * @sa Renderer
 */
class RendererSprite
{
public:
    struct Impl;
    RendererSprite(GlProgram &program,
                   GlTexture &texture,
                   GlVboElementArray &vboIndices,
                   int position,
                   int textureCoord,
                   GlVboArrayStatic &&vboVertices);
    ~RendererSprite();

    /**
     * Actually display the sprite (call OpenGL to perform the display)
     */
    void print();

private:
    std::unique_ptr<Impl> pimpl;
};
