#include "LockFreeMapArrivalTest.h"
#include "LockFreeMapBenchmarkClock.h"
#include "LockFreeMapTest.h"
#include "LockFreeMapTestThread.h"
#include <algorithm>
#include <atomic>
#include <limits>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#endif

LockFreeMapArrivalWorker::LockFreeMapArrivalWorker() :
m_completed(0),
m_cpuNanoseconds((std::numeric_limits<uint64_t>::max)()),
m_maxCallNanoseconds(0),
m_firstCallNanoseconds((std::numeric_limits<uint64_t>::max)()),
m_lastReturnNanoseconds(0),
m_processorGroup((std::numeric_limits<unsigned>::max)()),
m_allowedProcessors((std::numeric_limits<uint64_t>::max)()),
m_finalProcessorGroup((std::numeric_limits<unsigned>::max)()),
m_finalAllowedProcessors((std::numeric_limits<uint64_t>::max)()),
m_errors(0)
{
    std::fill(m_attempts, m_attempts + 4, UINT64_C(0));
    std::fill(m_successes, m_successes + 4, UINT64_C(0));
    std::fill(m_firstProcessor, m_firstProcessor + 4, (std::numeric_limits<unsigned>::max)());
}

uint64_t LockFreeMapArrivalTest::cpuNanoseconds()
{
#ifdef _WIN32
    FILETIME created, ended, kernel, user;
    if (GetThreadTimes(GetCurrentThread(), &created, &ended, &kernel, &user))
    {
        const uint64_t kernelTime = (static_cast<uint64_t>(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime;
        const uint64_t userTime = (static_cast<uint64_t>(user.dwHighDateTime) << 32) | user.dwLowDateTime;
        const uint64_t limit = (std::numeric_limits<uint64_t>::max)() / 100;
        if (kernelTime <= limit && userTime <= limit - kernelTime)
        {
            return (kernelTime + userTime) * 100;
        }
    }
#endif
    return (std::numeric_limits<uint64_t>::max)();
}

uint64_t LockFreeMapArrivalTest::affinity(unsigned& group)
{
    group = (std::numeric_limits<unsigned>::max)();
#ifdef _WIN32
    GROUP_AFFINITY allowed;
    if (GetThreadGroupAffinity(GetCurrentThread(), &allowed))
    {
        group = allowed.Group;
        return static_cast<uint64_t>(allowed.Mask);
    }
#endif
    return (std::numeric_limits<uint64_t>::max)();
}

bool LockFreeMapArrivalTest::attempt(LockFreeMap<int, uint64_t>& map,
    LockFreeMapArrivalWorker& result, unsigned mode, unsigned id)
{
    if (mode == 2)
    {
        // 每个消费者有独立的持续工作，避免把暂时空表误判成内部竞争饥饿
        if (map.erase(static_cast<int>(id)) && map.set(static_cast<int>(id), id))
        {
            return true;
        }
    }
    else if (mode == 1)
    {
        if (map.set(0, id))
        {
            return true;
        }
    }
    else if (mode == 0)
    {
        if (map.lockFreeMapSet(0, id) == LOCK_FREE_MAP_COMMITTED)
        {
            return true;
        }
    }
    else
    {
        uint64_t value = 0;
        LockFreeMapToken token;
        if (map.lockFreeMapGet(0, value, token) == LOCK_FREE_MAP_OK)
        {
            const LockFreeMapStatus status = map.lockFreeMapUpdate(0, token, value + 1);
            if (status == LOCK_FREE_MAP_COMMITTED)
            {
                return true;
            }
            if (status == LOCK_FREE_MAP_CONFLICT)
            {
                return false;
            }
        }
    }
    ++result.m_errors;
    return false;
}

bool LockFreeMapArrivalTest::scenario(unsigned mode, unsigned incumbents)
{
    const unsigned workers = 8;
    LockFreeMap<int, uint64_t> map;
    for (unsigned id = 0; id < (mode == 2 ? workers : 1); ++id)
    {
        if (!map.set(static_cast<int>(id), id))
        {
            return false;
        }
    }
    // 浠呭湪鐙珛鐜璇婃柇鏄惧紡鍚敤鏃跺浐瀹氬伐浣滅嚎绋嬶紱榛樿鍔熻兘楠岃瘉淇濇寔绯荤粺璋冨害
    std::vector<uintptr_t> processorMasks;
#ifdef _WIN32
    char fixedWorkers[2] = { 0, 0 };
    const DWORD requested = GetEnvironmentVariableA("LOCK_FREE_MAP_TEST_FIXED_ARRIVAL", fixedWorkers, 2);
    if (requested != 0)
    {
        if (requested != 1 || fixedWorkers[0] != '1')
        {
            return false;
        }
        DWORD_PTR processMask = 0, systemMask = 0;
        DWORD bytes = 0;
        if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask) ||
            GetLogicalProcessorInformation(nullptr, &bytes) || GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
            bytes == 0 || bytes % sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) != 0)
        {
            return false;
        }
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> topology(bytes / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (!GetLogicalProcessorInformation(topology.data(), &bytes))
        {
            return false;
        }
        for (size_t index = 0; index < bytes / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++index)
        {
            if (topology[index].Relationship == RelationProcessorCore)
            {
                const DWORD_PTR available = topology[index].ProcessorMask & processMask;
                if (available != 0)
                {
                    processorMasks.push_back(static_cast<uintptr_t>(available & (~available + 1)));
                }
            }
        }
        if (processorMasks.size() < workers)
        {
            return false;
        }
    }
#endif
    LockFreeMapArrivalWorker results[workers];
    std::atomic<unsigned> ready(0);
    std::atomic<uint64_t> warmed(0);
    std::atomic<bool> start(false), cancel(false);
    uint64_t begin = 0;
    std::vector<LockFreeMapTestThread> threads;
    threads.reserve(workers);
    LockFreeMapTestThreadGroup cleanup(threads, &cancel);
    for (unsigned id = 0; id < workers; ++id)
    {
        if (id == incumbents)
        {
            // 先确认热线程实际提交，再创建后来者；没有固定成功配额后退出的线程
            const uint64_t limit = LockFreeMapBenchmarkClock::nanoseconds() + UINT64_C(5000000000);
            while (warmed.load() < 4096)
            {
                if (LockFreeMapBenchmarkClock::nanoseconds() >= limit)
                {
                    return false;
                }
                std::this_thread::yield();
            }
        }
        // 每个工作线程独占结果；start 的发布将共同开始时间传给全部参与者
        threads.emplace_back([&map, &results, &ready, &warmed, &start, &cancel, &begin, &processorMasks, mode, id, incumbents]()
        {
            LockFreeMapArrivalWorker& result = results[id];
#ifdef _WIN32
            if (!processorMasks.empty() && SetThreadAffinityMask(GetCurrentThread(),
                static_cast<DWORD_PTR>(processorMasks[id])) == 0)
            {
                ++result.m_errors;
                cancel.store(true);
                return;
            }
#endif
            ready.fetch_add(1);
            while (!start.load())
            {
                if (cancel.load() || result.m_errors != 0)
                {
                    return;
                }
                if (id < incumbents)
                {
                    if (attempt(map, result, mode, id))
                    {
                        warmed.fetch_add(1);
                    }
                }
                else
                {
                    std::this_thread::yield();
                }
            }
            const uint64_t cpuBegin = cpuNanoseconds();
            result.m_allowedProcessors = affinity(result.m_processorGroup);
            while (!cancel.load() && result.m_errors == 0)
            {
                const uint64_t elapsed = LockFreeMapBenchmarkClock::nanoseconds() - begin;
                if (elapsed >= static_cast<uint64_t>(WINDOW_NANOSECONDS) * 4)
                {
                    break;
                }
                const size_t part = static_cast<size_t>(elapsed / WINDOW_NANOSECONDS);
                if (result.m_firstCallNanoseconds == (std::numeric_limits<uint64_t>::max)())
                {
                    result.m_firstCallNanoseconds = elapsed;
                }
#ifdef _WIN32
                if (result.m_attempts[part] == 0)
                {
                    result.m_firstProcessor[part] = GetCurrentProcessorNumber();
                }
#endif
                ++result.m_attempts[part];
                if (attempt(map, result, mode, id))
                {
                    ++result.m_completed;
                    // 成功归属完成时刻，不能把跨窗口的长调用算成早已取得执行机会
                    const uint64_t finished = LockFreeMapBenchmarkClock::nanoseconds() - begin;
                    if (finished >= elapsed && finished - elapsed > result.m_maxCallNanoseconds)
                    {
                        result.m_maxCallNanoseconds = finished - elapsed;
                    }
                    result.m_lastReturnNanoseconds = finished;
                    if (finished < static_cast<uint64_t>(WINDOW_NANOSECONDS) * 4)
                    {
                        ++result.m_successes[static_cast<size_t>(finished / WINDOW_NANOSECONDS)];
                    }
                }
            }
            const uint64_t cpuEnd = cpuNanoseconds();
            result.m_finalAllowedProcessors = affinity(result.m_finalProcessorGroup);
            if (cpuBegin != (std::numeric_limits<uint64_t>::max)() &&
                cpuEnd != (std::numeric_limits<uint64_t>::max)() && cpuEnd >= cpuBegin)
            {
                result.m_cpuNanoseconds = cpuEnd - cpuBegin;
            }
        });
    }
    const uint64_t limit = LockFreeMapBenchmarkClock::nanoseconds() + UINT64_C(5000000000);
    while (ready.load() != workers)
    {
        if (LockFreeMapBenchmarkClock::nanoseconds() >= limit)
        {
            return false;
        }
        std::this_thread::yield();
    }
    begin = LockFreeMapBenchmarkClock::nanoseconds();
    start.store(true);
    bool passed = true;
    for (unsigned id = 0; id < workers; ++id)
    {
        passed = threads[id].join() && passed;
    }
    uint64_t total = warmed.load();
    for (unsigned id = 0; id < workers; ++id)
    {
        const LockFreeMapArrivalWorker& result = results[id];
        passed = result.m_errors == 0 && passed;
        total += result.m_completed;
        passed = std::fprintf(LockFreeMapTestReport::m_file,
            "late_timing,%u,%u,%u,%llu,%llu,%llu,%llu,%u\n", mode, incumbents, id,
            static_cast<unsigned long long>(result.m_cpuNanoseconds),
            static_cast<unsigned long long>(result.m_maxCallNanoseconds),
            static_cast<unsigned long long>(result.m_firstCallNanoseconds),
            static_cast<unsigned long long>(result.m_lastReturnNanoseconds), result.m_errors) > 0 && passed;
        passed = std::fprintf(LockFreeMapTestReport::m_file,
            "late_cpu,%u,%u,%u,%u,%llu,%u,%llu,%u,%u,%u,%u\n", mode, incumbents, id,
            result.m_processorGroup, static_cast<unsigned long long>(result.m_allowedProcessors),
            result.m_finalProcessorGroup, static_cast<unsigned long long>(result.m_finalAllowedProcessors),
            result.m_firstProcessor[0], result.m_firstProcessor[1], result.m_firstProcessor[2], result.m_firstProcessor[3]) > 0 && passed;
        passed = std::fprintf(LockFreeMapTestReport::m_file,
            "late_completed,%u,%u,%u,%llu\n", mode, incumbents, id,
            static_cast<unsigned long long>(result.m_completed)) > 0 && passed;
        for (unsigned part = 0; part < 4; ++part)
        {
            passed = result.m_attempts[part] != 0 && result.m_successes[part] != 0 && passed;
            passed = std::fprintf(LockFreeMapTestReport::m_file,
                "late_progress,%u,%u,%u,%u,%llu,%llu\n", mode, incumbents, id, part,
                static_cast<unsigned long long>(result.m_attempts[part]),
                static_cast<unsigned long long>(result.m_successes[part])) > 0 && passed;
        }
    }
    for (unsigned id = 0; id < (mode == 2 ? workers : 1); ++id)
    {
        uint64_t value = 0;
        const bool found = map.get(static_cast<int>(id), value);
        passed = found && (mode == 2 ? value == id : mode == 3 ? value == total : value < workers) && passed;
    }
    passed = map.size() == (mode == 2 ? workers : 1) && passed;
    map.clearAndFreeMemory();
    return passed && map.retainedNodeCount() == 0 && map.retainedValueCount() == 0;
}

bool LockFreeMapArrivalTest::run()
{
    try
    {
        for (unsigned mode = 0; mode < 4; ++mode)
        {
            if (!scenario(mode, 1) || !scenario(mode, 4))
            {
                return false;
            }
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}