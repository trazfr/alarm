#pragma once

#include "audio_read.hpp"
#include "toolbox_io.hpp"

#include <memory>

/**
 * @brief Read Ogg Vorbis audio files
 * 
 * @attention this does not support VBR, it is only for basic audio
 */
class AudioReadOgg : public AudioRead
{
public:
    struct Impl;

    /**
     * This method cannot be called from outside. Create with create() method instead
     */
    explicit AudioReadOgg(std::unique_ptr<Impl> impl);
    ~AudioReadOgg() override;

    // no static library initialization / cleanup
    static void loadLib(std::ostream &) {}
    static void unloadLib() {}

    /**
     * Create a AudioReadOgg object if the file is of right format
     * 
     * In case of success, takes the ownership of file
     * 
     * @return a valid unique_ptr in case of success, nullptr otherwise (this is not an OGG/Vorbis file for instance)
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
