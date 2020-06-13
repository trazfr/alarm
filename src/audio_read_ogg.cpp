#include "audio_read_ogg.hpp"

#ifndef NO_AUDIO_READ_OGG

#include <vorbis/vorbisfile.h>

#include <cstring>
#include <iostream>

struct AudioReadOgg::Impl
{
    ~Impl()
    {
        if (vf.datasource)
        {
            ov_clear(&vf);
        }
    }
    FILEUnique file;
    OggVorbis_File vf = {};
    vorbis_info *vi = nullptr;
    int currentSection = 0;
};

AudioReadOgg::AudioReadOgg(std::unique_ptr<Impl> impl)
    : pimpl{std::move(impl)}
{
}

AudioReadOgg::~AudioReadOgg() = default;

std::unique_ptr<AudioRead> AudioReadOgg::create(FILEUnique &file, const char *)
{
    if (file == nullptr)
    {
        return nullptr;
    }
    std::fseek(file.get(), 0, SEEK_SET);

    // early failure if this is not an Ogg Vorbis
    OggVorbis_File vf;
    if (ov_test_callbacks(file.get(), &vf, nullptr, 0, OV_CALLBACKS_NOCLOSE))
    {
        return nullptr;
    }
    if (ov_test_open(&vf))
    {
        std::cerr << "Ogg Vorbis could not load the file" << std::endl;
        ov_clear(&vf);
        return nullptr;
    }
    auto pimpl = std::make_unique<Impl>();
    std::memcpy(&pimpl->vf, &vf, sizeof(vf));

    if (pimpl->vi = ov_info(&pimpl->vf, -1); pimpl->vi == nullptr)
    {
        std::cerr << "Ogg Vorbis could not get info about the file" << std::endl;
        return nullptr;
    }

    pimpl->file = std::move(file);
    return std::make_unique<AudioReadOgg>(std::move(pimpl));
}

int AudioReadOgg::getChannels() const
{
    return pimpl->vi->channels;
}

uint64_t AudioReadOgg::getSamples() const
{
    return ov_pcm_total(&pimpl->vf, -1);
}

int AudioReadOgg::getRate() const
{
    return pimpl->vi->rate;
}

size_t AudioReadOgg::readBuffer(char *buffer, size_t bufferSize, bool loop)
{
    size_t totalRead = 0;
    while (totalRead < bufferSize)
    {
        const long read = ov_read(&pimpl->vf, buffer + totalRead, bufferSize - totalRead, BYTE_ORDER == BIG_ENDIAN, 2, 1, &pimpl->currentSection);
        if (read <= 0)
        {
            break;
        }

        totalRead += read;
    }

    if (totalRead < bufferSize && loop)
    {
        // EOF, loop
        ov_pcm_seek(&pimpl->vf, 0);
        return totalRead + readBuffer(buffer + totalRead, bufferSize - totalRead, false);
    }

    return totalRead;
}

std::ostream &AudioReadOgg::toStream(std::ostream &str) const
{
    return str << "Ogg Vorbis rate=" << getRate() << " channels=" << getChannels();
}

#endif // NO_AUDIO_READ_OGG
