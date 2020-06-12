#pragma once

#include "toolbox_position.hpp"

#include <iosfwd>
#include <memory>

class Config;
class RendererSprite;
class RendererText;
class RendererTextStatic;

#define DEGREE "\x82"

enum class Asset
{
    Clock, ///< Clock from the main screen
    Arrow, ///< Triangle on the configuration screens
};

/**
 * @brief render assets on screen
 */
class Renderer
{
public:
    struct Impl;

    static constexpr int kDefaultCharSize = 32;

    Renderer(const Config &config);
    ~Renderer();

    /**
     * Logical width to place elements on screen
     */
    static constexpr unsigned int getWidth()
    {
        return 320;
    }

    /**
     * Logical height to place elements on screen
     */
    static constexpr unsigned int getHeight()
    {
        return 240;
    }

    /**
     * Method to be called before any drawing on screen
     * 
     * Clears the OpenGL buffer
     */
    void begin();

    /**
     * Method to be called after all drawing on screen
     * 
     * Check for OpenGL errors
     */
    void end();

    /**
     * Render a sprite at a given position
     * 
     * @param asset sprite to be rendered
     * @param x X position
     * @param y Y position
     * @param align where do (x,y) refer regarding the sprite? DownLeft corner...
     * @param rotation90Degree is in range [0,3]. The rotation of the sprite (0: no rotation, 1: 90deg, 2: 180deg, 3: 270deg)
     */
    RendererSprite renderSprite(Asset asset, int x, int y, Position align, int rotation90Degree = 0);

    /**
     * Create a text box. The text can change
     */
    RendererText renderText(int x, int y, int numCol, int numRow, Position align, int size = kDefaultCharSize);

    /**
     * Create a text box. The text is fixed
     */
    RendererTextStatic renderStaticText(int x, int y, const char *text, Position align, int size = kDefaultCharSize);

    friend std::ostream &operator<<(std::ostream &str, const Renderer &renderer)
    {
        return renderer.toStream(str);
    }

private:
    std::ostream &toStream(std::ostream &str) const;

    std::unique_ptr<Impl> pimpl;
};
