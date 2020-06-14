
#include "audio_read_mod.hpp"

#ifndef NO_AUDIO_READ_MOD

#include <gtest/gtest.h>

#include <cstring>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace
{

constexpr char kFilename[] = "test/assets/test.mod";
constexpr char kDummyFilename[] = "test.mod";

} // namespace

class TestAudioReadMod : public ::testing::Test
{
protected:
    FILEUnique createDummyFile(const char *content)
    {
        std::ofstream{kDummyFilename, std::ios_base::out | std::ios_base::trunc} << content;

        FILEUnique result{std::fopen(kDummyFilename, "rb")};
        EXPECT_TRUE(result);
        return result;
    }

    FILEUnique getModFile()
    {
        FILEUnique result{std::fopen(kFilename, "rb")};
        EXPECT_TRUE(result);
        return result;
    }

    void SetUp() override
    {
        std::ostringstream str;
        AudioReadMod::loadLib(str);
        ASSERT_TRUE(str.str().empty());
    }

    void TearDown() override
    {
        AudioReadMod::unloadLib();

        unlink(kDummyFilename);
    }
};

TEST_F(TestAudioReadMod, wrongExtension)
{
    auto file = getModFile();

    const auto audio = AudioReadMod::create(file, "mp3");
    EXPECT_FALSE(audio);
    EXPECT_TRUE(file);
}

TEST_F(TestAudioReadMod, nullFile)
{
    FILEUnique file;
    const auto audio = AudioReadMod::create(file, "mod");
    EXPECT_FALSE(audio);
}

TEST_F(TestAudioReadMod, devnullFile)
{
    FILEUnique file{std::fopen("/dev/null", "rb")};
    const auto audio = AudioReadMod::create(file, "mod");
    EXPECT_FALSE(audio);
}

TEST_F(TestAudioReadMod, wrongContent)
{
    auto file = createDummyFile("Wrong content");

    const auto audio = AudioReadMod::create(file, "mod");
    EXPECT_FALSE(audio);
    EXPECT_TRUE(file);
}

TEST_F(TestAudioReadMod, read)
{
    auto file = getModFile();
    const auto audio = AudioReadMod::create(file, "mod");

    EXPECT_FALSE(file);
    ASSERT_TRUE(audio);

    const int channels = audio->getChannels();
    const int rate = audio->getRate();
    const uint64_t samples = audio->getSamples();

    EXPECT_EQ(2, channels); // libmodplug is configured to output only 2 channels
    EXPECT_EQ(44100, rate);
    EXPECT_EQ(8 * 44100, samples); // is only an estimation (may loop forever)

    std::vector<char> bufferLoop1(channels * samples * 2);
    const size_t readLoop1 = audio->readBuffer(bufferLoop1.data(), bufferLoop1.size(), true);
    EXPECT_EQ(bufferLoop1.size(), readLoop1);

    std::ostringstream str;
    str << *audio;
    EXPECT_FALSE(str.str().empty());
}

#endif // NO_AUDIO_READ_MOD
