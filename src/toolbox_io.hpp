#pragma once

/**
 * @file
 *
 * This file is to provide some general tools related to input-output
 */

#include <cstdio>
#include <memory>
#include <string>

/**
 * @brief Functor to close a C-like FILE
 */
struct CloseFile
{
    void operator()(FILE *file) const
    {
        std::fclose(file);
    }
};
/**
 * std::unique_ptr<> to close a C-like FILE at destruction time
 *
 * @sa CloseFile
 */
using FILEUnique = std::unique_ptr<FILE, CloseFile>;

/**
 * @brief Structure to close a Unix-like file destructor at destruction time
 */
struct FileUnix
{
    explicit FileUnix(int fd = -1) : fd{fd} {}

    explicit FileUnix(FileUnix &&other);
    FileUnix &operator=(FileUnix &&other);

    ~FileUnix();
    int fd;
};

/**
 * @brief Memory mapping of a file
 */
class MmapFile
{
public:
    MmapFile() = default;

    /**
     * Open by filename (read only)
     */
    explicit MmapFile(const char *filename);
    /**
     * Mmap an already open C-like FILE (read only)
     */
    explicit MmapFile(FILE *file);
    /**
     * Mmap an already open Unix file descriptor (read only)
     */
    explicit MmapFile(int fd);

    /**
     * Mmap an already mmapped file
     */
    MmapFile(void *content, size_t size);

    explicit MmapFile(MmapFile &&other);
    MmapFile &operator=(MmapFile &&other);

    ~MmapFile();

    void *content = nullptr;
    size_t size = 0;

private:
    MmapFile(int fd, const char *filenameDebug);
};

size_t copyBuffer(void *dest, size_t destSize, const void *src, size_t srcSize);
template <typename T, size_t S>
inline size_t copyBuffer(std::array<T, S> &dest, std::string_view src)
{
    return copyBuffer(dest.data(), sizeof(dest), src.data(), src.size());
}

/**
 * Read max bufferSize-1 from the filename.
 * The resulting string is always null-terminated
 *
 * @return the number of bytes (not counting the last '\0')
 */
size_t readFile(const char *filename, char *buffer, size_t bufferSize);

/**
 * Read the whole content of a file
 */
std::string readFile(const char *filename);
inline std::string readFile(const std::string &filename)
{
    return readFile(filename.c_str());
}
