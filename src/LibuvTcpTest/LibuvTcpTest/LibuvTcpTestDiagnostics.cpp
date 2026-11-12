#include "LibuvTcpTestDiagnostics.h"
#include <cstdio>
#include "LibuvTcp/LibuvBase.h"

// 仅测试进程的固定诊断存储，不分配内存，不向生产库导出
static LibuvSubmitCounters s_submitCounters[64] = {};
static std::atomic<uint32_t> s_submitCounterCount(0);

LibuvSubmitProfile::LibuvSubmitProfile() :
m_counters(nullptr),
m_previous(0)
{
#ifdef _MSC_VER
    static __declspec(thread) uint32_t s_index = UINT32_MAX;
#else
    static thread_local uint32_t s_index = UINT32_MAX;
#endif
    if (s_index == UINT32_MAX)
    {
        s_index = s_submitCounterCount.fetch_add(1);
    }
    if (s_index < 64)
    {
        m_counters = &s_submitCounters[s_index];
        if ((m_counters->m_calls++ & 1023U) == 0)
        {
            m_previous = uv_hrtime();
        }
    }
}

void LibuvSubmitProfile::mark(uint32_t stage)
{
    if (m_previous != 0 && stage < 6)
    {
        const uint64_t now = uv_hrtime();
        m_counters->m_nanoseconds[stage] += now - m_previous;
        ++m_counters->m_samples[stage];
        m_previous = now;
    }
}

void LibuvSubmitProfile::queued(int32_t result)
{
    if (m_counters != nullptr)
    {
        ++m_counters->m_results[result >= 0 && result < 15 ? result : 15];
    }
}

void LibuvDumpSubmitProfiles()
{
    LibuvSubmitCounters total = {};
    for (uint32_t index = 0; index < 64; ++index)
    {
        const LibuvSubmitCounters& counters = s_submitCounters[index];
        total.m_calls += counters.m_calls;
        for (uint32_t stage = 0; stage < 6; ++stage)
        {
            total.m_samples[stage] += counters.m_samples[stage];
            total.m_nanoseconds[stage] += counters.m_nanoseconds[stage];
        }
        for (uint32_t result = 0; result < 16; ++result)
        {
            total.m_results[result] += counters.m_results[result];
        }
    }
    std::printf("PROFILE calls=%llu stages(ns/count):", static_cast<unsigned long long>(total.m_calls));
    for (uint32_t stage = 0; stage < 6; ++stage)
    {
        std::printf(" %llu/%llu", static_cast<unsigned long long>(total.m_nanoseconds[stage]),
            static_cast<unsigned long long>(total.m_samples[stage]));
    }
    std::printf(" results:");
    for (uint32_t result = 0; result < 16; ++result)
    {
        std::printf(" %llu", static_cast<unsigned long long>(total.m_results[result]));
    }
    std::printf("\n");
}

bool LibuvTestReserve(LibuvBase* state, uint32_t charge)
{
#ifndef LIBUV_TEST_BASELINE
    const bool accepted = state->tryReserveSend(charge, true) == 0;
    if (!accepted)
    {
        std::fprintf(stderr, "reserve rejected charge=%u pendingBytes=%llu\n", charge, static_cast<unsigned long long>(state->m_pendingBytes.load()));
    }
    return accepted;
#else
    (void)state;
    (void)charge;
    return false;
#endif
}

LockFreeQueueResult LibuvTestPush(LibuvBase* state, const LibuvPendingSend& pending)
{
    const LockFreeQueueResult result = state->m_queue->tryPush(pending, true, false);
    if (result != LockFreeQueueSuccess)
    {
        const LockFreeQueuePoolStatistics stats = state->m_queue->poolStatistics();
        std::fprintf(stderr, "push rejected result=%d capacity=%u used=%u free=%u reserved=%u producers=%u\n",
            int(result), uint32_t(stats.currentCapacity), uint32_t(stats.usedNodes),
            uint32_t(stats.reusableFreeCapacity), uint32_t(stats.reservedCapacity), uint32_t(stats.activeProducers));
    }
    return result;
}