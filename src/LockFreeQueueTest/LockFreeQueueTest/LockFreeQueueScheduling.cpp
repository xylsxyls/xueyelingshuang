#define NOMINMAX
#include <Windows.h>
#include "LockFreeQueue.h"
#include "LockFreeQueueBenchmarkPush.h"
#include "LockFreeQueueThreadGroup.h"
#include "LockFreeQueueScheduling.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

LockFreeQueueScheduling::Lane::Lane() : m_progress(0), m_completed(0), m_count(0), m_attempts(0),
m_cycles(0), m_cpu100ns(0), m_empty(0), m_capacityFailures(0), m_sampledGapNs(0), m_priority(0)
{
}

uint64_t LockFreeQueueScheduling::now()
{
    LARGE_INTEGER value, frequency;
    QueryPerformanceCounter(&value);
    QueryPerformanceFrequency(&frequency);
    return static_cast<uint64_t>(value.QuadPart / frequency.QuadPart) * 1000000000ULL +
        static_cast<uint64_t>((value.QuadPart % frequency.QuadPart) * 1000000000ULL / frequency.QuadPart);
}

uint64_t LockFreeQueueScheduling::cpu()
{
    FILETIME creation, exit, kernel, user;
    if (!GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user))
    {
        throw std::runtime_error("GetThreadTimes failed");
    }
    return ((uint64_t(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime) +
        ((uint64_t(user.dwHighDateTime) << 32) | user.dwLowDateTime);
}

uint64_t LockFreeQueueScheduling::cycles()
{
    ULONG64 value = 0;
    if (!QueryThreadCycleTime(GetCurrentThread(), &value))
    {
        throw std::runtime_error("QueryThreadCycleTime failed");
    }
    return value;
}

int LockFreeQueueScheduling::run(size_t producers, uint32_t milliseconds, int mode, bool cancelable, size_t capacity)
{
    // Mode 0: ordinary push; 1: bound push; 2: ordinary tryPush;
    // 3: bound tryPush; 4: external Entry with ready-prefix consumer;
    // 5: bound push, distributed counters and value-prefix consumer.
    LockFreeQueue<uint64_t> queue(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = capacity;
    config.maximumCapacity = static_cast<int64_t>(capacity);
    config.growThreshold = 0;
    config.maximumProducers = producers + 4;
    if (queue.configurePool(config) != LockFreeQueueSuccess ||
        queue.configureCancelableReservations(cancelable) != LockFreeQueueSuccess ||
        queue.configureProducerCounters(mode == 5) != LockFreeQueueSuccess ||
        queue.configureEntryOnly(mode == 4) != LockFreeQueueSuccess ||
        queue.tryInit() != LockFreeQueueSuccess)
    {
        throw std::runtime_error("Queue configuration failed");
    }
    const size_t ringSize = 8192;
    struct Slot
    {
        LockFreeQueueEntry<uint64_t> m_entry;
        std::atomic<bool> m_free;
        Slot() : m_free(true) {}
    };
    std::vector<std::unique_ptr<Slot[]> > entries(producers);
    if (mode == 4)
    {
        for (size_t lane = 0; lane < producers; ++lane)
        {
            entries[lane].reset(new Slot[ringSize]);
        }
    }
    std::unique_ptr<Lane[]> lanes(new Lane[producers + 1]);
    std::atomic<bool> stop(false), start(false), failed(false);
    std::atomic<size_t> ready(0), done(0);
    LockFreeQueueThreadGroup group(stop, &start);
    group.m_threads.reserve(producers + 1);
    for (size_t lane = 0; lane < producers; ++lane)
    {
        group.m_threads.emplace_back([&, lane]()
        {
            bool announced = false;
            try
            {
                LockFreeQueueProducer<uint64_t> writer;
                if ((mode == 1 || mode == 3 || mode == 4 || mode == 5) && queue.tryAcquireProducer(writer) != LockFreeQueueSuccess)
                {
                    throw std::runtime_error("Producer registration failed");
                }
                Lane& result = lanes[lane];
                result.m_priority = GetThreadPriority(GetCurrentThread());
                ready.fetch_add(1);
                announced = true;
                while (!start.load()) { std::this_thread::yield(); }
                const uint64_t beginCpu = cpu(), beginCycles = cycles();
                while (!stop.load(std::memory_order_relaxed))
                {
                    const uint64_t value = (uint64_t(lane) << 48) | result.m_count;
                    int32_t status = LockFreeQueueTemporaryUnavailable;
                    if (mode == 4)
                    {
                        Slot& slot = entries[lane][result.m_count % ringSize];
                        if (!slot.m_free.load(std::memory_order_acquire))
                        {
                            std::this_thread::yield();
                            continue;
                        }
                        slot.m_free.store(false, std::memory_order_relaxed);
                        slot.m_entry.m_value = value;
                        status = writer.tryPushEntry(slot.m_entry);
                        if (status != LockFreeQueueSuccess) { slot.m_free.store(true, std::memory_order_release); }
                    }
                    else if (mode == 0) { status = LockFreeQueueBenchmarkPush::submit(queue, value); }
                    else if (mode == 1 || mode == 5) { status = LockFreeQueueBenchmarkPush::submit(writer, value); }
                    else if (mode == 2) { status = queue.tryPush(value); }
                    else { status = writer.tryPush(value); }
                    ++result.m_attempts;
                    if (result.m_attempts == 1 || (result.m_attempts & 63) == 0)
                    {
                        result.m_completed.store(result.m_attempts, std::memory_order_relaxed);
                    }
                    if ((mode == 0 || mode == 1 || mode == 5) && status == LockFreeQueueBusy)
                    {
                        throw std::runtime_error("Non-try push returned Busy");
                    }
                    if (status == LockFreeQueueCapacityLimit) { ++result.m_capacityFailures; }
                    if (status == LockFreeQueueSuccess)
                    {
                        ++result.m_count;
                        // The first completion must be observable even if a
                        // heavily contended lane completes fewer than 64 items.
                        if (result.m_count == 1 || (result.m_count & 63) == 0)
                        {
                            result.m_progress.store(result.m_count, std::memory_order_relaxed);
                        }
                    }
                    else if (LockFreeQueueBenchmarkPush::transient(status) || status == LockFreeQueueCapacityLimit)
                    {
                        // Identical caller backpressure for both revisions.
                        std::this_thread::yield();
                    }
                    else { throw std::runtime_error("Unexpected producer result"); }
                }
                result.m_cycles = cycles() - beginCycles;
                result.m_cpu100ns = cpu() - beginCpu;
                result.m_progress.store(result.m_count);
            }
            catch (...) { failed.store(true); stop.store(true); if (!announced) { ready.fetch_add(1); } }
            done.fetch_add(1);
        });
    }
    group.m_threads.emplace_back([&]()
    {
        bool announced = false;
        try
        {
            Lane& result = lanes[producers];
            std::vector<uint64_t> expected(producers, 0);
            result.m_priority = GetThreadPriority(GetCurrentThread());
            ready.fetch_add(1);
            announced = true;
            while (!start.load()) { std::this_thread::yield(); }
            const uint64_t beginCpu = cpu(), beginCycles = cycles();
            uint64_t maintenanceAt = 0;
            while (!failed.load() && (!stop.load() || done.load() != producers || !queue.empty()))
            {
                uint64_t values[64];
                LockFreeQueueEntry<uint64_t>* batch[64];
                size_t count = 0;
                if (mode == 4)
                {
                    queue.popReadyEntryPrefix(batch, 64, count);
                    for (size_t i = 0; i < count; ++i) { values[i] = batch[i]->m_value; }
                }
                else if (mode == 5) { queue.popReadyPrefix(values, 64, count); }
                else if (queue.pop(&values[0])) { count = 1; }
                ++result.m_attempts;
                if (result.m_attempts == 1 || (result.m_attempts & 63) == 0)
                {
                    result.m_completed.store(result.m_attempts, std::memory_order_relaxed);
                }
                if (count == 0) { ++result.m_empty; } // Deliberately busy consumer; library must cooperate.
                for (size_t i = 0; i < count; ++i)
                {
                    const size_t lane = static_cast<size_t>(values[i] >> 48);
                    const uint64_t sequence = values[i] & ((uint64_t(1) << 48) - 1);
                    if (lane >= producers || sequence != expected[lane]++)
                    {
                        throw std::runtime_error("FIFO, duplicate or missing item");
                    }
                    if (mode == 4)
                    {
                        Slot& slot = entries[lane][sequence % ringSize];
                        if (batch[i] != &slot.m_entry) { throw std::runtime_error("Wrong Entry address"); }
                        slot.m_free.store(true, std::memory_order_release);
                    }
                }
                result.m_count += count;
                if (count != 0 && (result.m_count == count || (result.m_count & 63) < count))
                {
                    result.m_progress.store(result.m_count, std::memory_order_relaxed);
                }
                if ((mode == 4 || mode == 5) && result.m_count - maintenanceAt >= 1024)
                {
                    queue.maintain(64);
                    maintenanceAt = result.m_count;
                }
            }
            result.m_cycles = cycles() - beginCycles;
            result.m_cpu100ns = cpu() - beginCpu;
            result.m_progress.store(result.m_count);
        }
        catch (...) { failed.store(true); stop.store(true); if (!announced) { ready.fetch_add(1); } }
    });
    while (ready.load() != producers + 1) { std::this_thread::yield(); }
    const uint64_t begin = now();
    std::vector<uint64_t> last(producers + 1, 0), lastChange(producers + 1, begin), activeAttempts(producers + 1, 0);
    start.store(true);
    while (!failed.load() && now() - begin < uint64_t(milliseconds) * 1000000)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        const uint64_t timestamp = now();
        for (size_t lane = 0; lane <= producers; ++lane)
        {
            // A completed capacity-error call proves CPU/call progress, but
            // does not prove admission. Keep both measurements distinct.
            const uint64_t progress = capacity == 256 && lane < producers ?
                lanes[lane].m_completed.load(std::memory_order_relaxed) :
                lanes[lane].m_progress.load(std::memory_order_relaxed);
            lanes[lane].m_sampledGapNs = (std::max)(lanes[lane].m_sampledGapNs, timestamp - lastChange[lane]);
            if (progress != last[lane]) { last[lane] = progress; lastChange[lane] = timestamp; }
        }
    }
    // Capture progress while the window is still active. Final join/drain counts
    // must not conceal a lane that ran only after the monitor requested stop.
    for (size_t lane = 0; lane <= producers; ++lane)
    {
        last[lane] = lanes[lane].m_progress.load(std::memory_order_relaxed);
        activeAttempts[lane] = lanes[lane].m_completed.load(std::memory_order_relaxed);
    }
    const uint64_t stopTime = now();
    stop.store(true);
    group.join();
    const uint64_t end = now();
    uint64_t total = 0;
    bool valid = !failed.load();
    bool progressed = true, samePriority = true;
    for (size_t lane = 0; lane <= producers; ++lane)
    {
        const Lane& r = lanes[lane];
        if (lane < producers) { total += r.m_count; }
        // Only the deliberately capacity-constrained producer case allows
        // no admission. It must still finish a call before stop is requested.
        const bool requireAdmission = capacity != 256 || lane == producers;
        progressed = progressed && activeAttempts[lane] != 0 && r.m_cycles != 0 &&
            (!requireAdmission || (last[lane] != 0 && r.m_count != 0));
        samePriority = samePriority && r.m_priority == lanes[0].m_priority;
        std::cout << "LANE index=" << lane << " consumer=" << (lane == producers) << " count=" << r.m_count
            << " active_count=" << last[lane] << " active_attempts=" << activeAttempts[lane]
            << " capacity_failures=" << r.m_capacityFailures << " attempts=" << r.m_attempts << " empty=" << r.m_empty << " cycles=" << r.m_cycles
            << " cpu_ms=" << r.m_cpu100ns / 10000.0 << " priority=" << r.m_priority
            << " sampled_progress_gap_ms=" << r.m_sampledGapNs / 1000000.0 << std::endl;
    }
    valid = valid && total != 0 && total == lanes[producers].m_count && queue.empty();
    if (mode == 4)
    {
        for (size_t lane = 0; lane < producers; ++lane)
        {
            for (size_t i = 0; i < ringSize; ++i) { valid = valid && entries[lane][i].m_free.load(); }
        }
    }
    std::cout << "SCHEDULE producers=" << producers << " mode=" << mode << " cancelable=" << cancelable << " capacity=" << capacity
        << " items=" << total << " consumed=" << lanes[producers].m_count << " active_ms=" << (stopTime - begin) / 1000000.0
        << " drain_ms=" << (end - stopTime) / 1000000.0 << " items_per_second=" << total * 1000000000.0 / (end - begin)
        << " process_priority=" << GetPriorityClass(GetCurrentProcess()) << " data_valid=" << valid
        << " same_priority=" << samePriority << " progress_valid=" << progressed
        << " valid=" << (valid && samePriority && progressed) << std::endl;
    return !valid || !samePriority ? 1 : (progressed ? 0 : 3);
}

int main(int argc, char** argv)
{
    if (argc != 5 && argc != 6) { return 2; }
    const int producers = std::atoi(argv[1]), duration = std::atoi(argv[2]), mode = std::atoi(argv[3]), cancelable = std::atoi(argv[4]);
    if (producers < 1 || producers > 16 || duration < 100 || duration > 60000 || mode < 0 || mode > 5 || cancelable < 0 || cancelable > 1) { return 2; }
    const int capacity = argc == 6 ? std::atoi(argv[5]) : 65536;
    if (capacity != 256 && capacity != 65536) { return 2; }
    try { return LockFreeQueueScheduling::run(producers, duration, mode, cancelable != 0, capacity); }
    catch (const std::exception& error) { std::cout << "FAIL " << error.what() << std::endl; return 1; }
    catch (...) { std::cout << "FAIL unknown exception" << std::endl; return 1; }
}