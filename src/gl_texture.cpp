#include "gl_texture.hpp"

#include "gl_texture_loader.hpp"
#include "toolbox_gl.hpp"

GlTexture::Guard::~Guard()
{
    if (texture)
    {
        glDeleteTextures(1, &texture);
    }
}

GlTexture::GlTexture(const char *filename)
    : GlTexture{GlTextureLoader{filename}}
{
}

GlTexture::GlTexture(const GlTextureLoader &loader)
{
    glGenTextures(1, &guard.texture);
    glBindTexture(GL_TEXTURE_2D, get());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    const GLenum glFormat = loader.getGlFormat();
    const GLenum glType = loader.getGlType();

    for (uint32_t level = 0; level < loader.getMipmapCount(); ++level)
    {
        const char *data = loader.getMipmap(level);
        const uint32_t size = loader.getMipmapSize(level);
        const uint32_t width = loader.getMipmapWidth(level);
        const uint32_t height = loader.getMipmapHeight(level);

        switch (glFormat)
        {
#ifdef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
#endif
#ifdef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
#endif
#ifdef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
#endif
            glCompressedTexImage2D(GL_TEXTURE_2D, level, glFormat, width, height, 0, size, data);
            break;

        default:
            glTexImage2D(GL_TEXTURE_2D, level, glFormat, width, height, 0, glFormat, glType, data);
            break;
        }
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    if (loader.getMipmapCount() == 1)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
}

GlTexture::GlTexture(GlTexture &&other)
{
    std::swap(guard.texture, other.guard.texture);
}

GlTexture::~GlTexture() = default;

GlTexture &GlTexture::operator=(GlTexture &&other)
{
    std::swap(guard.texture, other.guard.texture);
    return *this;
}

void GlTexture::bind()
{
    glBindTexture(GL_TEXTURE_2D, get());
}

unsigned int GlTexture::get() const
{
    return guard.texture;
}
