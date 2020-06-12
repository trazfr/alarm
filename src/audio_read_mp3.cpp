#include "audio_read_mp3.hpp"

#include "error.hpp"

#include <mpg123.h>

#include <cstring>
#include <iostream>

namespace
{

constexpr long kRate = 44100;
constexpr int kChannels = MPG123_STEREO;
constexpr int kEncodings = MPG123_ENC_SIGNED_16;

struct Mpg123Deleter
{
    void operator()(mpg123_handle *obj) const
    {
        mpg123_close(obj);
        mpg123_delete(obj);
    }
};

template <typename T>
using Mpg123Unique = std::unique_ptr<T, Mpg123Deleter>;

ssize_t mp3Read(void *fd, void *buf, size_t count)
{
    const auto file = reinterpret_cast<FILE *>(fd);
    const size_t read = std::fread(buf, 1, count, file);
    if (read == 0 && std::ferror(file))
    {
        return -1;
    }
    return read;
}

off_t mp3Seek(void *fd, off_t offset, int whence)
{
    const auto file = reinterpret_cast<FILE *>(fd);
    if (std::fseek(file, offset, whence))
    {
        return -1;
    }
    return std::ftell(file);
}

} // namespace

struct AudioReadMp3::Impl
{
    FILEUnique file;
    Mpg123Unique<mpg123_handle> handle;

    long rate;
    int channels;
    int encoding;
};

AudioReadMp3::AudioReadMp3(Impl impl)
    : pimpl{std::make_unique<Impl>(std::move(impl))}
{
}

AudioReadMp3::~AudioReadMp3() = default;

void AudioReadMp3::loadLib(std::ostream &str)
{
    if (const int err = mpg123_init(); err != MPG123_OK)
    {
        throw std::runtime_error{"Could not initialize mpg123"};
    }

    str << "mpg123 decoders:";
    for (const char **decoders = mpg123_decoders(); *decoders != nullptr; ++decoders)
    {
        str << "\n - " << *decoders;
    }

    str << "\nmpg123 supported decoders:";
    for (const char **decoders = mpg123_supported_decoders(); *decoders != nullptr; ++decoders)
    {
        str << "\n - " << *decoders;
    }

    str << "\nmpg123 supported rates:";
    const long *rates;
    size_t ratesSize = 0;
    mpg123_rates(&rates, &ratesSize);
    for (size_t i = 0; i < ratesSize; ++i)
    {
        str << "\n - " << rates[i];
    }

    str << "\nmpg123 supported encodings:";
    const int *encodings;
    size_t encodingsSize = 0;
    mpg123_encodings(&encodings, &encodingsSize);
    for (size_t i = 0; i < encodingsSize; ++i)
    {
        str << "\n - " << encodings[i] << " (" << mpg123_encsize(encodings[i]) << "B)";
    }
    str << '\n';
}

void AudioReadMp3::unloadLib()
{
    mpg123_exit();
}

std::unique_ptr<AudioRead> AudioReadMp3::create(FILEUnique &file, const char *extension)
{
    if (file == nullptr || extension == nullptr || strcasecmp(extension, "mp3"))
    {
        return nullptr;
    }

    std::fseek(file.get(), 0, SEEK_SET);

    Impl impl;
    int err;
    impl.handle.reset(mpg123_new(nullptr, &err));
    if (impl.handle == nullptr)
    {
        std::cerr << "Could not create mpg123 handle: " << mpg123_plain_strerror(err) << std::endl;
        return nullptr;
    }

    mpg123_format_none(impl.handle.get());
    if (const int err = mpg123_format(impl.handle.get(), kRate, kChannels, kEncodings); err != MPG123_OK)
    {
        std::cerr << "Could not set mpg123 format: " << mpg123_plain_strerror(err) << std::endl;
        return nullptr;
    }

    if (const int err = mpg123_replace_reader_handle(impl.handle.get(), mp3Read, mp3Seek, nullptr); err != MPG123_OK)
    {
        std::cerr << "Could not replace mpg123 reader handle: " << mpg123_plain_strerror(err) << std::endl;
        return nullptr;
    }

    if (const int err = mpg123_open_handle(impl.handle.get(), file.get()); err != MPG123_OK)
    {
        std::cerr << "Could not open mpg123 handle: " << mpg123_plain_strerror(err) << std::endl;
        return nullptr;
    }

    if (const int err = mpg123_scan(impl.handle.get()); err != MPG123_OK)
    {
        std::cerr << "Could not scan mpg123 handle: " << mpg123_plain_strerror(err) << std::endl;
        return nullptr;
    }

    if (const int err = mpg123_getformat(impl.handle.get(), &impl.rate, &impl.channels, &impl.encoding); err != MPG123_OK)
    {
        std::cerr << "Could not get mpg123 format: " << mpg123_plain_strerror(err) << std::endl;
        return nullptr;
    }

    impl.file = std::move(file);
    return std::make_unique<AudioReadMp3>(std::move(impl));
}

int AudioReadMp3::getChannels() const
{
    return pimpl->channels;
}

uint64_t AudioReadMp3::getSamples() const
{
    const auto length = mpg123_length(pimpl->handle.get());
    if (length > 0)
    {
        return length;
    }
    return 0;
}

int AudioReadMp3::getRate() const
{
    return pimpl->rate;
}

size_t AudioReadMp3::readBuffer(char *buffer, size_t bufferSize, bool loop)
{
    size_t totalRead = 0;
    while (totalRead < bufferSize)
    {
        size_t read;
        if (const int err = mpg123_read(pimpl->handle.get(),
                                        reinterpret_cast<unsigned char *>(buffer + totalRead),
                                        bufferSize - totalRead,
                                        &read);
            err != MPG123_OK || read == 0)
        {
            break;
        }

        totalRead += read;
    }

    if (totalRead < bufferSize && loop)
    {
        // EOF, loop
        mpg123_seek(pimpl->handle.get(), 0, SEEK_SET);
        return totalRead + readBuffer(buffer + totalRead, bufferSize - totalRead, false);
    }

    return totalRead;
}

std::ostream &AudioReadMp3::toStream(std::ostream &str) const
{
    return str << "mpg123 rate=" << getRate() << " channels=" << getChannels() << " encoding=" << pimpl->encoding;
}
