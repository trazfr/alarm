#pragma once

#include <memory>

/**
 * @brief Load a .DDS file (Direct Draw Surface)
 */
class GlTextureLoader
{
public:
    struct Impl;

    explicit GlTextureLoader(const char *filename);
    ~GlTextureLoader();

    // mipmap data
    uint32_t getMipmapCount() const;
    uint32_t getMipmapSize(uint32_t mipmap) const;
    const char *getMipmap(uint32_t mipmap) const;
    uint32_t getMipmapWidth(uint32_t mipmap) const;
    uint32_t getMipmapHeight(uint32_t mipmap) const;

    /**
     * Get the internalFormat/format to be used in:
     *
     * @arg glCompressedTexImage2D()
     * @arg glTexImage2D()
     */
    unsigned int getGlFormat() const;

    /**
     * Get the type to be used in:
     *
     * @arg glTexImage2D()
     */
    unsigned int getGlType() const;

private:
    std::unique_ptr<Impl> pimpl;
};
