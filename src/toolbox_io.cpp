#include "toolbox_io.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace
{

std::string getErrorMessage(std::string message, const char *filenameDebug)
{
    if (filenameDebug != nullptr)
    {
        message += ' ';
        message += filenameDebug;
    }
    return message;
}

} // namespace

FileUnix::~FileUnix()
{
    if (fd >= 0)
    {
        close(fd);
    }
}

MmapFile::MmapFile(const char *filename)
    : MmapFile{FileUnix{open(filename, O_RDONLY)}.fd, filename}
{
}

MmapFile::MmapFile(FILE *file)
    : MmapFile{fileno(file), nullptr}
{
}

MmapFile::MmapFile(int fd)
    : MmapFile{fd, nullptr}
{
}

MmapFile::MmapFile(int fd, const char *filenameDebug)
{
    if (fd < 0)
    {
        throw std::runtime_error{getErrorMessage("Could not open file", filenameDebug)};
    }
    const auto fileSize = lseek(fd, 0, SEEK_END);
    if (fileSize < 0)
    {
        throw std::runtime_error{getErrorMessage("Could not determine file size", filenameDebug)};
    }
    lseek(fd, 0, SEEK_SET);

    MmapFile result = {mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0),
                       static_cast<size_t>(fileSize)};
    std::swap(*this, result);
}

MmapFile::MmapFile(void *content, size_t size)
    : content{content},
      size{size}
{
    if (content == nullptr || content == MAP_FAILED)
    {
        throw std::runtime_error{"Could not mmap the file"};
    }
}

MmapFile::~MmapFile()
{
    if (content != nullptr && content != MAP_FAILED && size != 0)
    {
        munmap(content, size);
    }
}

std::string readFile(const char *filename)
{
    const MmapFile mmapped{filename};
    return {reinterpret_cast<const char *>(mmapped.content), mmapped.size};
}
