#pragma once

#include <memory>

/**
 * @brief Handles the interactions with Alsa to output audio
 */
class Audio
{
public:
    struct Impl;

    explicit Audio(const char *deviceName);
    ~Audio();

    /**
     * Function to be called at each loop to fetch audio from the source
     *
     * @arg If playStream() has been called, decode some audio chuncks and feed Alsa
     * @arg If an error occurred, try to recover
     * @arg Else (not playing) do nothing
     *
     * @return true if the audio is playing or paused
     */
    bool run();

    /**
     * @return true if we are producing sound
     */
    bool isPlaying() const;

    /**
     * Cancel the current stream and load a new one
     *
     * @return true in case of success
     */
    bool loadStream(const char *filename);

    /**
     * Play stream in loop.
     *
     * If the audio is just pause, resume
     */
    void playStream();
    /**
     * Stop the streaming and reset the audio
     */
    void stopStream();

    /**
     * Pause the stream, keeping the buffers at their state, allowing to resume with playStream()
     */
    void pauseStream();

private:
    std::unique_ptr<Impl> pimpl;
};
