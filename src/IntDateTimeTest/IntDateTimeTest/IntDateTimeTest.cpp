#include "../../../common/PlatformTest/LibraryCaseSelection.h"
#include "IntDateTimeTest.h"
#include "../../../common/PlatformTest/LibraryBenchmark.h"
#include <cstring>
#include <cstdio>
#include "IntDateTime/IntDateTimeAPI.h"
#include <atomic>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

/** 对本库的一项固定规模操作连续采样，结果用于同平台的改动前后比较。 */
static void RunBenchmark()
{
    IntDateTime when(0, 0);
    LibraryBenchmark::run("IntDateTime", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t index = 0; index < count; ++index)
        {
            when.setTime(static_cast<time_t>(1704067200 + index));
            checksum += when.getTime();
        }
        return checksum;
    });
}

/** 核对值对象的日期、边界与运算。 */
static int TestValues()
{
    int32_t totalCount = 0;
    int32_t failCount = 0;
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
        if (!ok)
        {
            ++failCount;
        }
    };

    const IntDateTime ancient(-10203, 0);
    check(ancient.getYear() == -1 && ancient.getMonth() == 2 && ancient.getDay() == 3, "signed date fields");
    check(ancient.dateToString() == "-1-02-03" && ancient.dateNumToString() == "-10203" &&
        ancient.toString() == "-1-02-03 00:00:00", "signed date displays positive month and day");
    check(IntDateTime(ancient.toString()) == ancient, "signed date round trips");
    IntDateTime parseTime("2026-08-09 03:41:22");
    check(parseTime.getDate() == 20260809 && parseTime.getTime() == 34122000, "IntDateTime parse datetime");
    check(parseTime.getYear() == 2026 && parseTime.getMonth() == 8 && parseTime.getDay() == 9, "IntDateTime date fields");
    check(parseTime.getHour() == 3 && parseTime.getMin() == 41 && parseTime.getSeconds() == 22, "IntDateTime time fields");
    check(parseTime.toString() == "2026-08-09 03:41:22", "IntDateTime toString");
    check(parseTime.dateToString() == "2026-08-09", "IntDateTime dateToString");
    check(parseTime.dateNumToString() == "20260809", "IntDateTime dateNumToString");
    check(parseTime.timeToString() == "03:41:22", "IntDateTime timeToString");
    IntDateTime dateOnly("2026-08-09");
    check(dateOnly.getDate() == 20260809 && dateOnly.getTime() == 0, "IntDateTime parse date only");
    IntDateTime copied(parseTime);
    check(copied == parseTime, "IntDateTime copy constructor");
    copied.clear();
    check(copied.empty() && copied == 0, "IntDateTime clear empty");
    copied.setTime(20260810, 101112000);
    check(copied.getDate() == 20260810 && copied.getTime() == 101112000, "IntDateTime setTime int");
    copied.setTime("2026-08-11 12:13:14");
    check(copied.toString() == "2026-08-11 12:13:14", "IntDateTime setTime string");
    IntDateTime preserved("2026-08-09 01:02:03");
    preserved.setTime("2024-01-01 2147484");
    check(preserved.getDate() == 20260809 && preserved.getTime() == 10203000, "out-of-range milliseconds preserve date and time");
    preserved.setTime("999999999999999999999999 00:00:00");
    check(preserved.getDate() == 20260809 && preserved.getTime() == 10203000, "out-of-range date preserves earlier value");
    const IntDateTime emptyDate(0, 0);
    check((emptyDate + 7).empty() && (13 - 7 + emptyDate).empty(), "empty date arithmetic stays empty");
#if defined(_WIN32) && defined(_USE_32BIT_TIME_T)
    // 此 CRT 只接受 1970 到 2038，整个有效域不足以构成饱和秒差。
    const IntDateTime last("2037-01-02");
    check(last - IntDateTime("2037-01-01") == 86400 &&
        IntDateTime("2080-01-02") - last == 0, "32-bit Windows time conversion limits");
#else
    const IntDateTime early(sizeof(time_t) >= 8 ? "2000-01-02" : "1903-01-02");
    const IntDateTime late(sizeof(time_t) >= 8 ? "2080-01-02" : "2037-01-02");
    check(late - early == INT32_MAX && early - late == INT32_MIN, "date difference saturates at both limits");
#endif
    const IntDateTime leap("2024-02-29 23:59:59");
    const IntDateTime march("2024-03-01 00:00:00");
    check(leap + 1 == march && 1 + leap == march && march - 1 == leap && march - leap == 1,
        "calendar arithmetic crosses leap-day midnight in both directions");
    check(leap <= march && march >= leap && leap != march && leap <= leap && !(leap > leap),
        "inclusive comparisons and equality boundaries");
    IntDateTime assigned(0, 0);
    assigned = std::string("2024-02-29 23:59:59");
    check(assigned == leap, "text assignment");
    assigned = march;
    check(assigned == march, "object assignment");
    IntDateTime nullText(static_cast<const char*>(nullptr));
    check(nullText.empty(), "IntDateTime null char pointer safe");
    IntDateTime fromEpoch(static_cast<time_t>(0));
    check(!fromEpoch.empty(), "IntDateTime time_t constructor safe");
    check(IntDateTime("2026-08-09 03:41:23") > parseTime, "IntDateTime compare greater");
    check(IntDateTime("2026-08-09 03:41:21") < parseTime, "IntDateTime compare less");
    std::cout << "IntDateTime test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << std::endl;
    return failCount == 0 ? 0 : 1;
}

#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
/** 并行执行转换并核对所有线程的结果。 */
static int TestThreads()
{
    std::atomic<int32_t> failedThreadCount(0);
    std::vector<std::thread> threads;
    for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
    {
        struct tm expected = {};
        expected.tm_year = 124;
        expected.tm_mon = 1;
        expected.tm_mday = threadIndex + 1;
        expected.tm_hour = 12;
        expected.tm_isdst = -1;
        const time_t stamp = mktime(&expected);
        if (stamp == static_cast<time_t>(-1))
        {
            failedThreadCount.fetch_add(1);
            continue;
        }
        threads.push_back(std::thread([&failedThreadCount, stamp, threadIndex]() -> void
        {
            for (int32_t loopIndex = 0; loopIndex < 5000; ++loopIndex)
            {
                IntDateTime value("2026-08-09 03:41:22");
                IntDateTime converted(0, 0);
                converted = stamp;
                if (value.getDate() != 20260809 || value.timeToString() != "03:41:22" ||
                    converted.getDate() != 20240201 + threadIndex || converted.getTime() != 120000000)
                {
                    failedThreadCount.fetch_add(1);
                    return;
                }
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    return failedThreadCount.load() == 0 ? 0 : 1;
}
#endif

/** 按稳定功能用例号或显式性能参数运行。 */
int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        RunBenchmark();
        return 0;
    }
    const LibraryTestCase cases[] = {
        {1, false, TestValues},
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
        {2, true, TestThreads},
#endif
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}