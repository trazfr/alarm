#include <gtest/gtest.h>

#include "toolbox_time.hpp"

#include <cstdlib>

namespace
{
constexpr char kTimezoneVar[] = "TZ";
} // namespace

class TestToolboxTime : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if (const auto var = getenv(kTimezoneVar))
        {
            tzBackup.emplace(var);
        }
    }

    void TearDown() override
    {
        if (tzBackup)
        {
            setenv(kTimezoneVar, tzBackup->c_str(), true);
        }
        else
        {
            unsetenv(kTimezoneVar);
        }
        tzset();
    }

    std::optional<std::string> tzBackup;
};

TEST_F(TestToolboxTime, cppClockToUnixTimestamp)
{
    const auto unixNow = time(nullptr);
    const auto cppNow = Clock::now();

    const auto result = getTimeSinceEpoch(cppNow);

    // the can be 1s difference if we are just about to change the second between time() and Clock::now()
    EXPECT_TRUE(result - unixNow <= 1);
}

TEST_F(TestToolboxTime, localtime)
{
    setenv(kTimezoneVar, "UTC-2", true); // man 3 timezone, this is actually UTC+2, thanks POSIX

    const Clock::time_point fixedCppTime = Clock::from_time_t(1591257960); // 4 June 2020 @ 08:06:00 UTC
    const struct tm localFixedTime = getLocalTime(fixedCppTime);

    EXPECT_EQ(120, localFixedTime.tm_year);
    EXPECT_EQ(5, localFixedTime.tm_mon);
    EXPECT_EQ(4, localFixedTime.tm_mday);
    EXPECT_EQ(10, localFixedTime.tm_hour);
    EXPECT_EQ(6, localFixedTime.tm_min);
    EXPECT_EQ(0, localFixedTime.tm_sec);
}
