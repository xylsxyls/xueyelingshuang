#ifndef LOCK_FREE_MAP_BENCHMARK_CLOCK_INL
#define LOCK_FREE_MAP_BENCHMARK_CLOCK_INL

inline uint64_t LockFreeMapBenchmarkClock::nanoseconds()
{
    const uint64_t units = 1000000000;
#ifdef _WIN32
    LARGE_INTEGER frequency, counter;
    if (!QueryPerformanceFrequency(&frequency) || !QueryPerformanceCounter(&counter) ||
        frequency.QuadPart <= 0 || counter.QuadPart < 0)
    {
        return 0;
    }
    const uint64_t divisor = static_cast<uint64_t>(frequency.QuadPart);
    const uint64_t ticks = static_cast<uint64_t>(counter.QuadPart);
    const uint64_t limit = (std::numeric_limits<uint64_t>::max)();
    if (divisor > limit / units || ticks / divisor >= limit / units)
    {
        return 0;
    }
    return (ticks / divisor) * units + (ticks % divisor) * units / divisor + 1;
#else
    const int64_t ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    return ticks < 0 ? 0 : static_cast<uint64_t>(ticks) / (1000000000 / units) + 1;
#endif
}

#endif