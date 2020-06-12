#include "toolbox_time.hpp"

struct tm getLocalTime(std::time_t time)
{
    struct tm result = {};
    localtime_r(&time, &result);
    return result;
}
