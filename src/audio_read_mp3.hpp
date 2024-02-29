#pragma once

#ifndef NO_AUDIO_READ_MP3

#include "audio_read.hpp"
#include "toolbox_io.hpp"

#include <memory>

/**
 * @brief Read MP3 audio files
 *
 * The patent is expired, so it is safe to use it
 */
class AudioReadMp3 : public AudioRead
{
public:
    struct Impl;

    /**
     * This method cannot be called from outside. Create with create() method instead
     */
    explicit AudioReadMp3(Impl impl);
    ~AudioReadMp3() override;

    /**
     * library initialization
     */
    static void loadLib(std::ostream &str);

    /**
     * library cleanup
     */
    static void unloadLib();

    /**
     * Create a AudioReadMp3 object if the file is of right format
     *
     * In case of success, takes the ownership of file
     *
     * @return a valid unique_ptr in case of success, nullptr otherwise (this is not an MP3 for instance)
     */
    static std::unique_ptr<AudioRead> create(FILEUnique &file, const char *extension);

    int getChannels() const override;
    uint64_t getSamples() const override;
    int getRate() const override;

    size_t readBuffer(char *buffer, size_t bufferSize, bool loop) override;

private:
    std::ostream &toStream(std::ostream &str) const override;

    std::unique_ptr<Impl> pimpl;
};

#endif // NO_AUDIO_READ_MP3
