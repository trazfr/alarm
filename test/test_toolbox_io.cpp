#include <gtest/gtest.h>

#include "toolbox_io.hpp"

#include <fcntl.h>

#include <cstring>

namespace
{
const char kFilename[] = "test_toolbox_io.bin";
const char kMissingFilename[] = "/my/non/existing/file/path";
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

TEST_F(TestToolboxIo, readFile_Basic)
{
    const std::string filename = kFilename;
    const std::string content = readFile(filename);

    ASSERT_EQ(sizeof(kContent), content.size());
    EXPECT_FALSE(std::memcmp(kContent, content.data(), content.size()));
}

TEST_F(TestToolboxIo, readFile_Missing)
{
    try
    {
        readFile(kMissingFilename);
        FAIL() << "Should have thrown";
    }
    catch (const std::runtime_error &)
    {
    }
}

TEST_F(TestToolboxIo, mmap_Basic)
{
    const MmapFile mmapped{kFilename};
    ASSERT_EQ(sizeof(kContent), mmapped.size);
    EXPECT_FALSE(std::memcmp(kContent, mmapped.content, mmapped.size));
}

TEST_F(TestToolboxIo, mmap_C_FILE)
{
    const FILEUnique fd{std::fopen(kFilename, "rb")};
    const MmapFile mmapped{fd.get()};
    ASSERT_EQ(sizeof(kContent), mmapped.size);
    EXPECT_FALSE(std::memcmp(kContent, mmapped.content, mmapped.size));
}

TEST_F(TestToolboxIo, mmap_Unix_FILE)
{
    const FileUnix fd{open(kFilename, O_RDONLY)};
    const MmapFile mmapped{fd.fd};
    ASSERT_EQ(sizeof(kContent), mmapped.size);
    EXPECT_FALSE(std::memcmp(kContent, mmapped.content, mmapped.size));
}

TEST_F(TestToolboxIo, mmap_Missing)
{
    try
    {
        MmapFile mmapped{kMissingFilename};
    }
    catch (const std::runtime_error &)
    {
    }
}

TEST_F(TestToolboxIo, mmap_NotMmap)
{
    try
    {
        MmapFile mmapped{"/dev/null"};
    }
    catch (const std::runtime_error &)
    {
    }
}
