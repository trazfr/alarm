#include "audio.hpp"

#include "audio_read.hpp"
#include "audio_read_mod.hpp"
#include "audio_read_mp3.hpp"
#include "audio_read_ogg.hpp"
#include "error.hpp"
#include "toolbox_io.hpp"

#include <asoundlib.h>

#include <cstring>
#include <iostream>

namespace
{
constexpr snd_pcm_format_t kPcmFormatFormat = SND_PCM_FORMAT_S16; // CPU endian

constexpr int64_t kChannels = 2;
constexpr int64_t kBufferReadSizeBytes = sizeof(int16_t) * kChannels * 4096;

constexpr int64_t kRate = 44100;
constexpr int64_t kBufferTimeUs = 1000 * 1000;
constexpr int64_t kBufferSamples = kChannels * kRate * kBufferTimeUs / (1000 * 1000);
constexpr int64_t kAlsaBufferSamples = kBufferSamples * 2;

struct AudioEnd;

/**
 * This class is used to initialize the AudioRead* classes and run open the audio files in a recursive way computed a compile time
 */
template <typename... T>
struct AudioFormats;

/**
 * End of recursion
 */
template <>
struct AudioFormats<AudioEnd>
{
    static void loadLib() {}
    static void unloadLib() {}
    static std::unique_ptr<AudioRead> create(FILEUnique &, const char *) { return nullptr; }
};

/**
 * Load/unload lib and perform recursion to the next class
 */
template <typename T, typename... Tn>
struct AudioFormats<T, Tn...>
{
    static void loadLib()
    {
        T::loadLib(std::cerr);
        AudioFormats<Tn...>::loadLib();
    }
    static void unloadLib()
    {
        AudioFormats<Tn...>::unloadLib();
        T::unloadLib();
    }
    static std::unique_ptr<AudioRead> create(FILEUnique &file, const char *extension)
    {
        std::unique_ptr<AudioRead> result = T::create(file, extension);
        if (result == nullptr)
        {
            result = AudioFormats<Tn...>::create(file, extension);
        }
        return result;
    }
};

/**
 * Definition of the recursions (OGG, then MOD, then MP3)
 */
using AllFormats = AudioFormats<
#ifndef NO_AUDIO_READ_OGG
    AudioReadOgg,
#endif
#ifndef NO_AUDIO_READ_MOD
    AudioReadMod,
#endif
#ifndef NO_AUDIO_READ_MP3
    AudioReadMp3,
#endif
    AudioEnd>;

/**
 * Internal exception which gives some debug from ALSA
 */
class AlsaError : public Error
{
public:
    explicit AlsaError(const char *description,
                       int error,
                       const char *func = __builtin_FUNCTION(),
                       const char *file = __builtin_FILE(),
                       int line = __builtin_LINE())
        : Error{description + std::string{". Alsa Error: "} + snd_strerror(error), func, file, line}
    {
    }
};

/**
 * Functor to delete the ALSA handler
 * 
 * @sa AlsaUnique
 */
struct AlsaDeleter
{
    void operator()(snd_pcm_t *obj) const
    {
        std::cerr << "Destroy Alsa" << std::endl;
        snd_pcm_drop(obj);
        snd_pcm_close(obj);
    }
};

/**
 * std::unique_ptr<> which destroys the ALSA handler in destructor
 */
template <typename T>
using AlsaUnique = std::unique_ptr<T, AlsaDeleter>;

/**
 * Fetch PCM frames from audio_read_* and feed Alsa
 */
void readMusic(snd_pcm_t *handle, AudioRead &audio)
{
    snd_pcm_sframes_t avail = snd_pcm_avail_update(handle);
    while (avail >= kBufferReadSizeBytes)
    {
        char buf[kBufferReadSizeBytes];
        const size_t readBytes = audio.readBuffer(buf, sizeof(buf), true);
        if (readBytes == 0)
        {
            return;
        }

        const snd_pcm_sframes_t frames = readBytes / snd_pcm_frames_to_bytes(handle, 1);
        if (const snd_pcm_sframes_t written = snd_pcm_writei(handle, buf, frames); written != frames)
        {
            if (const int err = snd_pcm_recover(handle, written >= 0 ? -EPIPE : written, 1); err < 0)
            {
                throw AlsaError{"Write error", err};
            }
        }
        avail -= readBytes;
    }
}

} // namespace

struct Audio::Impl
{
    AlsaUnique<snd_pcm_t> handle;
    std::unique_ptr<AudioRead> music;
};

Audio::Audio(const char *deviceName)
    : pimpl{std::make_unique<Impl>()}
{
    AllFormats::loadLib();

    {
        std::cerr << "Alsa: open audio device " << deviceName << std::endl;
        snd_pcm_t *handle;
        if (const int err = snd_pcm_open(&handle, deviceName, SND_PCM_STREAM_PLAYBACK, 0); err < 0)
        {
            throw AlsaError{"Cannot open audio device", err};
        }
        pimpl->handle.reset(handle);
    }

    snd_pcm_hw_params_t *hwParams;
    snd_pcm_hw_params_alloca(&hwParams);

    if (const int err = snd_pcm_hw_params_any(pimpl->handle.get(), hwParams); err < 0)
    {
        throw AlsaError{"Cannot initialize hardware parameter structure", err};
    }

    if (const int err = snd_pcm_hw_params_set_access(pimpl->handle.get(), hwParams, SND_PCM_ACCESS_RW_INTERLEAVED); err < 0)
    {
        throw AlsaError{"Cannot set access type", err};
    }

    if (const int err = snd_pcm_hw_params_set_format(pimpl->handle.get(), hwParams, kPcmFormatFormat); err < 0)
    {
        throw AlsaError{"Cannot set sample format", err};
    }

    unsigned int rate = kRate;
    if (const int err = snd_pcm_hw_params_set_rate_near(pimpl->handle.get(), hwParams, &rate, 0); err < 0)
    {
        throw AlsaError{"Cannot set sample rate", err};
    }

    if (const int err = snd_pcm_hw_params_set_channels(pimpl->handle.get(), hwParams, kChannels); err < 0)
    {
        throw AlsaError{"Cannot set channel count", err};
    }

    snd_pcm_uframes_t bufferFrames = kAlsaBufferSamples;
    if (const int err = snd_pcm_hw_params_set_buffer_size_near(pimpl->handle.get(), hwParams, &bufferFrames); err < 0)
    {
        throw AlsaError{"Cannot set buffer size", err};
    }

    if (const int err = snd_pcm_hw_params(pimpl->handle.get(), hwParams); err < 0)
    {
        throw AlsaError{"Cannot set parameters", err};
    }

    snd_pcm_hw_params_current(pimpl->handle.get(), hwParams);

    int dir = 0;

    unsigned int channels = 0;
    snd_pcm_hw_params_get_channels(hwParams, &channels);

    rate = 0;
    snd_pcm_hw_params_get_rate(hwParams, &rate, &dir);

    snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;
    snd_pcm_hw_params_get_format(hwParams, &format);

    bufferFrames = 0;
    snd_pcm_hw_params_get_buffer_size(hwParams, &bufferFrames);

    std::cerr << "Alsa: channels=" << channels << " rate=" << rate << " format=" << snd_pcm_format_name(format) << " buffer=" << bufferFrames << std::endl;
    std::cerr << "Alsa PCM name: " << snd_pcm_name(pimpl->handle.get()) << std::endl;
    std::cerr << "Alsa PCM state: " << snd_pcm_state_name(snd_pcm_state(pimpl->handle.get())) << std::endl;
}

Audio::~Audio()
{
    AllFormats::unloadLib();
}

bool Audio::loadStream(const char *filename)
{
    FILEUnique file{std::fopen(filename, "rb")};
    if (file == nullptr)
    {
        return false;
    }

    const char *extension = std::strrchr(filename, '.');
    if (extension)
    {
        ++extension;
    }
    if (auto music = AllFormats::create(file, extension))
    {
        pimpl->music = std::move(music);
        return true;
    }

    return false;
}

bool Audio::run()
{
    if (const snd_pcm_state_t state = snd_pcm_state(pimpl->handle.get());
        state == SND_PCM_STATE_RUNNING || state == SND_PCM_STATE_PAUSED)
    {
        readMusic(pimpl->handle.get(), *pimpl->music);
        return true;
    }
    else if (state == SND_PCM_STATE_XRUN)
    {
        stopStream();
    }
    else if (state != SND_PCM_STATE_PREPARED)
    {
        std::cerr << "ALSA PCM state: " << snd_pcm_state_name(state) << std::endl;
    }
    return false;
}

bool Audio::isPlaying() const
{
    return snd_pcm_state(pimpl->handle.get()) == SND_PCM_STATE_RUNNING;
}

void Audio::playStream()
{
    switch (snd_pcm_state(pimpl->handle.get()))
    {
    case SND_PCM_STATE_RUNNING:
        break;

    case SND_PCM_STATE_PAUSED:
        snd_pcm_pause(pimpl->handle.get(), 0);
        break;

    default:
        if (pimpl->music)
        {
            snd_pcm_start(pimpl->handle.get());
            readMusic(pimpl->handle.get(), *pimpl->music);
        }
        break;
    }
}

void Audio::stopStream()
{
    switch (snd_pcm_state(pimpl->handle.get()))
    {
    case SND_PCM_STATE_XRUN:
        if (const int err = snd_pcm_recover(pimpl->handle.get(), -EPIPE, 1); err < 0)
        {
            throw AlsaError{"Write error", err};
        }
        [[fallthrough]];

    case SND_PCM_STATE_RUNNING:
    case SND_PCM_STATE_PAUSED:
        snd_pcm_drop(pimpl->handle.get());
        snd_pcm_prepare(pimpl->handle.get());
        break;

    default:
        break;
    }
    pimpl->music.reset();
}

void Audio::pauseStream()
{
    if (snd_pcm_state(pimpl->handle.get()) == SND_PCM_STATE_RUNNING)
    {
        snd_pcm_pause(pimpl->handle.get(), 1);
    }
}
