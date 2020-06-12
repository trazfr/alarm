#pragma once

#include <cstdint>
#include <iosfwd>

/**
 * @brief Base class to decode audio files
 */
class AudioRead
{
public:
    virtual ~AudioRead() = 0;

    /**
     * Number of audio channels in the audio stream (usually 2)
     */
    virtual int getChannels() const = 0;

    /**
     * Number of audio samples in the audio stream.
     * The duration is getRate() * getSamples()
     */
    virtual uint64_t getSamples() const = 0;

    /**
     * Sample rate in the audio stream (should be 48000, 44100, 22050...)
     */
    virtual int getRate() const = 0;

    /**
     * Fetch an Audio buffer.
     * 
     * @return 0 in case of error (this can be EOF with loop=false)
     */
    virtual size_t readBuffer(char *buffer, size_t bufferSize, bool loop) = 0;

    friend std::ostream &operator<<(std::ostream &str, const AudioRead &obj)
    {
        return obj.toStream(str);
    }

private:
    virtual std::ostream &toStream(std::ostream &str) const = 0;
};
