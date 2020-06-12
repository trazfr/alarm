#include "gl_texture_loader.hpp"

#include "toolbox_gl.hpp"
#include "toolbox_io.hpp"

#include <byteswap.h>
#include <endian.h>

#include <vector>

struct GlTextureLoader::Impl
{
    /**
     * @brief Stores the mipmap's raw data
     */
    struct MipMap
    {
        MipMap(std::vector<char> data,
               uint32_t width,
               uint32_t height)
            : data{std::move(data)},
              width{width},
              height{height}
        {
        }
        std::vector<char> data;
        uint32_t width;
        uint32_t height;
    };

    GLenum glFormat;
    GLenum glType;
    std::vector<MipMap> mipMaps;
};

namespace
{

using MipMap = GlTextureLoader::Impl::MipMap;

/**
 * Compute FOURCC as uint32_t regardless of byte ordering
 */
constexpr uint32_t fourchar(char a, char b, char c, char d)
{
    if constexpr (BYTE_ORDER == BIG_ENDIAN)
    {
        return a << 24 | b << 16 | c << 8 | d;
    }
    if constexpr (BYTE_ORDER == LITTLE_ENDIAN)
    {
        return d << 24 | c << 16 | b << 8 | a;
    }
}

// https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat
struct DdsPixelFormat
{
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwAlphaBitMask;
};

// https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
struct DdsHeader
{
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DdsPixelFormat ddspf;
    uint32_t dwCaps[4];
    uint32_t dwReserved2;
};

// https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide
struct DdsFileHeader
{
    uint32_t dwMagic;
    DdsHeader header;
};
static_assert(sizeof(DdsFileHeader) == 128);

// constants as per Microsoft's doc

constexpr uint32_t DDSD_CAPS = 0x1;
constexpr uint32_t DDSD_HEIGHT = 0x2;
constexpr uint32_t DDSD_WIDTH = 0x4;
constexpr uint32_t DDSD_PITCH = 0x8;
constexpr uint32_t DDSD_PIXELFORMAT = 0x1000;
constexpr uint32_t DDSD_MIPMAPCOUNT = 0x20000;
constexpr uint32_t DDSD_LINEARSIZE = 0x80000;
constexpr uint32_t DDSD_DEPTH = 0x800000;

constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
constexpr uint32_t DDPF_ALPHA = 0x2;
constexpr uint32_t DDPF_FOURCC = 0x4;
constexpr uint32_t DDPF_RGB = 0x40;

constexpr GLenum fourccToGlInternalFormat(uint32_t fourcc)
{
    switch (fourcc)
    {
#ifdef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
    case fourchar('D', 'X', 'T', '1'):
        return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
#endif
#ifdef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
    case fourchar('D', 'X', 'T', '3'):
        return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
#endif
#ifdef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
    case fourchar('D', 'X', 'T', '5'):
        return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
#endif
    default:
        break;
    }
    return 0;
}

// format, type, reorderBytes, offset, jumpBytes
std::tuple<GLenum, GLenum, uint32_t, uint32_t, uint32_t> masksToGlInternalFormat(uint32_t bitCount,
                                                                                 uint32_t rMask,
                                                                                 uint32_t gMask,
                                                                                 uint32_t bMask,
                                                                                 uint32_t aMask)
{
    if (bitCount == 16)
    {
        // R5G6B5
        if (rMask == 0xf8000000 && gMask == 0x07e00000 && bMask == 0x001f0000 && aMask == 0x00000000)
        {
            return {GL_RGB, GL_UNSIGNED_SHORT_5_6_5, 0, 0, 0};
        }
        // RGBA4
        if (rMask == 0xf0000000 && gMask == 0x0f000000 && bMask == 0x00f00000 && aMask == 0x000f0000)
        {
            return {GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4, 0, 0, 0};
        }
        // RGB5A1
        if (rMask == 0xf8000000 && gMask == 0x07c00000 && bMask == 0x003e0000 && aMask == 0x00010000)
        {
            return {GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, 0, 0, 0};
        }
    }
    if (bitCount == 24)
    {
        // RGB8
        if (rMask == 0xff000000 && gMask == 0x00ff0000 && bMask == 0x0000ff00 && aMask == 0x00000000)
        {
            return {GL_RGB, GL_UNSIGNED_BYTE, 0, 0, 0};
        }
        // BGR8
        if (bMask == 0xff000000 && gMask == 0x00ff0000 && rMask == 0x0000ff00 && aMask == 0x00000000)
        {
            return {GL_RGB, GL_UNSIGNED_BYTE, 3, 0, 3};
        }
    }
    if (bitCount == 32)
    {
        // BGRA8
        if (bMask == 0xff000000 && gMask == 0x00ff0000 && rMask == 0x0000ff00 && aMask == 0x000000ff)
        {
            return {GL_RGBA, GL_UNSIGNED_BYTE, 3, 0, 4};
        }
        // RGBA8
        if (rMask == 0xff000000 && gMask == 0x00ff0000 && bMask == 0x0000ff00 && aMask == 0x000000ff)
        {
            return {GL_RGBA, GL_UNSIGNED_BYTE, 0, 0, 0};
        }
        // ABGR8
        if (aMask == 0xff000000 && bMask == 0x00ff0000 && gMask == 0x0000ff00 && rMask == 0x000000ff)
        {
            return {GL_RGBA, GL_UNSIGNED_BYTE, 4, 0, 4};
        }
    }
    throw std::runtime_error{"Texture: Invalid format"};
}

void reorder2(char *ptr, uint32_t size, uint32_t jumpBytes)
{
    for (const char *const end = ptr + size; ptr != end; ptr += jumpBytes)
    {
        uint16_t &toSwap = *reinterpret_cast<uint16_t *>(ptr);
        toSwap = bswap_16(toSwap);
    }
}

void reorder3(char *ptr, uint32_t size, uint32_t jumpBytes)
{
    for (const char *const end = ptr + size; ptr != end; ptr += jumpBytes)
    {
        std::swap(ptr[0], ptr[2]);
    }
}

void reorder4(char *ptr, uint32_t size, uint32_t jumpBytes)
{
    for (const char *const end = ptr + size; ptr != end; ptr += jumpBytes)
    {
        uint32_t &toSwap = *reinterpret_cast<uint32_t *>(ptr);
        toSwap = bswap_32(toSwap);
    }
}

// only support DXT1, DXT3, DXT5
void loadFourcc(GlTextureLoader::Impl &impl,
                const char *buffer,
                uint32_t width,
                uint32_t height,
                uint32_t mipMapCount,
                uint32_t fourcc)
{
    impl.mipMaps.reserve(mipMapCount);
    impl.glFormat = fourccToGlInternalFormat(fourcc);
    const uint32_t blockSize = (impl.glFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) ? 8 : 16;

    uint32_t offset = 0;
    for (uint32_t level = 0; level < mipMapCount; ++level)
    {
        const char *ptr = buffer + offset;
        const uint32_t size = (width / 4) * (height / 4) * blockSize;
        impl.mipMaps.emplace_back(std::vector<char>{ptr, ptr + size}, width, height);

        offset += size;
        width >>= 1;
        height >>= 1;
    }
}

void loadRgba(GlTextureLoader::Impl &impl,
              const char *buffer,
              uint32_t width,
              uint32_t height,
              uint32_t mipMapCount,
              uint32_t bitCount,
              uint32_t rMask,
              uint32_t gMask,
              uint32_t bMask,
              uint32_t aMask)
{
    impl.mipMaps.reserve(mipMapCount);
    const auto [glFormat, glType, reorderBytes, reorderOffset, reorderJump] = masksToGlInternalFormat(bitCount, rMask, gMask, bMask, aMask);
    impl.glFormat = glFormat;
    impl.glType = glType;
    const uint32_t blockSize = bitCount >> 3;

    uint32_t offset = 0;
    for (uint32_t level = 0; level < mipMapCount; ++level)
    {
        const char *const mipmapBuffer = buffer + offset;
        const uint32_t mipmapSize = width * height * blockSize;
        impl.mipMaps.emplace_back(std::vector<char>{mipmapBuffer, mipmapBuffer + mipmapSize}, width, height);
        auto &data = impl.mipMaps.back().data;

        switch (reorderBytes)
        {
        case 2:
            reorder2(data.data() + reorderOffset, data.size() - reorderOffset, reorderJump);
            break;
        case 3:
            reorder3(data.data() + reorderOffset, data.size() - reorderOffset, reorderJump);
            break;
        case 4:
            reorder4(data.data() + reorderOffset, data.size() - reorderOffset, reorderJump);
            break;
        default:
            break;
        }

        offset += mipmapSize;
        width >>= 1;
        height >>= 1;
    }
}

} // namespace

GlTextureLoader::GlTextureLoader(const char *filename)
    : pimpl{std::make_unique<Impl>()}
{
    MmapFile mmap{filename};
    if (mmap.size < sizeof(DdsFileHeader))
    {
        throw std::runtime_error{"Not a DDS file: too small"};
    }

    const auto fileHeader = reinterpret_cast<const DdsFileHeader *>(mmap.content);
    if (fileHeader->dwMagic != fourchar('D', 'D', 'S', ' '))
    {
        throw std::runtime_error{"Not a DDS file: Wrong magic number"};
    }

    const DdsHeader &header = fileHeader->header;
    if (le32toh(header.dwSize) != sizeof(header))
    {
        throw std::runtime_error{"Not a DDS file: Wrong header size"};
    }

    const uint32_t ddsdFlags = le32toh(header.dwFlags);
    if ((ddsdFlags & (DDSD_CAPS | DDSD_PIXELFORMAT)) != (DDSD_CAPS | DDSD_PIXELFORMAT))
    {
        throw std::runtime_error{"Missing mandatory flags in DDSD"};
    }

    const uint32_t width = le32toh(header.dwWidth);
    const uint32_t height = le32toh(header.dwHeight);
    if ((width & (width - 1)) | (height & (height - 1)))
    {
        throw std::runtime_error{"Size is not a power of 2"};
    }
    const DdsPixelFormat &ddspf = header.ddspf;
    if (le32toh(ddspf.dwSize) != sizeof(ddspf))
    {
        throw std::runtime_error{"Not a DDS file: Wrong DDS_PIXELFORMAT size"};
    }

    const uint32_t ddpfFlags = le32toh(ddspf.dwFlags);

    const uint32_t mipMapCount = (ddsdFlags & DDSD_MIPMAPCOUNT) ? le32toh(header.dwMipMapCount) : 1;

    const char *buffer = reinterpret_cast<const char *>(mmap.content) + sizeof(DdsFileHeader);
    if (ddpfFlags & DDPF_FOURCC)
    {
        loadFourcc(*pimpl, buffer, width, height, mipMapCount,
                   ddspf.dwFourCC);
    }
    else if (ddpfFlags & DDPF_RGB)
    {
        loadRgba(*pimpl, buffer, width, height, mipMapCount,
                 le32toh(ddspf.dwRGBBitCount),
                 be32toh(ddspf.dwRBitMask),
                 be32toh(ddspf.dwGBitMask),
                 be32toh(ddspf.dwBBitMask),
                 ddpfFlags & DDPF_ALPHAPIXELS ? be32toh(ddspf.dwAlphaBitMask) : 0);
    }
}

GlTextureLoader::~GlTextureLoader() = default;

uint32_t GlTextureLoader::getMipmapCount() const
{
    return pimpl->mipMaps.size();
}

uint32_t GlTextureLoader::getMipmapSize(uint32_t mipmap) const
{
    if (mipmap < pimpl->mipMaps.size())
    {
        return pimpl->mipMaps[mipmap].data.size();
    }
    return 0;
}

const char *GlTextureLoader::getMipmap(uint32_t mipmap) const
{
    if (mipmap < pimpl->mipMaps.size())
    {
        return pimpl->mipMaps[mipmap].data.data();
    }
    return nullptr;
}

uint32_t GlTextureLoader::getMipmapWidth(uint32_t mipmap) const
{
    if (mipmap < pimpl->mipMaps.size())
    {
        return pimpl->mipMaps[mipmap].width;
    }
    return 0;
}

uint32_t GlTextureLoader::getMipmapHeight(uint32_t mipmap) const
{
    if (mipmap < pimpl->mipMaps.size())
    {
        return pimpl->mipMaps[mipmap].height;
    }
    return 0;
}

unsigned int GlTextureLoader::getGlFormat() const
{
    static_assert(std::is_same_v<decltype(pimpl->glFormat), decltype(getGlFormat())>);
    return pimpl->glFormat;
}

unsigned int GlTextureLoader::getGlType() const
{
    static_assert(std::is_same_v<decltype(pimpl->glType), decltype(getGlType())>);
    return pimpl->glType;
}
