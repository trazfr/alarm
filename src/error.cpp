#include "error.hpp"

Error::Error(std::string description, const char *func, const char *file, int line)
    : std::runtime_error{description},
      function{func},
      file{file},
      line{line}
{
}
