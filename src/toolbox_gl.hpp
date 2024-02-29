#pragma once

/**
 * @file
 *
 * This file is the only file to include the OpenGL headers (GLES2 due to Raspberry PI)
 * It also provides a function to check OpenGL errors
 */

#include "error.hpp"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <iosfwd>

/**
 * Print some info about the OpenGL driver
 */
void glDebug(std::ostream &str);

/**
 * Throw a GLError in case of problem
 */
void glCheckError(const char *func = __builtin_FUNCTION(),
                  const char *file = __builtin_FILE(),
                  int line = __builtin_LINE());

/**
 * @brief Exception with useful OpenGL info
 */
class GLError : public Error
{
public:
    explicit GLError(const char *description,
                     GLenum error = glGetError(),
                     const char *func = __builtin_FUNCTION(),
                     const char *file = __builtin_FILE(),
                     int line = __builtin_LINE());
};
