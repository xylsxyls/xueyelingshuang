#include "PlatformTest.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

bool PlatformTest::caseNumber(const char*& cursor, unsigned maximum, unsigned& value)
{
    value = 0;
    if (cursor == nullptr || *cursor < '1' || *cursor > '9') { return false; }
    do
    {
        const unsigned digit = static_cast<unsigned>(*cursor++ - '0');
        if (digit > maximum || value > (maximum - digit) / 10) { return false; }
        value = value * 10 + digit;
    } while (*cursor >= '0' && *cursor <= '9');
    return true;
}

int PlatformTest::runCases(int argc, char** argv, size_t count, void* context, CaseInfo info, CaseRun run)
{
    if (argc < 1 || argv == nullptr || count == 0 || info == nullptr || run == nullptr)
    {
        return 2;
    }
    for (int i = 0; i < argc; ++i)
    {
        if (argv[i] == nullptr) { return 2; }
    }
    const bool all = argc == 1 || (argc == 2 && std::strcmp(argv[1], "--all") == 0);
    const bool instant = argc == 2 && std::strcmp(argv[1], "--instant") == 0;
    const bool stress = argc == 2 && std::strcmp(argv[1], "--stress") == 0;
    const bool specified = argc == 3 && std::strcmp(argv[1], "--cases") == 0;
    std::vector<bool> selected(count, false);
    if (specified)
    {
        bool entryStress = false;
        const unsigned maximum = info(context, count - 1, entryStress);
        const char* cursor = argv[2];
        for (;;)
        {
            unsigned first = 0, last = 0;
            if (!caseNumber(cursor, maximum, first)) { return 2; }
            last = first;
            if (*cursor == '-')
            {
                ++cursor;
                if (!caseNumber(cursor, maximum, last) || last < first) { return 2; }
            }
            unsigned matched = 0;
            for (size_t i = 0; i < count; ++i)
            {
                const unsigned id = info(context, i, entryStress);
                if (id >= first && id <= last)
                {
                    selected[i] = true;
                    ++matched;
                }
            }
            if (matched != last - first + 1) { return 2; }
            if (*cursor == '\0') { break; }
            if (*cursor++ != ',') { return 2; }
        }
    }
    else if (!all && !instant && !stress)
    {
        std::fprintf(stderr, "Expected --all, --instant, --stress, or --cases 1,3-4\n");
        return 2;
    }
    unsigned executed = 0, failed = 0;
    for (size_t i = 0; i < count; ++i)
    {
        bool entryStress = false;
        const unsigned id = info(context, i, entryStress);
        if (!(selected[i] || all || (instant && !entryStress) || (stress && entryStress))) { continue; }
        ++executed;
        const int code = run(context, i);
        std::printf("case %u returned %d\n", id, code);
        if (code != 0) { ++failed; }
    }
    if (executed == 0) { std::fprintf(stderr, "No runnable cases on this target\n"); return 2; }
    return failed == 0 ? 0 : 1;
}

double PlatformTest::seconds()
{
#ifdef _WIN32
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return double(counter.QuadPart) / double(frequency.QuadPart);
#else
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

void PlatformTest::runBenchmark(const char* name, const void* context, BenchmarkBatch batch, uint32_t operations)
{
    if (name == nullptr || batch == nullptr || operations == 0)
    {
        throw std::invalid_argument("A benchmark requires a name, batch callback and positive operation count");
    }
    volatile uint64_t observed = 0;
    uint32_t count = 1;
    // 独立执行一次，首次载入编解码器、JIT 编译等一次性成本不参与批量选择。
    observed = batch(context, 1);
    // 用短暂的预热寻找约20毫秒以上的批次，使宿主计时器的粒度远小于批耗时
    for (;;)
    {
        const double start = seconds();
        observed = batch(context, count);
        const double elapsed = seconds() - start;
        if (count >= (1u << 26))
        {
            break;
        }
        if (elapsed >= 0.020)
        {
            // 慢批次再核一次，避免单次系统抢占把稳定在微秒级的操作校准成单发计时。
            const double confirmation = seconds();
            observed = batch(context, count);
            if (seconds() - confirmation >= 0.020)
            {
                break;
            }
        }
        count *= 2;
    }
    double samples[9];
    double sorted[9];
    for (size_t i = 0; i < 9; ++i)
    {
        const double start = seconds();
        observed = batch(context, count);
        samples[i] = (seconds() - start) * 1e9 / (double(count) * operations);
        sorted[i] = samples[i];
    }
    std::sort(sorted, sorted + 9);
    std::printf("[BENCH] %s iterations=%u ops_per_iteration=%u median_ns=%.3f q1_ns=%.3f q3_ns=%.3f samples_ns=",
        name, static_cast<unsigned>(count), static_cast<unsigned>(operations), sorted[4], sorted[2], sorted[6]);
    for (size_t i = 0; i < 9; ++i)
    {
        std::printf("%s%.3f", i == 0 ? "" : ",", samples[i]);
    }
    std::printf(" checksum=%08x%08x\n", static_cast<unsigned>(observed >> 32), static_cast<unsigned>(observed));
}