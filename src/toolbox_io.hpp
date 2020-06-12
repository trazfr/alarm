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
    ~FileUnix();
    int fd = -1;
};

/**
 * @brief Memory mapping of a file
 */
class MmapFile
{
public:
    MmapFile() = default;
    /**
     * Open by filename
     */
    explicit MmapFile(const char *filename);
    /**
     * Mmap an already open C-like FILE
     */
    explicit MmapFile(FILE *file);
    /**
     * Mmap an already open Unix file descriptor
     */
    explicit MmapFile(int fd);
    ~MmapFile();

    void *content = nullptr;
    size_t size = 0;

private:
    MmapFile(int fd, const char *filenameDebug);
};

/**
 * Read the whole content of a file
 */
std::string readFile(const char *filename);
inline std::string readFile(const std::string &filename)
{
    return readFile(filename.c_str());
}
