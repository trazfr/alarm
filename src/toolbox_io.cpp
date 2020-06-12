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
    lseek(fd, 0, SEEK_SET);

    void *const mapped = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped == MAP_FAILED)
    {
        throw std::runtime_error{getErrorMessage("Could not mmap the file", filenameDebug)};
    }

    content = mapped;
    size = fileSize;
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
