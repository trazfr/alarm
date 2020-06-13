#include "audio_read_ogg.hpp"

#ifndef NO_AUDIO_READ_OGG

#include <gtest/gtest.h>

#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{

constexpr char kFilename[] = "test.ogg";

} // namespace

class TestAudioReadOgg : public ::testing::Test
{
protected:
    FILEUnique createFile()
    {
        FILEUnique result{std::fopen(kFilename, "rb")};
        EXPECT_TRUE(result);
        return result;
    }

    void SetUp() override
    {
        std::ostringstream str;
        AudioReadOgg::loadLib(str);
        ASSERT_TRUE(str.str().empty());

        // generate the test file on the fly
        std::string commandLine = "ffmpeg -y -loglevel quiet -f lavfi -i sine=f=440 -t 1 -c:a libvorbis ";
        commandLine += kFilename;
        system(commandLine.c_str());
    }

    void TearDown() override
    {
        AudioReadOgg::unloadLib();

        unlink(kFilename);
    }
};

TEST_F(TestAudioReadOgg, wrongExtension)
{
    // there is a "fast" check to know if it is an OGG container, no check on extension
    auto file = createFile();

    const auto audioNullOgg = AudioReadOgg::create(file, "weirdEntension");
    EXPECT_TRUE(audioNullOgg);
    EXPECT_FALSE(file);
}

TEST_F(TestAudioReadOgg, nullFile)
{
    FILEUnique file;
    const auto audioNullOgg = AudioReadOgg::create(file, "ogg");
    EXPECT_FALSE(audioNullOgg);
}

TEST_F(TestAudioReadOgg, wrongContent)
{
    // replace content
    std::ofstream{kFilename} << "dummy content";

    auto file = createFile();
    const auto audioNullOgg = AudioReadOgg::create(file, "ogg");
    EXPECT_FALSE(audioNullOgg);
    EXPECT_TRUE(file);
}

TEST_F(TestAudioReadOgg, read)
{
    auto file = createFile();
    const auto audio = AudioReadOgg::create(file, "ogg");

    EXPECT_FALSE(file);
    ASSERT_TRUE(audio);

    const int channels = audio->getChannels();
    const int rate = audio->getRate();
    const uint64_t samples = audio->getSamples();

    EXPECT_EQ(1, channels);
    EXPECT_EQ(44100, rate);
    EXPECT_EQ(44100, samples);

    // read the full file #1
    std::vector<char> bufferLoop1(channels * samples * 2);
    const size_t readLoop1 = audio->readBuffer(bufferLoop1.data(), bufferLoop1.size(), true);
    EXPECT_EQ(bufferLoop1.size(), readLoop1);

    // read the full file #2
    std::vector<char> bufferLoop2(bufferLoop1.size());
    const size_t readLoop2 = audio->readBuffer(bufferLoop2.data(), bufferLoop2.size(), true);
    EXPECT_EQ(bufferLoop2.size(), readLoop2);

    // the 2 reads should be equal
    EXPECT_EQ(bufferLoop1, bufferLoop2);

    std::ostringstream str;
    str << *audio;
    EXPECT_FALSE(str.str().empty());
}

#endif // NO_AUDIO_READ_OGG
