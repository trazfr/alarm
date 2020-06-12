#include <gtest/gtest.h>

#include "toolbox_io.hpp"

#include <cstring>

namespace
{
const char kFilename[] = "test_toolbox_io.bin";
const char kContent[] = "MyFileContent\0 some content";
} // namespace

class TestToolboxIo : public ::testing::Test
{
protected:
    void SetUp() override
    {
        FILEUnique fd{std::fopen(kFilename, "wb+")};
        ASSERT_TRUE(fd);
        std::fwrite(kContent, 1, sizeof(kContent), fd.get());
    }

    void TearDown() override
    {
        unlink(kFilename);
    }
};

TEST_F(TestToolboxIo, Basic)
{
    const std::string filename = kFilename;
    const std::string content = readFile(filename);

    ASSERT_EQ(sizeof(kContent), content.size());
    EXPECT_FALSE(std::memcmp(kContent, content.data(), content.size()));
}
