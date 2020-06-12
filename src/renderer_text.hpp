#pragma once

#include <memory>

class GlProgram;
class GlTexture;
class GlVboArrayStatic;
class GlVboElementArray;

/**
 * @brief Text box whose content may change
 * 
 * created from Renderer
 * 
 * @sa Renderer
 */
class RendererText
{
public:
    struct Impl;
    RendererText(GlProgram &program,
                 GlTexture &texture,
                 int attribPositionOnScreen,
                 int attribTextIndice,
                 int glyphsPerLine,
                 GlVboArrayStatic vboVertices,
                 GlVboElementArray vboIndices);
    ~RendererText();

    /**
     * Update the text in the textbox
     */
    void set(const char *text);

    /**
     * Actually display the text (call OpenGL to perform the display)
     */
    void print();

private:
    std::unique_ptr<Impl> pimpl;
};

/**
 * @brief Unmutable text box
 * 
 * created from Renderer
 * 
 * @sa Renderer
 */
class RendererTextStatic
{
public:
    struct Impl;
    RendererTextStatic(GlProgram &program,
                       GlTexture &texture,
                       int attribPositionOnScreen,
                       int attribTextIndice,
                       GlVboArrayStatic vboVertices,
                       GlVboElementArray vboIndices);
    ~RendererTextStatic();

    /**
     * Actually display the text (call OpenGL to perform the display)
     */
    void print();

private:
    std::unique_ptr<Impl> pimpl;
};
