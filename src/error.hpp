#pragma once

#include <stdexcept>

/**
 * @brief base class for exceptions with details about where the exception happened
 */
class Error : public std::runtime_error
{
public:
    const char *const function;
    const char *const file;
    const int line;

protected:
    Error(std::string description, const char *func, const char *file, int line);
};
