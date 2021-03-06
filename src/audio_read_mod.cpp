#include "audio_read_mod.hpp"

#ifndef NO_AUDIO_READ_MOD

#include "toolbox_io.hpp"

#include <libmodplug/modplug.h>

#include <algorithm>
#include <cstring>
#include <iostream>

namespace
{
struct ModDeleter
{
    void operator()(ModPlugFile *obj) const
    {
        ModPlug_Unload(obj);
    }
};

template <typename T>
using ModUnique = std::unique_ptr<T, ModDeleter>;

constexpr ModPlug_Settings kSettings = {
    MODPLUG_ENABLE_OVERSAMPLING |
        MODPLUG_ENABLE_NOISE_REDUCTION |
        MODPLUG_ENABLE_REVERB |
        MODPLUG_ENABLE_MEGABASS |
        MODPLUG_ENABLE_SURROUND, ///< mFlags
    2,                           ///< mChannels
    16,                          ///< mBits
    44100,                       ///< mFrequency
    MODPLUG_RESAMPLE_FIR,        ///< mResamplingMode
    0,                           ///< mStereoSeparation
    0,                           ///< mMaxMixChannels
    30,                          ///< mReverbDepth
    100,                         ///< mReverbDelay
    40,                          ///< mBassAmount
    30,                          ///< mBassRange
    20,                          ///< mSurroundDepth
    20,                          ///< mSurroundDelay
    0,                           ///< mLoopCount
};

} // namespace

constexpr char kExtensions[][4] = {
    "669",
    "abc",
    "amf",
    "ams",
    "dbm",
    "dmf",
    "dsm",
    "far",
    "it",
    "mdl",
    "med",
    "mid",
    "mod",
    "mt2",
    "mtm",
    "okt",
    "pat",
    "psm",
    "ptm",
    "s3m",
    "stm",
    "ult",
    "umx",
    "xm",
};

struct AudioReadMod::Impl
{
    ModUnique<ModPlugFile> mod;
};

AudioReadMod::AudioReadMod(Impl impl)
    : pimpl{std::make_unique<Impl>(std::move(impl))}
{
}

AudioReadMod::~AudioReadMod() = default;

void AudioReadMod::loadLib(std::ostream &)
{
    ModPlug_SetSettings(&kSettings);
}

std::unique_ptr<AudioRead> AudioReadMod::create(FILEUnique &file, const char *extension)
{
    if (file == nullptr || extension == nullptr ||
        std::binary_search(kExtensions, kExtensions + sizeof(kExtensions) / sizeof(*kExtensions), extension,
                           [](const char *a, const char *b) { return strcasecmp(a, b) < 0; }) == false)
    {
        return nullptr;
    }

    Impl impl;
    try
    {
        const MmapFile mmapFile{file.get()};
        impl.mod.reset(ModPlug_Load(mmapFile.content, mmapFile.size));
    }
    catch (const std::exception &e)
    {
        std::cerr << "ModPlug: caught exception while loading the file: " << e.what() << std::endl;
        return nullptr;
    }

    if (impl.mod == nullptr)
    {
        std::cerr << "ModPlug: could not load the file as a MOD" << std::endl;
        return nullptr;
    }

    // the file is read... we don't need to keep a reference to the file
    file.reset();
    return std::make_unique<AudioReadMod>(std::move(impl));
}

int AudioReadMod::getChannels() const
{
    return kSettings.mChannels;
}

uint64_t AudioReadMod::getSamples() const
{
    return ModPlug_GetLength(pimpl->mod.get()) * kSettings.mFrequency / 1000;
}

int AudioReadMod::getRate() const
{
    return kSettings.mFrequency;
}

size_t AudioReadMod::readBuffer(char *buffer, size_t bufferSize, bool loop)
{
    size_t totalRead = 0;
    while (totalRead < bufferSize)
    {
        const int read = ModPlug_Read(pimpl->mod.get(), buffer + totalRead, bufferSize - totalRead);
        if (read <= 0)
        {
            break;
        }

        totalRead += read;
    }

    if (totalRead < bufferSize && loop)
    {
        // EOF, loop
        ModPlug_Seek(pimpl->mod.get(), 0);
        return totalRead + readBuffer(buffer + totalRead, bufferSize - totalRead, false);
    }

    return totalRead;
}

std::ostream &AudioReadMod::toStream(std::ostream &str) const
{
    return str << "mod type" << ModPlug_GetModuleType(pimpl->mod.get())
               << "instruments=" << ModPlug_NumInstruments(pimpl->mod.get())
               << " samples=" << ModPlug_NumSamples(pimpl->mod.get())
               << " channels=" << ModPlug_NumChannels(pimpl->mod.get())
               << " duration_ms=" << ModPlug_GetLength(pimpl->mod.get());
}

#endif // NO_AUDIO_READ_MOD
