#include "LockFreeQueueSharedPtrBenchmark.h"
#include "LockFreeQueue.h"
#include "LockFreeQueueBenchmarkPush.h"
#include "LockFreeQueueThreadGroup.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#if defined(_WIN32)
#include <Windows.h>
#include <Psapi.h>
#endif

double LockFreeQueueSharedPtrBenchmark::now()
{
#if defined(_WIN32)
    LARGE_INTEGER frequency, counter;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return static_cast<double>(counter.QuadPart) / static_cast<double>(frequency.QuadPart);
#else
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

uint32_t LockFreeQueueSharedPtrBenchmark::processor()
{
#if defined(_WIN32)
    return GetCurrentProcessorNumber();
#else
    return 0;
#endif
}

uint64_t LockFreeQueueSharedPtrBenchmark::cpuTicks()
{
#if defined(_WIN32)
    FILETIME created, exited, kernel, user;
    if (GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user))
    {
        return (static_cast<uint64_t>(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
            (static_cast<uint64_t>(user.dwHighDateTime) << 32) + user.dwLowDateTime;
    }
#endif
    return 0;
}

int LockFreeQueueSharedPtrBenchmark::run(size_t producers, size_t items, bool bound, bool prefix, bool distributed, bool reserved)
{
    typedef std::shared_ptr<LockFreeQueueSharedPtrNode> Value;
    typedef LockFreeQueue<Value> Queue;
    const size_t perProducer = items / producers;
    std::vector<Value> owners;
    owners.reserve(items);
    for (size_t lane = 0; lane < producers; ++lane)
    {
        for (size_t sequence = 0; sequence < perProducer; ++sequence)
        {
            Value node = std::make_shared<LockFreeQueueSharedPtrNode>();
            node->m_identity = (static_cast<uint64_t>(lane) << 48) | sequence;
            owners.push_back(node);
        }
    }
    // All 20 million objects remain distinct and owned throughout both phases.
    Queue queue;
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = items + 64 * producers + 1;
    config.maximumCapacity = static_cast<int64_t>(config.initialCapacity);
    config.growThreshold = 0;
    config.maximumProducers = producers + 4;
    if (queue.configurePool(config) != LockFreeQueueSuccess)
    {
        throw std::runtime_error("Fixed pool configuration failed");
    }
#if defined(LOCK_FREE_QUEUE_SHARED_EXTRA)
    if (queue.configureProducerCounters(distributed) != LockFreeQueueSuccess)
    {
        throw std::runtime_error("Counter configuration failed");
    }
    if (queue.configureCancelableReservations(reserved) != LockFreeQueueSuccess)
    {
        throw std::runtime_error("Publication configuration failed");
    }
#else
    if (prefix || distributed || reserved)
    {
        throw std::runtime_error("Requested API is unavailable in this baseline");
    }
#endif
    const LockFreeQueueResult initialized = queue.tryInit();
    if (initialized != LockFreeQueueSuccess)
    {
        std::cout << "INIT_FAILURE status=" << initialized << std::endl;
        return 4;
    }
    const LockFreeQueuePoolStatistics initial = queue.poolStatistics();
    std::cout << std::setprecision(12) << "SETUP case=46 items=" << items << " producers=" << producers
        << " node_bytes=" << sizeof(LockFreeQueueSharedPtrNode) << " shared_ptr_bytes=" << sizeof(Value)
        << " capacity=" << initial.currentCapacity << " retained_bytes=" << initial.retainedBytes
        << " initial_allocations=" << initial.allocationCalls << " bound=" << bound << " prefix=" << prefix
        << " distributed=" << distributed << " reserved=" << reserved
        << " move_noexcept=" << std::is_nothrow_move_assignable<Value>::value << std::endl;
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX memory = {};
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)))
    {
        std::cout << "MEMORY private_bytes=" << memory.PrivateUsage << " working_set_bytes=" << memory.WorkingSetSize << std::endl;
    }
#endif
    now();
    for (int phase = 0; phase < 2; ++phase)
    {
        std::atomic<bool> start(false), stop(false);
        std::atomic<size_t> ready(0);
        std::atomic<int32_t> failure(0);
        std::vector<double> finished(producers, 0.0);
        std::vector<size_t> sent(producers, 0), expected(producers, 0);
        std::vector<uint32_t> processorBegin(producers + 1, 0), processorEnd(producers + 1, 0);
        std::vector<uint64_t> cpuUsed(producers + 1, 0);
        size_t consumed = 0, emptyPolls = 0;
        double consumerFinished = 0, started = 0;
        LockFreeQueueThreadGroup threads(stop, &start);
        const LockFreeQueuePoolStatistics before = queue.poolStatistics();
        // Each producer owns its partition, optional context and result slot.
        for (size_t lane = 0; lane < producers; ++lane)
        {
            threads.m_threads.push_back(std::thread([&, lane]()
            {
                LockFreeQueueProducer<Value> producer;
                if (bound)
                {
                    const int32_t acquired = queue.tryAcquireProducer(producer);
                    if (acquired != LockFreeQueueSuccess)
                    {
                        failure.store(acquired);
                        stop.store(true);
                    }
                }
                ready.fetch_add(1);
                while (!start.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
                const size_t offset = lane * perProducer;
                processorBegin[lane] = processor();
                const uint64_t cpuStarted = cpuTicks();
                size_t submitted = 0;
                for (size_t sequence = 0; sequence < perProducer; ++sequence)
                {
                    if ((sequence & 4095) == 0 && stop.load(std::memory_order_relaxed))
                    {
                        break;
                    }
                    const int32_t status = bound ? LockFreeQueueBenchmarkPush::submit(producer, owners[offset + sequence]) :
                        LockFreeQueueBenchmarkPush::submit(queue, owners[offset + sequence]);
                    if (status != LockFreeQueueSuccess)
                    {
                        failure.store(status);
                        stop.store(true);
                        break;
                    }
                    ++submitted;
                }
                finished[lane] = now();
                cpuUsed[lane] = cpuTicks() - cpuStarted;
                processorEnd[lane] = processor();
                sent[lane] = submitted;
                producer.release();
            }));
        }
        // One consumer checks per-producer order and total delivery. Identity
        // checking is inside the measured interval in both compared revisions.
        const auto consume = [&]()
        {
            Value output[64];
            processorBegin[producers] = processor();
            const uint64_t cpuStarted = cpuTicks();
            while (consumed < items && !stop.load(std::memory_order_relaxed))
            {
                size_t count = 0;
#if defined(LOCK_FREE_QUEUE_SHARED_EXTRA)
                if (prefix)
                {
                    queue.popReadyPrefix(output, 64, count);
                }
                else
#endif
                {
                    count = queue.pop(output) ? 1 : 0;
                }
                if (count == 0)
                {
                    ++emptyPolls;
                }
                for (size_t index = 0; index < count; ++index)
                {
                    if (!output[index])
                    {
                        failure.store(-1);
                        stop.store(true);
                        break;
                    }
                    const uint64_t identity = output[index]->m_identity;
                    const size_t lane = static_cast<size_t>(identity >> 48);
                    const size_t sequence = static_cast<size_t>(identity & UINT64_C(0x0000FFFFFFFFFFFF));
                    if (lane >= producers || sequence != expected[lane] || sequence >= perProducer)
                    {
                        failure.store(-2);
                        stop.store(true);
                        break;
                    }
                    ++expected[lane];
                    ++consumed;
                }
            }
            // Releasing the last output references is also charged to consumption.
            for (size_t index = 0; index < 64; ++index)
            {
                output[index].reset();
            }
            consumerFinished = now();
            cpuUsed[producers] = cpuTicks() - cpuStarted;
            processorEnd[producers] = processor();
        };
        if (phase == 1)
        {
            threads.m_threads.push_back(std::thread([&]()
            {
                ready.fetch_add(1);
                while (!start.load(std::memory_order_acquire))
                {
                    std::this_thread::yield();
                }
                consume();
            }));
        }
        while (ready.load() != producers + (phase == 1 ? 1 : 0))
        {
            std::this_thread::yield();
        }
        started = now();
        start.store(true, std::memory_order_release);
        threads.join();
        const double producerSeconds = *std::max_element(finished.begin(), finished.end()) - started;
        double consumerStarted = started;
        if (phase == 0)
        {
            consumerStarted = now();
            consume();
        }
        const double consumerSeconds = consumerFinished - consumerStarted;
        const LockFreeQueuePoolStatistics after = queue.poolStatistics();
        bool valid = failure.load() == 0 && consumed == items && queue.empty() && queue.size() == 0 &&
            after.activeProducers == 0 && after.currentCapacity == before.currentCapacity &&
            after.allocationCalls == before.allocationCalls && after.successfulGrowCount == before.successfulGrowCount;
        for (size_t lane = 0; lane < producers; ++lane)
        {
            valid = valid && sent[lane] == perProducer && expected[lane] == perProducer;
        }
        std::cout << "RESULT case=46 phase=" << (phase == 0 ? "separate" : "concurrent") << " items=" << items
            << " producers=" << producers << " producer_seconds=" << producerSeconds
            << " consumer_seconds=" << consumerSeconds << " producer_items_per_second=" << items / producerSeconds
            << " consumer_items_per_second=" << items / consumerSeconds << " empty_polls=" << emptyPolls
            << " pool_allocations=" << after.allocationCalls - before.allocationCalls
            << " pool_grows=" << after.successfulGrowCount - before.successfulGrowCount
            << " cas_retries=" << after.casRetries - before.casRetries
            << " retained_bytes=" << after.retainedBytes << " status=" << failure.load() << " valid=" << valid << std::endl;
        for (size_t lane = 0; lane <= producers; ++lane)
        {
            std::cout << "LANE phase=" << (phase == 0 ? "separate" : "concurrent") << " index=" << lane
                << " consumer=" << (lane == producers) << " cpu_begin=" << processorBegin[lane]
                << " cpu_end=" << processorEnd[lane] << " cpu_milliseconds=" << cpuUsed[lane] / 10000.0 << std::endl;
        }
        if (!valid)
        {
            return 3;
        }
    }
    return 0;
}

int main(int argc, char** argv)
{
    if ((argc != 7 && argc != 8) || std::string(argv[1]) != "46")
    {
        std::cerr << "Usage: benchmark 46 producers items bound prefix distributed [reserved]" << std::endl;
        return 2;
    }
    char* end = nullptr;
    const unsigned long producers = std::strtoul(argv[2], &end, 10);
    if (*end != 0 || producers < 1 || producers > 16)
    {
        return 2;
    }
    const unsigned long items = std::strtoul(argv[3], &end, 10);
    if (*end != 0 || items < producers || items > 20000000 || items % producers != 0)
    {
        return 2;
    }
    for (int index = 4; index < argc; ++index)
    {
        if (std::string(argv[index]) != "0" && std::string(argv[index]) != "1")
        {
            return 2;
        }
    }
    try
    {
        return LockFreeQueueSharedPtrBenchmark::run(producers, items, argv[4][0] == '1', argv[5][0] == '1', argv[6][0] == '1',
            argc == 8 && argv[7][0] == '1');
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL " << error.what() << std::endl;
        return 4;
    }
}