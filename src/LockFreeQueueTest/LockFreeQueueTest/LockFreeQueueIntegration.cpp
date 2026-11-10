#include "LockFreeQueueIntegration.h"
#include "LockFreeQueueInterfaceContract.h"
#include "LockFreeQueueThreadGroup.h"
#include "LockFreeQueueBenchmarkPush.h"
#include <LockFreeMap/LockFreeMap.h>
#include <LockFreeHashMap/LockFreeHashMap.h>
#include <Semaphore/Semaphore.h>
#include <thread>
#include <vector>
#include <iostream>
#include <cstring>

bool LockFreeQueueIntegration::fourHeaders()
{
    LockFreeMap<int32_t, int32_t> ordered;
    LockFreeHashMap<int32_t, int32_t> hashed;
    LockFreeQueue<int32_t> queue;
    LockFreeQueue<std::shared_ptr<int32_t>> shared;
    int32_t a = 0, b = 0;
    queue.push(5);
    const bool passed = ordered.set(1, 7) && ordered.get(1, a) && a == 7 &&
        hashed.set(2, 9) && hashed.get(2, b) && b == 9 && queue.pop(&a) && a == 5;
    shared.push(std::make_shared<int32_t>(8));
    std::shared_ptr<int32_t> value;
    return passed && shared.pop(&value) && value && *value == 8;
}

bool LockFreeQueueIntegration::notification()
{
    LockFreeQueue<uint64_t> queue;
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = 256;
    config.maximumProducers = 8;
    if (queue.configurePool(config) != LockFreeQueueSuccess || queue.tryInit() != LockFreeQueueSuccess)
    {
        return false;
    }
    Semaphore notification;
    std::atomic<bool> stop(false), error(false);
    std::atomic<size_t> done(0);
    const size_t producers = 2, count = 3000;
    LockFreeQueueThreadGroup group(stop);
    std::vector<std::thread>& workers = group.m_threads;
    for (size_t lane = 0; lane < producers; ++lane)
    {
        workers.emplace_back([&, lane]() {
            try
            {
            for (size_t i = 0; i < count && !stop.load(); ++i)
            {
                const uint64_t value = (uint64_t(lane) << 32) | i;
                for (;;)
                {
                    const LockFreeQueueResult result = queue.tryPush(value);
                    if (result == LockFreeQueueSuccess)
                    {
                        // The notification follows the queue commit. Coalesce 32 items.
                        if ((i % 32 == 31 || i + 1 == count) && !notification.trySignal().committed)
                        {
                            error.store(true);
                        }
                        break;
                    }
                    if (stop.load() || (!LockFreeQueueBenchmarkPush::transient(result) && result != LockFreeQueueCapacityLimit))
                    {
                        error.store(true);
                        break;
                    }
                    std::this_thread::yield();
                }
            }
            }
            catch (...)
            {
                error.store(true);
                stop.store(true);
            }
            done.fetch_add(1);
            notification.trySignal(); // Stop wake, allowed to be surplus.
        });
    }
    size_t next[2] = { 0, 0 };
    size_t received = 0;
    bool valid = true;
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while ((done.load() != producers || !queue.empty()) && std::chrono::steady_clock::now() < deadline)
    {
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
        // Always recheck the real queue after a bounded wait, including timeout.
        const SemaphoreResult result = notification.waitResult(5);
        valid = (result.status == SEMAPHORE_OK || result.status == SEMAPHORE_TIMEOUT) && valid;
    }
    stop.store(true);
    notification.trySignal();
    group.join();
    valid = valid && !error.load() && received == producers * count;
    notification.reset();
    notification.trySignal();
    notification.trySignal();
    valid = notification.wait(0) && notification.wait(0) && !notification.wait(0) && queue.empty() && valid;
    // A published item remains discoverable even if its producer has not signalled.
    valid = queue.tryPush(42) == LockFreeQueueSuccess && !notification.wait(5) && valid;
    uint64_t value = 0;
    valid = queue.pop(&value) && value == 42 && valid;
    // Failed enqueue must not manufacture a notification.
    queue.destroy();
    const LockFreeQueueResult rejected = queue.tryPush(43);
    if (rejected == LockFreeQueueSuccess)
    {
        notification.trySignal();
    }
    valid = rejected == LockFreeQueueNotInitialized && !notification.wait(0) && valid;
    std::cout << "NOTIFICATION received=" << received << " coalesced_batch=32 finite_wait_ms=5 valid=" << valid << std::endl;
    return valid;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    try
    {
        const bool known = std::strcmp(argv[1], "9") == 0 || std::strcmp(argv[1], "23") == 0;
        const bool passed = known && (std::strcmp(argv[1], "9") == 0 ?
            LockFreeQueueIntegration::notification() : LockFreeQueueIntegration::fourHeaders());
        std::cout << (passed ? "PASS" : "FAIL") << " id=" << argv[1] << " scope=queue_snapshot_adapter_only" << std::endl;
        return passed ? 0 : 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL exception=" << error.what() << std::endl;
        return 1;
    }
}