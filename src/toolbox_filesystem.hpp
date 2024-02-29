#pragma once

/**
 * @file
 *
 * This file is to provide the compatibility with GCC6 (Ubuntu 18.04) regarding the filesystem.
 * This is the only file to include <filesystem> due to a missing header
 *
 * The namespace "fs" is either std::filesystem or std::experimental::filesystem
 */

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = ::std::filesystem;
#elif __has_include(<experimental/filesystem>)
#include <experimental/filesystem>
namespace fs = ::std::experimental::filesystem;
#else
#error no filesystem
#endif
