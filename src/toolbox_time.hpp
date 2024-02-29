#pragma once

/**
 * @file
 *
 * This file is to provide a fixed clock type for the whole program and some helpers
 */

#include <chrono>

using Clock = ::std::chrono::high_resolution_clock;

/**
 * Convert time_t to struct tm (local time)
 */
struct tm getLocalTime(std::time_t time);

/**
 * Convert a time_point to time_t (Unix timestamp)
 */
template <typename Duration>
std::time_t getTimeSinceEpoch(const std::chrono::time_point<Clock, Duration> &time)
{
    return static_cast<std::time_t>(std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count());
}

/**
 * Convert a time_point to struct tm (local time)
 */
template <typename Duration>
struct tm getLocalTime(const std::chrono::time_point<Clock, Duration> &time)
{
    return getLocalTime(getTimeSinceEpoch<Duration>(time));
}
