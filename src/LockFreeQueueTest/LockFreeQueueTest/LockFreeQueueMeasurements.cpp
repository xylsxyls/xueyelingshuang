#include "LockFreeQueueMeasurements.h"
#include "LockFreeQueueThreadGroup.h"
#ifdef LOCK_FREE_QUEUE_BASELINE
#include "LockFreeQueue.h"
#else
#include "LockFreeQueue.h"
#endif
#include "LockFreeQueueBenchmarkPush.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <new>
#include <cstring>
#include <memory>
#include <limits>
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#endif

bool g_lockFreeQueueMeasureSkipAllocations = false;
bool g_lockFreeQueueMeasurePreallocated = false;
std::atomic<bool> g_lockFreeQueueMeasureAllocations(false);
std::atomic<uint64_t> g_lockFreeQueueMeasureCalls(0);
std::atomic<uint64_t> g_lockFreeQueueMeasureBytes(0);

void* operator new(size_t bytes)
{
    void* memory = std::malloc(bytes == 0 ? 1 : bytes);
    if (memory == nullptr)
    {
        throw std::bad_alloc();
    }
    if (g_lockFreeQueueMeasureAllocations.load(std::memory_order_relaxed))
    {
        g_lockFreeQueueMeasureCalls.fetch_add(1, std::memory_order_relaxed);
        g_lockFreeQueueMeasureBytes.fetch_add(bytes, std::memory_order_relaxed);
    }
    return memory;
}

void operator delete(void* memory) throw()
{
    std::free(memory);
}

void* operator new[](size_t bytes)
{
    return ::operator new(bytes);
}

void operator delete[](void* memory) throw()
{
    ::operator delete(memory);
}

uint64_t LockFreeQueueMeasurements::ticks()
{
#ifdef _WIN32
    LARGE_INTEGER value;
    LARGE_INTEGER frequency;
    QueryPerformanceCounter(&value);
    QueryPerformanceFrequency(&frequency);
    return (value.QuadPart / frequency.QuadPart) * 1000000000ULL +
        (value.QuadPart % frequency.QuadPart) * 1000000000ULL / frequency.QuadPart;
#else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}

uint64_t LockFreeQueueMeasurements::cpu()
{
#ifdef _WIN32
    FILETIME creation, exitTime, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exitTime, &kernel, &user))
    {
        return 0;
    }
    return ((uint64_t(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
        (uint64_t(user.dwHighDateTime) << 32) + user.dwLowDateTime) * 100;
#else
    return 0;
#endif
}

void LockFreeQueueMeasurements::resources(const char* phase)
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX memory = {};
    memory.cb = sizeof(memory);
    const BOOL memoryOK = GetProcessMemoryInfo(GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory));
    DWORD handles = 0;
    const BOOL handlesOK = GetProcessHandleCount(GetCurrentProcess(), &handles);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    size_t threads = 0;
    THREADENTRY32 entry = {};
    entry.dwSize = sizeof(entry);
    if (snapshot != INVALID_HANDLE_VALUE && Thread32First(snapshot, &entry))
    {
        do
        {
            threads += entry.th32OwnerProcessID == GetCurrentProcessId() ? 1 : 0;
        } while (Thread32Next(snapshot, &entry));
    }
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        CloseHandle(snapshot);
    }
    std::cout << "RESOURCE phase=" << phase << " memory_ok=" << memoryOK << " handles_ok=" << handlesOK <<
        " private_bytes=" << memory.PrivateUsage << " working_set=" << memory.WorkingSetSize <<
        " handles=" << handles << " threads=" << threads << std::endl;
#else
    std::cout << "RESOURCE phase=" << phase << " NOT_RUN=native_resource_probe" << std::endl;
#endif
}

void LockFreeQueueMeasurements::latency(const char* operation, std::vector<uint64_t>& samples)
{
    std::sort(samples.begin(), samples.end());
    if (samples.empty())
    {
        return;
    }
    std::cout << "LATENCY operation=" << operation << " samples=" << samples.size() <<
        " p50_ns=" << samples[(samples.size() - 1) * 50 / 100] <<
        " p95_ns=" << samples[(samples.size() - 1) * 95 / 100] <<
        " p99_ns=" << samples[(samples.size() - 1) * 99 / 100] <<
        " max_ns=" << samples.back() << std::endl;
}

bool LockFreeQueueMeasurements::preparePool(LockFreeQueue<uint64_t>& queue, size_t itemCount,
    uint64_t& allocationCalls)
{
    allocationCalls = 0;
    if (itemCount == (std::numeric_limits<size_t>::max)() ||
        uint64_t(itemCount) >= uint64_t((std::numeric_limits<int64_t>::max)()))
    {
        return false;
    }
#ifndef LOCK_FREE_QUEUE_BASELINE
    if (reinterpret_cast<uintptr_t>(&queue) % 8 != 0)
    {
        std::cout << "FAIL queue_atomic_alignment address=" << &queue << std::endl;
        return false;
    }
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = itemCount + 1;
    config.maximumCapacity = static_cast<int64_t>(itemCount + 1);
    // Keep the normal protection-record budget; only remove growth from this test.
    if (queue.configurePool(config) != LockFreeQueueSuccess || queue.tryInit() != LockFreeQueueSuccess)
    {
        return false;
    }
#endif
    for (size_t i = 0; i < itemCount; ++i)
    {
        LockFreeQueueBenchmarkPush::checked(queue, static_cast<uint64_t>(i));
    }
    bool valid = true;
    for (size_t i = 0; i < itemCount; ++i)
    {
        uint64_t value = UINT64_MAX;
        valid = queue.pop(&value) && value == i && valid;
    }
#ifndef LOCK_FREE_QUEUE_BASELINE
    // Recover the warmed nodes before timing. No shrink callback is installed.
    for (size_t pass = 0; pass < 4; ++pass)
    {
        valid = queue.maintain(itemCount + 1).result == LockFreeQueueSuccess && valid;
    }
    const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
    allocationCalls = stats.allocationCalls;
    valid = stats.currentCapacity == itemCount + 1 && stats.successfulGrowCount == 0 &&
        stats.usedNodes == 1 && stats.retiredNodes == 0 && stats.reusableFreeCapacity == itemCount && valid;
    std::cout << "PREPARED capacity=" << stats.currentCapacity << " reusable=" << stats.reusableFreeCapacity <<
        " allocation_calls=" << allocationCalls << " grow_count=" << stats.successfulGrowCount <<
        " retained_bytes=" << stats.retainedBytes << " valid=" << valid << std::endl;
#else
    std::cout << "PREPARED baseline_warm_items=" << itemCount << " valid=" << valid << std::endl;
#endif
    return valid && queue.empty();
}

bool LockFreeQueueMeasurements::verifyPool(const LockFreeQueue<uint64_t>& queue, size_t itemCount,
    uint64_t allocationCalls)
{
#ifndef LOCK_FREE_QUEUE_BASELINE
    const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
    const bool valid = stats.currentCapacity == itemCount + 1 && stats.successfulGrowCount == 0 &&
        stats.allocationCalls == allocationCalls &&
        (g_lockFreeQueueMeasureSkipAllocations || g_lockFreeQueueMeasureCalls.load() == 0);
    std::cout << "PREALLOC_VERIFY capacity=" << stats.currentCapacity <<
        " allocation_before=" << allocationCalls << " allocation_after=" << stats.allocationCalls <<
        " allocation_delta=" << stats.allocationCalls - allocationCalls <<
        " grow_count=" << stats.successfulGrowCount << " valid=" << valid << std::endl;
    return valid;
#else
    (void)queue;
    (void)itemCount;
    (void)allocationCalls;
    return true;
#endif
}

bool LockFreeQueueMeasurements::burst(size_t width, size_t rounds)
{
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    std::vector<uint64_t> pushes, pops;
    pushes.reserve(width * rounds / 64 + rounds);
    pops.reserve(width * rounds / 64 + rounds);
    uint64_t allocationBaseline = 0;
    if (g_lockFreeQueueMeasurePreallocated)
    {
        if (!preparePool(queue, width, allocationBaseline))
        {
            return false;
        }
    }
    else
    {
        // Warm the same retained capacity before measuring hot paths.
        for (size_t i = 0; i < width; ++i)
        {
            LockFreeQueueBenchmarkPush::checked(queue, i);
        }
        queue.clear();
    }
    g_lockFreeQueueMeasureCalls.store(0);
    g_lockFreeQueueMeasureBytes.store(0);
    const uint64_t cpuStart = cpu();
    const uint64_t start = ticks();
    bool valid = true;
    g_lockFreeQueueMeasureAllocations.store(!g_lockFreeQueueMeasureSkipAllocations);
    for (size_t round = 0; round < rounds; ++round)
    {
        for (size_t i = 0; i < width; ++i)
        {
            const uint64_t begin = (i % 64 == 0) ? ticks() : 0;
            LockFreeQueueBenchmarkPush::checked(queue, round * width + i);
            if (i % 64 == 0)
            {
                pushes.push_back(ticks() - begin);
            }
        }
        for (size_t i = 0; i < width; ++i)
        {
            uint64_t value = UINT64_MAX;
            const uint64_t begin = (i % 64 == 0) ? ticks() : 0;
            const bool popped = queue.pop(&value);
            if (i % 64 == 0)
            {
                pops.push_back(ticks() - begin);
            }
            valid = popped && value == round * width + i && valid;
        }
    }
    g_lockFreeQueueMeasureAllocations.store(false);
    const uint64_t elapsed = ticks() - start;
    const uint64_t cpuElapsed = cpu() - cpuStart;
    if (g_lockFreeQueueMeasurePreallocated)
    {
        valid = verifyPool(queue, width, allocationBaseline) && valid;
    }
    std::cout << "BURST width=" << width << " rounds=" << rounds << " items=" << width * rounds <<
        " elapsed_ns=" << elapsed << " cpu_ns=" << cpuElapsed << " items_per_second=" <<
        (double(width) * rounds * 1000000000.0 / elapsed) << " allocations=" << g_lockFreeQueueMeasureCalls.load() <<
        " allocated_bytes=" << g_lockFreeQueueMeasureBytes.load() << " valid=" << valid << std::endl;
#ifndef LOCK_FREE_QUEUE_BASELINE
    const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
    std::cout << "POOL capacity=" << stats.currentCapacity << " retained_bytes=" << stats.retainedBytes <<
        " protection_bytes=" << stats.protectionBytes << " retries=" << stats.casRetries << std::endl;
#else
    std::cout << "POOL capacity=1 retained_bytes=" << sizeof(queue) + sizeof(QueueNode<uint64_t>) << std::endl;
#endif
    latency("push", pushes);
    latency("pop", pops);
    resources("drained");
    return valid && queue.empty();
}

bool LockFreeQueueMeasurements::concurrent(size_t producers, size_t count)
{
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    uint64_t allocationBaseline = 0;
    if (g_lockFreeQueueMeasurePreallocated)
    {
        if (count == 0 || producers > (std::numeric_limits<size_t>::max)() / count ||
            !preparePool(queue, producers * count, allocationBaseline))
        {
            return false;
        }
    }
    std::vector<uint64_t> next(producers, 0);
    std::vector<uint64_t> popSamples;
    std::vector<std::vector<uint64_t>> pushSamples(producers);
    std::atomic<bool> start(false), stop(false), error(false);
    std::atomic<size_t> done(0);
    std::atomic<uint64_t> producersEnded(0);
    std::atomic<uint64_t> refused(0);
    LockFreeQueueThreadGroup group(stop, &start);
    std::vector<std::thread>& workers = group.m_threads;
    workers.reserve(producers);
    popSamples.reserve(producers * count / 64 + 1);
    for (size_t lane = 0; lane < producers; ++lane)
    {
        pushSamples[lane].reserve(count / 64 + 1);
        workers.emplace_back([&, lane]() {
            try
            {
            while (!start.load())
            {
                std::this_thread::yield();
            }
            for (size_t i = 0; i < count && !stop.load(); ++i)
            {
                const uint64_t begin = i % 64 == 0 ? ticks() : 0;
                const uint64_t value = (uint64_t(lane) << 32) | i;
#ifdef LOCK_FREE_QUEUE_BASELINE
                LockFreeQueueBenchmarkPush::checked(queue, value);
#else
                while (!stop.load())
                {
                    const LockFreeQueueResult result = queue.tryPush(value);
                    if (result == LockFreeQueueSuccess)
                    {
                        break;
                    }
                    if (!LockFreeQueueBenchmarkPush::transient(result) && result != LockFreeQueueCapacityLimit)
                    {
                        error.store(true);
                        stop.store(true);
                    }
                    refused.fetch_add(1);
                    std::this_thread::yield();
                }
#endif
                if (i % 64 == 0)
                {
                    pushSamples[lane].push_back(ticks() - begin);
                }
            }
            }
            catch (...)
            {
                error.store(true);
                stop.store(true);
            }
            if (done.fetch_add(1) + 1 == producers)
            {
                producersEnded.store(ticks());
            }
        });
    }
    g_lockFreeQueueMeasureCalls.store(0);
    g_lockFreeQueueMeasureBytes.store(0);
    const uint64_t cpuStart = cpu();
    const uint64_t begin = ticks();
    const uint64_t deadline = begin + 30000000000ULL;
    size_t received = 0;
    bool valid = true;
    g_lockFreeQueueMeasureAllocations.store(!g_lockFreeQueueMeasureSkipAllocations);
    start.store(true);
    while ((done.load() != producers || !queue.empty()) && ticks() < deadline)
    {
        uint64_t value = 0;
        const uint64_t popStart = received % 64 == 0 ? ticks() : 0;
        if (queue.pop(&value))
        {
            if (received % 64 == 0)
            {
                popSamples.push_back(ticks() - popStart);
            }
            ++received;
            const size_t lane = static_cast<size_t>(value >> 32);
            if (lane >= producers || uint32_t(value) != next[lane]++)
            {
                valid = false;
            }
        }
        else
        {
            std::this_thread::yield();
        }
    }
    stop.store(true);
    group.join();
    g_lockFreeQueueMeasureAllocations.store(false);
    const uint64_t elapsed = ticks() - begin;
    const uint64_t cpuElapsed = cpu() - cpuStart;
    valid = valid && !error.load() && received == producers * count;
    if (g_lockFreeQueueMeasurePreallocated)
    {
        valid = verifyPool(queue, producers * count, allocationBaseline) && valid;
    }
    std::cout << "MPSC producers=" << producers << " items=" << received << " elapsed_ns=" << elapsed <<
        " producer_elapsed_ns=" << producersEnded.load() - begin <<
        " cpu_ns=" << cpuElapsed << " items_per_second=" << received * 1000000000.0 / elapsed <<
        " allocations=" << g_lockFreeQueueMeasureCalls.load() << " allocated_bytes=" << g_lockFreeQueueMeasureBytes.load() <<
        " rejected_retries=" << refused.load() << " valid=" << valid << std::endl;
    std::vector<uint64_t> combined;
    for (size_t i = 0; i < producers; ++i)
    {
        combined.insert(combined.end(), pushSamples[i].begin(), pushSamples[i].end());
    }
    latency("push", combined);
    latency("pop", popSamples);
#ifndef LOCK_FREE_QUEUE_BASELINE
    const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
    std::cout << "POOL capacity=" << stats.currentCapacity << " retained_bytes=" << stats.retainedBytes <<
        " protection_bytes=" << stats.protectionBytes << " retries=" << stats.casRetries << std::endl;
#endif
    resources("joined");
    return valid;
}

bool LockFreeQueueMeasurements::producersOnly(size_t producers, size_t count)
{
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    // Identical physical headroom for all pooled versions. One maximum private
    // batch per active producer prevents a cached batch from becoming pressure.
    const size_t capacityItems = producers * count + producers * 64;
    uint64_t baseline = 0;
    if (!preparePool(queue, capacityItems, baseline))
    {
        return false;
    }
    std::atomic<bool> start(false), stop(false), failed(false);
    std::atomic<uint64_t> last(0), refused(0);
    std::atomic<size_t> done(0);
    std::vector<std::vector<uint64_t>> samples(producers);
    LockFreeQueueThreadGroup group(stop, &start);
    for (size_t lane = 0; lane < producers; ++lane)
    {
        samples[lane].reserve(count / 64 + 1);
        group.m_threads.emplace_back([&, lane]() {
            while (!start.load())
            {
                std::this_thread::yield();
            }
            try
            {
                for (size_t index = 0; index < count && !stop.load(); ++index)
                {
                    const uint64_t value = (uint64_t(lane) << 32) | index;
                    const uint64_t began = index % 64 == 0 ? ticks() : 0;
#ifdef LOCK_FREE_QUEUE_BASELINE
                    LockFreeQueueBenchmarkPush::checked(queue, value);
#else
                    for (;;)
                    {
                        const LockFreeQueueResult result = queue.tryPush(value);
                        if (result == LockFreeQueueSuccess)
                        {
                            break;
                        }
                        refused.fetch_add(1);
                        if (!LockFreeQueueBenchmarkPush::transient(result) || stop.load())
                        {
                            failed.store(true);
                            stop.store(true);
                            break;
                        }
                        std::this_thread::yield();
                    }
#endif
                    if (index % 64 == 0)
                    {
                        samples[lane].push_back(ticks() - began);
                    }
                }
            }
            catch (...)
            {
                failed.store(true);
                stop.store(true);
            }
            if (done.fetch_add(1) + 1 == producers)
            {
                last.store(ticks());
            }
        });
    }
    g_lockFreeQueueMeasureCalls.store(0);
    g_lockFreeQueueMeasureBytes.store(0);
    const uint64_t cpuStart = cpu();
    const uint64_t begin = ticks();
    g_lockFreeQueueMeasureAllocations.store(!g_lockFreeQueueMeasureSkipAllocations);
    start.store(true);
    // The caller performs no queue reads, pops or validation during enqueue.
    group.join();
    g_lockFreeQueueMeasureAllocations.store(false);
    const uint64_t elapsed = last.load() - begin;
    const uint64_t cpuElapsed = cpu() - cpuStart;
    bool valid = !failed.load() && queue.size() == producers * count;
    std::vector<uint64_t> next(producers, 0);
    size_t received = 0;
    uint64_t value = 0;
    while (queue.pop(&value))
    {
        const size_t lane = static_cast<size_t>(value >> 32);
        if (lane >= producers || uint32_t(value) != next[lane]++)
        {
            valid = false;
        }
        ++received;
    }
    valid = received == producers * count && queue.empty() && valid;
    valid = verifyPool(queue, capacityItems, baseline) && valid;
    std::cout << "PRODUCERS_ONLY producers=" << producers << " items=" << received <<
        " capacity_items=" << capacityItems << " elapsed_ns=" << elapsed << " cpu_ns=" << cpuElapsed <<
        " items_per_second=" << received * 1e9 / elapsed <<
        " allocations=" << g_lockFreeQueueMeasureCalls.load() << " allocated_bytes=" << g_lockFreeQueueMeasureBytes.load() <<
        " rejected_retries=" << refused.load() << " valid=" << valid << std::endl;
    std::vector<uint64_t> combined;
    for (size_t lane = 0; lane < producers; ++lane)
    {
        combined.insert(combined.end(), samples[lane].begin(), samples[lane].end());
    }
    latency("push", combined);
#ifndef LOCK_FREE_QUEUE_BASELINE
    const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
    std::cout << "POOL capacity=" << stats.currentCapacity << " retained_bytes=" << stats.retainedBytes <<
        " protection_bytes=" << stats.protectionBytes << " retries=" << stats.casRetries << std::endl;
#endif
    return valid;
}

bool LockFreeQueueMeasurements::registered(size_t producers, size_t count, bool consume)
{
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    const size_t capacityItems = producers * count + producers * 64;
    uint64_t allocationBaseline = 0;
    if (!preparePool(queue, capacityItems, allocationBaseline))
    {
        return false;
    }
    std::atomic<bool> start(false), stop(false), failed(false);
    std::atomic<size_t> ready(0), done(0);
    std::atomic<uint64_t> ended(0), refused(0);
    std::vector<std::vector<uint64_t>> samples(producers);
    std::vector<uint64_t> popSamples;
    popSamples.reserve(producers * count / 64 + 1);
    std::vector<uint64_t> next(producers, 0);
    LockFreeQueueThreadGroup group(stop, &start);
    group.m_threads.reserve(producers);
    for (size_t lane = 0; lane < producers; ++lane)
    {
        samples[lane].reserve(count / 64 + 1);
        group.m_threads.emplace_back([&, lane]() {
#ifndef LOCK_FREE_QUEUE_BASELINE
            LockFreeQueueProducer<uint64_t> producer;
            if (queue.tryAcquireProducer(producer) != LockFreeQueueSuccess)
            {
                failed.store(true);
                stop.store(true);
            }
#endif
            ready.fetch_add(1);
            while (!start.load())
            {
                std::this_thread::yield();
            }
            try
            {
                for (size_t index = 0; index < count && !stop.load(); ++index)
                {
                    const uint64_t value = (uint64_t(lane) << 32) | index;
                    const uint64_t began = index % 64 == 0 ? ticks() : 0;
#ifdef LOCK_FREE_QUEUE_BASELINE
                    LockFreeQueueBenchmarkPush::checked(queue, value);
#else
                    for (;;)
                    {
                        const LockFreeQueueResult result = producer.tryPush(value);
                        if (result == LockFreeQueueSuccess)
                        {
                            break;
                        }
                        refused.fetch_add(1);
                        if (!LockFreeQueueBenchmarkPush::transient(result) || stop.load())
                        {
                            failed.store(true);
                            stop.store(true);
                            break;
                        }
                        std::this_thread::yield();
                    }
#endif
                    if (index % 64 == 0)
                    {
                        samples[lane].push_back(ticks() - began);
                    }
                }
            }
            catch (...)
            {
                failed.store(true);
                stop.store(true);
            }
#ifndef LOCK_FREE_QUEUE_BASELINE
            producer.release();
#endif
            if (done.fetch_add(1) + 1 == producers)
            {
                ended.store(ticks());
            }
        });
    }
    while (ready.load() != producers)
    {
        std::this_thread::yield();
    }
    g_lockFreeQueueMeasureCalls.store(0);
    g_lockFreeQueueMeasureBytes.store(0);
    const uint64_t cpuStart = cpu();
    const uint64_t begin = ticks();
    const uint64_t deadline = begin + 30000000000ULL;
    g_lockFreeQueueMeasureAllocations.store(!g_lockFreeQueueMeasureSkipAllocations);
    size_t received = 0;
    bool valid = true;
    start.store(true);
    if (consume)
    {
        while (received < producers * count && !stop.load() && ticks() < deadline)
        {
            uint64_t value = 0;
            const uint64_t began = received % 64 == 0 ? ticks() : 0;
            if (queue.pop(&value))
            {
                if (received % 64 == 0)
                {
                    popSamples.push_back(ticks() - began);
                }
                ++received;
                const size_t lane = static_cast<size_t>(value >> 32);
                if (lane >= producers || uint32_t(value) != next[lane]++)
                {
                    valid = false;
                }
            }
            else
            {
                std::this_thread::yield();
            }
        }
        stop.store(true);
    }
    group.join();
    g_lockFreeQueueMeasureAllocations.store(false);
    const uint64_t elapsed = (consume ? ticks() : ended.load()) - begin;
    const uint64_t cpuElapsed = cpu() - cpuStart;
    if (!consume)
    {
        // No queue observation or consumption occurred during producer timing.
        uint64_t value = 0;
        while (queue.pop(&value))
        {
            ++received;
            const size_t lane = static_cast<size_t>(value >> 32);
            if (lane >= producers || uint32_t(value) != next[lane]++)
            {
                valid = false;
            }
        }
    }
    valid = valid && !failed.load() && received == producers * count && queue.empty();
    valid = verifyPool(queue, capacityItems, allocationBaseline) && valid;
    std::cout << (consume ? "REGISTERED_MPSC" : "REGISTERED_PRODUCERS") << " producers=" << producers <<
        " items=" << received << " capacity_items=" << capacityItems << " elapsed_ns=" << elapsed <<
        " producer_elapsed_ns=" << ended.load() - begin << " cpu_ns=" << cpuElapsed <<
        " items_per_second=" << received * 1e9 / elapsed <<
        " allocations=" << g_lockFreeQueueMeasureCalls.load() << " allocated_bytes=" << g_lockFreeQueueMeasureBytes.load() <<
        " rejected_retries=" << refused.load() << " valid=" << valid << std::endl;
    std::vector<uint64_t> combined;
    for (size_t lane = 0; lane < producers; ++lane)
    {
        combined.insert(combined.end(), samples[lane].begin(), samples[lane].end());
    }
    latency("push", combined);
    latency("pop", popSamples);
#ifndef LOCK_FREE_QUEUE_BASELINE
    const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
    std::cout << "POOL capacity=" << stats.currentCapacity << " retained_bytes=" << stats.retainedBytes <<
        " protection_bytes=" << stats.protectionBytes << " retries=" << stats.casRetries <<
        " active_producers=" << stats.activeProducers << std::endl;
    valid = stats.activeProducers == 0 && valid;
#endif
    return valid;
}

bool LockFreeQueueMeasurements::backlog(size_t count)
{
    std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
    std::vector<uint64_t> samples(count);
    for (size_t i = 0; i < count; ++i)
    {
        LockFreeQueueBenchmarkPush::checked(queue, i);
    }
    bool valid = true;
    for (size_t i = 0; i < count; ++i)
    {
        uint64_t value = UINT64_MAX;
        const uint64_t start = ticks();
        const bool popped = queue.pop(&value);
        samples[i] = ticks() - start;
        valid = popped && value == i && valid;
    }
    std::cout << "BACKLOG count=" << count << " first_pop_ns=" << samples.front() << " valid=" << valid << std::endl;
    latency("pop_backlog", samples);
    return valid && queue.empty();
}

bool LockFreeQueueMeasurements::poolPaths()
{
#ifndef LOCK_FREE_QUEUE_BASELINE
    std::vector<uint64_t> growth, full, shrink;
    growth.reserve(50);
    full.reserve(50);
    shrink.reserve(50);
    size_t beforeBytes = 0, afterBytes = 0;
    bool valid = true;
    for (size_t round = 0; round < 50; ++round)
    {
        std::unique_ptr<LockFreeQueue<uint64_t>> instance(new LockFreeQueue<uint64_t>());
    LockFreeQueue<uint64_t>& queue = *instance;
        queue.destroy();
        LockFreeQueuePoolConfig config;
        config.initialCapacity = 2;
        config.maximumCapacity = 8;
        config.maximumProducers = 8;
        valid = queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess && valid;
        queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(2); });
        LockFreeQueueBenchmarkPush::checked(queue, 0);
        uint64_t start = ticks();
        valid = queue.tryPush(1) == LockFreeQueueSuccess && valid;
        growth.push_back(ticks() - start);
        for (uint64_t i = 2; i < 7; ++i)
        {
            LockFreeQueueBenchmarkPush::checked(queue, i);
        }
        start = ticks();
        valid = queue.tryPush(7) == LockFreeQueueCapacityLimit && valid;
        full.push_back(ticks() - start);
        for (uint64_t i = 0; i < 7; ++i)
        {
            uint64_t value = UINT64_MAX;
            valid = queue.pop(&value) && value == i && valid;
        }
        queue.setShrinkPolicy([](const LockFreeQueueShrinkContext&) {
            LockFreeQueueShrinkDecision decision = { 100, 1 };
            return decision;
        });
        queue.maintain(128);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        beforeBytes = queue.poolStatistics().retainedBytes;
        start = ticks();
        queue.maintain(128);
        queue.maintain(128);
        shrink.push_back(ticks() - start);
        afterBytes = queue.poolStatistics().retainedBytes;
        valid = queue.poolStatistics().currentCapacity == 2 && beforeBytes > afterBytes && valid;
    }
    latency("growth_allocate_publish", growth);
    latency("full_pool_reject", full);
    latency("shrink_detach_free_256_visits", shrink);
    std::cout << "SHRINK before_retained_bytes=" << beforeBytes << " after_retained_bytes=" << afterBytes <<
        " initial_capacity=2 drained_capacity=2 valid=" << valid << std::endl;
    return valid;
#else
    std::cout << "POOL_PATHS NOT_APPLICABLE=baseline_has_no_pool" << std::endl;
    return true;
#endif
}

bool LockFreeQueueMeasurements::payloadCost()
{
    LockFreeQueue<std::shared_ptr<std::vector<int32_t>>> queue;
    std::vector<uint64_t> direct, queued;
    direct.reserve(64);
    queued.reserve(64);
    bool valid = true;
    for (size_t i = 0; i < 64; ++i)
    {
        std::shared_ptr<std::vector<int32_t>> value = std::make_shared<std::vector<int32_t>>(65536, 9);
        uint64_t start = ticks();
        value.reset();
        direct.push_back(ticks() - start);
        value = std::make_shared<std::vector<int32_t>>(65536, 9);
        std::weak_ptr<std::vector<int32_t>> weak = value;
        LockFreeQueueBenchmarkPush::checked(queue, value);
        value.reset();
        start = ticks();
        valid = queue.pop(nullptr) && valid;
        queued.push_back(ticks() - start);
        valid = weak.expired() && valid;
    }
    latency("direct_destroy_256kb_payload", direct);
    latency("pop_destroy_256kb_payload", queued);
    return valid;
}

int LockFreeQueueMeasurements::run(int argc, char** argv)
{
    if (argc < 2 || argc > 4 || (std::strcmp(argv[1], "5") != 0 && std::strcmp(argv[1], "22") != 0 && std::strcmp(argv[1], "33") != 0 && std::strcmp(argv[1], "35") != 0))
    {
        return 2;
    }
    g_lockFreeQueueMeasureSkipAllocations = false;
    g_lockFreeQueueMeasurePreallocated = false;
    for (int i = 2; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--no-allocation-count") == 0 && !g_lockFreeQueueMeasureSkipAllocations)
        {
            g_lockFreeQueueMeasureSkipAllocations = true;
        }
        else if (std::strcmp(argv[i], "--preallocated") == 0 && !g_lockFreeQueueMeasurePreallocated)
        {
            g_lockFreeQueueMeasurePreallocated = true;
        }
        else
        {
            return 2;
        }
    }
    if (g_lockFreeQueueMeasurePreallocated && std::strcmp(argv[1], "5") != 0 && std::strcmp(argv[1], "33") != 0 && std::strcmp(argv[1], "35") != 0)
    {
        return 2;
    }
    if (std::strcmp(argv[1], "33") == 0 || std::strcmp(argv[1], "35") == 0)
    {
        g_lockFreeQueueMeasurePreallocated = true;
    }
    std::cout << "ENV allocation_tracking=" << (g_lockFreeQueueMeasureSkipAllocations ? "disabled" : "enabled") <<
        " pointer_bytes=" << sizeof(void*) << " seed=20261104 implementation=";
#ifdef LOCK_FREE_QUEUE_BASELINE
    std::cout << "baseline";
#else
    std::cout << "current";
#endif
    std::cout << " preallocated=" << g_lockFreeQueueMeasurePreallocated << std::endl;
    resources("before");
    bool valid = true;
    if (std::strcmp(argv[1], "35") == 0)
    {
        for (size_t producers = 1; producers <= 8; producers *= 2)
        {
            valid = registered(producers, 250000, false) && valid;
            valid = registered(producers, 50000, true) && valid;
        }
    }
    else if (std::strcmp(argv[1], "33") == 0)
    {
        for (size_t producers = 1; producers <= 8; producers *= 2)
        {
            valid = producersOnly(producers, 250000) && valid;
        }
    }
    else if (std::strcmp(argv[1], "22") == 0)
    {
        const size_t counts[] = { 1, 1024, 65536, 262144 };
        for (size_t i = 0; i < 4; ++i)
        {
            valid = backlog(counts[i]) && valid;
        }
    }
    else
    {
        valid = burst(1, 200000) && valid;
        valid = burst(4096, 30) && valid;
        for (size_t producers = 1; producers <= 8; producers *= 2)
        {
            valid = concurrent(producers, 50000) && valid;
        }
        if (!g_lockFreeQueueMeasurePreallocated)
        {
            valid = poolPaths() && valid;
            valid = payloadCost() && valid;
        }
    }
    resources("after_destroy");
    std::cout << (valid ? "PASS" : "FAIL") << " id=" << argv[1] << std::endl;
    return valid ? 0 : 1;
}

int main(int argc, char** argv)
{
    try
    {
        return LockFreeQueueMeasurements::run(argc, argv);
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL exception=" << error.what() << std::endl;
        return 1;
    }
}
