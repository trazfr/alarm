#include "toolbox_io.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>

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

std::pair<FileUnix, size_t> openFileWithSize(int fd)
{
    FileUnix fdResult{fd};
    size_t resultSize = 0;

    if (fdResult.fd >= 0)
    {
        resultSize = lseek(fdResult.fd, 0, SEEK_END);
        if (resultSize > 0)
        {
            lseek(fdResult.fd, 0, SEEK_SET);
        }
    }

    return std::pair<FileUnix, size_t>{
        std::move(fdResult),
        resultSize,
    };
}

std::pair<FileUnix, size_t> openFileWithSize(const char *filename)
{
    return openFileWithSize(open(filename, O_RDONLY, 0));
}

} // namespace

FileUnix::FileUnix(FileUnix &&other)
{
    std::swap(fd, other.fd);
}

FileUnix &FileUnix::operator=(FileUnix &&other)
{
    std::swap(fd, other.fd);
    return *this;
}

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
    const auto [fdUnix, size] = openFileWithSize(fd);
    if (size == 0)
    {
        throw std::runtime_error{getErrorMessage("Could not read file", filenameDebug)};
    }
    *this = MmapFile{mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fdUnix.fd, 0), size};
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

MmapFile::MmapFile(MmapFile &&other)
{
    std::swap(content, other.content);
    std::swap(size, other.size);
}

MmapFile &MmapFile::operator=(MmapFile &&other)
{
    std::swap(content, other.content);
    std::swap(size, other.size);
    return *this;
}

MmapFile::~MmapFile()
{
    if (content != nullptr && content != MAP_FAILED && size != 0)
    {
        munmap(content, size);
    }
}

size_t copyBuffer(void *dest, size_t destSize, const void *src, size_t srcSize)
{
    const size_t sizeToCopy = std::min(destSize - 1, srcSize);
    std::memcpy(dest, src, sizeToCopy);
    reinterpret_cast<char *>(dest)[sizeToCopy] = '\0';
    return sizeToCopy;
}

size_t readFile(const char *filename, char *buffer, size_t bufferSize)
{
    const FileUnix fd{open(filename, O_RDONLY, 0)};
    if (fd.fd < 0)
    {
        throw std::runtime_error{getErrorMessage("Could not open file", filename)};
    }

    const auto size = read(fd.fd, buffer, bufferSize - 1);
    if (size > 0)
    {
        buffer[size] = '\0';
        return size;
    }
    return 0;
}

std::string readFile(const char *filename)
{
    const auto [fd, size] = openFileWithSize(filename);
    if (fd.fd < 0)
    {
        throw std::runtime_error{getErrorMessage("Could not open file", filename)};
    }

    std::string result(size, '\0');
    const ssize_t readSize = read(fd.fd, result.data(), size);
    const size_t resizeLen = std::max<ssize_t>(readSize, 0);
    if (resizeLen < size)
    {
        result.resize(readSize);
    }
    return result;
}
