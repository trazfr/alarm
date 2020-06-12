#pragma once

class GlTextureLoader;

/**
 * @brief stores an OpenGL texture
 */
class GlTexture
{
    explicit GlTexture(const GlTexture &) = delete;
    GlTexture &operator=(const GlTexture &) = delete;

public:
    explicit GlTexture() = default;
    explicit GlTexture(const char *filename);
    explicit GlTexture(const GlTextureLoader &loader);

    explicit GlTexture(GlTexture &&other);
    GlTexture &operator=(GlTexture &&other);

    ~GlTexture();

    /**
     * Bind the texture (ie. make it active)
     */
    void bind();

    /**
     * Get the OpenGL identifier for the texture
     */
    unsigned int get() const;

private:
    /**
     * @brief Destroy the texture on OpenGL side in the destructor
     */
    struct Guard
    {
        ~Guard();
        unsigned int texture = 0;
    };

    Guard guard;
};
