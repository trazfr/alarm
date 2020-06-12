#include <gtest/gtest.h>

#include "audio_read_mp3.hpp"
#include "audio_read_ogg.hpp"

#include <cstring>
#include <sstream>
#include <stdexcept>

class TestAudioReadMp3Ogg : public ::testing::Test
{
protected:
    FILEUnique createFile(const std::string &extension)
    {
        const std::string filename = "test." + extension;
        return FILEUnique{std::fopen(filename.c_str(), "rb")};
    }

    void SetUp() override
    {
        {
            std::ostringstream str;
            AudioReadMp3::loadLib(str);
            ASSERT_FALSE(str.str().empty());
        }
        {
            std::ostringstream str;
            AudioReadOgg::loadLib(str);
            ASSERT_TRUE(str.str().empty());
        }

        system("ffmpeg -y -loglevel quiet -f lavfi -i sine=f=440 -t 1 -c:a libmp3lame test.mp3");
        system("ffmpeg -y -loglevel quiet -f lavfi -i sine=f=440 -t 1 -c:a libvorbis test.ogg");
    }

    void TearDown() override
    {
        AudioReadMp3::unloadLib();
        AudioReadOgg::unloadLib();

        unlink("test.mp3");
        unlink("test.ogg");
    }
};

TEST_F(TestAudioReadMp3Ogg, wrongInput)
{
    auto ogg = createFile("ogg");
    EXPECT_TRUE(ogg);
    const auto audioNullMp3 = AudioReadMp3::create(ogg, "ogg");
    EXPECT_FALSE(audioNullMp3);
    EXPECT_TRUE(ogg);

    auto mp3 = createFile("mp3");
    EXPECT_TRUE(mp3);
    const auto audioNullOgg = AudioReadOgg::create(mp3, "mp3");
    EXPECT_FALSE(audioNullOgg);
    EXPECT_TRUE(mp3);
}

TEST_F(TestAudioReadMp3Ogg, mp3)
{
    auto file = createFile("mp3");
    EXPECT_TRUE(file);

    const auto audio = AudioReadMp3::create(file, "mp3");

    EXPECT_FALSE(file);
    ASSERT_TRUE(audio);

    const int channels = audio->getChannels();
    const int rate = audio->getRate();
    const uint64_t samples = audio->getSamples();

    EXPECT_EQ(2, channels); // libmpg123 is configured to output only 2 channels
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

TEST_F(TestAudioReadMp3Ogg, ogg)
{
    auto file = createFile("ogg");
    EXPECT_TRUE(file);

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
