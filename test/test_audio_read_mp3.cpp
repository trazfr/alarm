#include <gtest/gtest.h>

#include "audio_read_mp3.hpp"

#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{

constexpr char kFilename[] = "test.mp3";

} // namespace

class TestAudioReadMp3 : public ::testing::Test
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
        AudioReadMp3::loadLib(str);
        ASSERT_FALSE(str.str().empty());

        // generate the test file on the fly
        std::string commandLine = "ffmpeg -y -loglevel quiet -f lavfi -i sine=f=440 -t 1 -c:a libmp3lame ";
        commandLine += kFilename;
        system(commandLine.c_str());
    }

    void TearDown() override
    {
        AudioReadMp3::unloadLib();

        unlink(kFilename);
    }
};

TEST_F(TestAudioReadMp3, wrongExtension)
{
    auto file = createFile();

    const auto audio = AudioReadMp3::create(file, "bin");
    EXPECT_FALSE(audio);
    EXPECT_TRUE(file);
}

TEST_F(TestAudioReadMp3, nullFile)
{
    FILEUnique file;
    const auto audio = AudioReadMp3::create(file, "mp3");
    EXPECT_FALSE(audio);
}

TEST_F(TestAudioReadMp3, devnullFile)
{
    FILEUnique file{std::fopen("/dev/null", "rb")};
    const auto audio = AudioReadMp3::create(file, "mp3");
    EXPECT_FALSE(audio);
}

TEST_F(TestAudioReadMp3, wrongContent)
{
    std::ofstream{kFilename} << "Wrong content";
    auto file = createFile();

    const auto audio = AudioReadMp3::create(file, "mp3");
    EXPECT_FALSE(audio);
    EXPECT_TRUE(file);
}

TEST_F(TestAudioReadMp3, read)
{
    auto file = createFile();
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
