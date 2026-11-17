#ifndef LOCK_FREE_QUEUE_BENCHMARK_AFFINITY_INL
#define LOCK_FREE_QUEUE_BENCHMARK_AFFINITY_INL

#include <cstdlib>
#include <iostream>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

inline bool LockFreeQueueBenchmarkAffinity::pin(size_t participant, bool announce)
{
#if defined(_WIN32)
    char plan[1024] = {};
    SetLastError(ERROR_SUCCESS);
    const DWORD length = GetEnvironmentVariableA("LOCK_FREE_QUEUE_TEST_CPU_PLAN", plan, sizeof(plan));
    if (length == 0)
    {
        return GetLastError() == ERROR_ENVVAR_NOT_FOUND || GetLastError() == ERROR_SUCCESS;
    }
    if (length >= sizeof(plan))
    {
        return false;
    }
    DWORD_PTR processMask = 0, systemMask = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask))
    {
        return false;
    }
    DWORD_PTR seen = 0, selected = 0;
    size_t index = 0;
    unsigned int selectedCpu = 0;
    const char* next = plan;
    for (;;)
    {
        if (*next < '0' || *next > '9')
        {
            return false;
        }
        unsigned int cpu = 0;
        do
        {
            cpu = cpu * 10 + static_cast<unsigned int>(*next - '0');
            if (cpu >= sizeof(DWORD_PTR) * 8)
            {
                return false;
            }
            ++next;
        } while (*next >= '0' && *next <= '9');
        const DWORD_PTR bit = DWORD_PTR(1) << cpu;
        if ((seen & bit) != 0 || (processMask & bit) == 0)
        {
            return false;
        }
        seen |= bit;
        if (index == participant)
        {
            selected = bit;
            selectedCpu = cpu;
        }
        ++index;
        if (*next == '\0')
        {
            break;
        }
        if (*next++ != ',')
        {
            return false;
        }
    }
    if (selected == 0 || SetThreadAffinityMask(GetCurrentThread(), selected) == 0 ||
        GetCurrentProcessorNumber() != selectedCpu)
    {
        return false;
    }
    if (announce)
    {
        std::cout << "AFFINITY plan=" << plan << " consumer=" << selectedCpu << " valid=1" << std::endl;
    }
    return true;
#else
    const char* plan = std::getenv("LOCK_FREE_QUEUE_TEST_CPU_PLAN");
    (void)participant;
    (void)announce;
    return plan == nullptr || *plan == '\0';
#endif
}

#endif