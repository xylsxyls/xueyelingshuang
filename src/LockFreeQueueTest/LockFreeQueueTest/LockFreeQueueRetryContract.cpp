#ifdef LOCK_FREE_QUEUE_TESTING
#include "LockFreeQueueTests.h"
#endif
#include "LockFreeQueueRetryContract.h"
#include "LockFreeQueueThreadGroup.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>

static std::atomic<size_t> s_capacityWaits(0);
static std::atomic<bool> s_observeCancelled(false);
static std::atomic<uint32_t> s_cancelledMask(0);
static std::thread::id s_cancelledWorkers[3];
bool LockFreeQueueRetryValue::m_failCopy = false;

/** 仅隔离副本记录实际进入容量等待，不修改队列结果 */
void LockFreeQueueTestCapacityWait()
{
    s_capacityWaits.fetch_add(1);
    if (s_observeCancelled.load())
    {
        for (size_t lane = 0; lane < 3; ++lane)
        {
            if (std::this_thread::get_id() == s_cancelledWorkers[lane])
            {
                s_cancelledMask.fetch_or(uint32_t(1) << lane);
            }
        }
    }
}

LockFreeQueueRetryValue::LockFreeQueueRetryValue(int32_t value) :
m_value(value)
{
}

LockFreeQueueRetryValue::LockFreeQueueRetryValue(const LockFreeQueueRetryValue& other) :
m_value(other.m_value)
{
    if (m_failCopy)
    {
        throw std::runtime_error("payload copy failure");
    }
}

LockFreeQueueRetryObserver::LockFreeQueueRetryObserver(const bool& failCopy) :
m_failCopy(&failCopy)
{
}

LockFreeQueueRetryObserver::LockFreeQueueRetryObserver(const LockFreeQueueRetryObserver& other) :
m_failCopy(other.m_failCopy)
{
    if (*m_failCopy)
    {
        throw std::runtime_error("observer copy failure");
    }
}

void LockFreeQueueRetryObserver::operator()(size_t amount, uint64_t count) const
{
    (void)amount;
    (void)count;
}

bool LockFreeQueueRetryContract::capacity(size_t mode, bool retryContention, bool retryCapacity)
{
    LockFreeQueueEntry<int32_t> entry(2);
    LockFreeQueue<int32_t> queue(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 2;
    config.maximumProducers = 4;
    if (queue.configurePool(config) != LockFreeQueueSuccess || !queue.init() || !queue.push(1))
    {
        return false;
    }
    std::atomic<bool> entered(false), finished(false), stop(false);
    LockFreeQueueResult result = LockFreeQueueInvalidState;
    /** 使用捕获的入口和重试开关提交一项，将状态写入 result 并设置 finished
    调用线程独占局部 Producer；捕获的队列、Entry 和结果存储存活到线程结束
    */
    const auto submit = [&]()
    {
        LockFreeQueueProducer<int32_t> retained;
        entered.store(true);
        if ((mode & 1) && queue.tryAcquireProducer(retained) != LockFreeQueueSuccess)
        {
            finished.store(true);
            return;
        }
        switch (mode)
        {
        case 0:
            result = queue.tryPush(2, retryContention, retryCapacity);
            break;
        case 1:
            result = retained.tryPush(2, retryContention, retryCapacity);
            break;
        case 2:
            result = queue.tryPushAndFence(2, retryContention, retryCapacity);
            break;
        case 3:
            result = retained.tryPushAndFence(2, retryContention, retryCapacity);
            break;
        case 4:
            result = queue.tryPushEntry(entry, retryContention, retryCapacity);
            break;
        case 5:
            result = retained.tryPushEntry(entry, retryContention, retryCapacity);
            break;
        case 6:
            result = queue.tryPushEntryAndFence(entry, retryContention, retryCapacity);
            break;
        case 7:
            result = retained.tryPushEntryAndFence(entry, retryContention, retryCapacity);
            break;
        case 8:
            result = queue.push(2) ? LockFreeQueueSuccess : LockFreeQueueInvalidState;
            break;
        default:
            result = retained.push(2) ? LockFreeQueueSuccess : LockFreeQueueInvalidState;
            break;
        }
        finished.store(true);
    };
    const bool waiting = retryCapacity || mode >= 8;
    int32_t value = 0;
    if (!waiting)
    {
        submit();
        return finished.load() && result == LockFreeQueueCapacityLimit && queue.pop(&value) && value == 1 && queue.empty();
    }
    s_capacityWaits.store(0);
    LockFreeQueueThreadGroup group(stop);
    group.m_threads.reserve(1);
    group.m_threads.emplace_back(submit);
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
#ifdef LOCK_FREE_QUEUE_TESTING
    while (!s_capacityWaits.load() && !finished.load() && std::chrono::steady_clock::now() < deadline)
#else
    while (!entered.load() && !finished.load() && std::chrono::steady_clock::now() < deadline)
#endif
    {
        std::this_thread::yield();
    }
    bool valid = !finished.load();
#ifdef LOCK_FREE_QUEUE_TESTING
    valid = s_capacityWaits.load() != 0 && valid;
#endif
    valid = queue.pop(&value) && value == 1 && valid;
    group.join();
    return valid && result == LockFreeQueueSuccess && queue.pop(&value) && value == 2 && queue.empty();
}

bool LockFreeQueueRetryContract::delay()
{
    uint32_t retries = 0;
#ifdef LOCK_FREE_QUEUE_TESTING
    LockFreeQueueTestControl::reset();
#endif
    for (uint32_t i = 0; i < 4096; ++i)
    {
        if (!LockFreeQueueProgress::waitCapacity(retries) || retries != i + 1)
        {
            return false;
        }
    }
#ifdef LOCK_FREE_QUEUE_TESTING
    if (LockFreeQueueTestControl::m_scheduleSleeps.load() != 0 ||
        LockFreeQueueTestControl::m_scheduleYields.load() != 256)
    {
        return false;
    }
#endif
    for (size_t i = 0; i < 8; ++i)
    {
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
        if (LockFreeQueueProgress::waitCapacity(retries) || retries != 4096)
        {
            return false;
        }
#else
        const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        if (!LockFreeQueueProgress::waitCapacity(retries) || retries != 4096 ||
            std::chrono::steady_clock::now() - begin < std::chrono::milliseconds(1))
        {
            return false;
        }
#ifdef LOCK_FREE_QUEUE_TESTING
        if (LockFreeQueueTestControl::m_scheduleSleeps.load() != i + 1)
        {
            return false;
        }
#endif
#endif
    }
    return true;
}

bool LockFreeQueueRetryContract::concurrent()
{
    // 在线程已创建、尚未开始调用库时模拟后续创建失败；必须取消并正常join。
    std::atomic<bool> cancelled(false), released(false);
    std::atomic<size_t> exited(0);
    bool caught = false;
    try
    {
        LockFreeQueueThreadGroup partial(cancelled, &released);
        partial.m_threads.reserve(4);
        partial.m_threads.emplace_back([&]()
        {
            while (!released.load())
            {
                std::this_thread::yield();
            }
            if (cancelled.load())
            {
                exited.fetch_add(1);
            }
        });
        throw std::runtime_error("injected later thread creation failure");
    }
    catch (const std::runtime_error&)
    {
        caught = true;
    }
    if (!caught || exited.load() != 1)
    {
        return false;
    }
    LockFreeQueue<int32_t> queue(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = 64;
    config.maximumProducers = 8;
    if (queue.configurePool(config) != LockFreeQueueSuccess || !queue.init())
    {
        return false;
    }
    // 创建失败先取消并放行启动门；线程尚未进入可能等待容量的push。
    std::atomic<bool> stop(false), start(false), failed(false);
    std::atomic<size_t> completed(0);
    LockFreeQueueThreadGroup group(stop, &start);
    group.m_threads.reserve(4);
    for (int32_t lane = 0; lane < 4; ++lane)
    {
        group.m_threads.emplace_back([&, lane]()
        {
            while (!start.load())
            {
                std::this_thread::yield();
            }
            for (int32_t i = 0; i < 2000 && !stop.load(); ++i)
            {
                if (!queue.push(lane * 2000 + i))
                {
                    failed.store(true);
                    break;
                }
            }
            completed.fetch_add(1);
        });
    }
    int32_t next[4] = {0, 0, 0, 0};
    size_t consumed = 0;
    start.store(true);
    while (completed.load() != 4 || !queue.empty())
    {
        int32_t value = 0;
        if (queue.pop(&value))
        {
            if (value < 0 || value >= 8000 || value % 2000 != next[value / 2000]++)
            {
                failed.store(true);
            }
            ++consumed;
        }
        else
        {
            std::this_thread::yield();
        }
    }
    group.join();
    return !failed.load() && consumed == 8000;
}

bool LockFreeQueueRetryContract::exceptions()
{
    LockFreeQueue<LockFreeQueueRetryValue> queue;
    LockFreeQueueProducer<LockFreeQueueRetryValue> producer;
    if (queue.tryAcquireProducer(producer) != LockFreeQueueSuccess)
    {
        return false;
    }
    LockFreeQueueRetryValue value(42);
    bool valid = true;
    LockFreeQueueRetryValue::m_failCopy = true;
    for (size_t flags = 0; flags < 4; ++flags)
    {
        const bool contention = (flags & 1) != 0, capacity = (flags & 2) != 0;
        valid = queue.tryPush(value, contention, capacity) == LockFreeQueueConstructionFailure && valid;
        valid = producer.tryPush(value, contention, capacity) == LockFreeQueueConstructionFailure && valid;
        valid = queue.tryPushAndFence(value, contention, capacity) == LockFreeQueueConstructionFailure && valid;
        valid = producer.tryPushAndFence(value, contention, capacity) == LockFreeQueueConstructionFailure && valid;
    }
    valid = !queue.push(value) && !producer.push(value) && queue.empty() && valid;
    LockFreeQueueRetryValue::m_failCopy = false;
    return queue.push(value) && queue.pop(&value) && value.m_value == 42 && queue.empty() && valid;
}

bool LockFreeQueueRetryContract::growth()
{
    size_t calls = 0, slots = 0;
    uint64_t last = 0;
    LockFreeQueue<int32_t> queue(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 8;
    config.growThreshold = 0;
    config.m_onGrowth = [&](size_t amount, uint64_t count)
    {
        ++calls;
        slots += amount;
        last = count;
        throw std::runtime_error("observer failure after committed growth");
    };
    if (queue.configurePool(config) != LockFreeQueueSuccess || !queue.init() || calls != 0 || !queue.push(0))
    {
        return false;
    }
    queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(0); });
    if (queue.tryPush(1, true, false) != LockFreeQueueGrowthDeclined || calls != 0)
    {
        return false;
    }
    queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(2); });
#ifdef LOCK_FREE_QUEUE_TESTING
    LockFreeQueueTestControl::reset();
    LockFreeQueueTestControl::m_failPoint.store(11);
    const LockFreeQueueResult allocation = queue.tryPush(1, true, false);
    LockFreeQueueTestControl::reset();
    if (allocation != LockFreeQueueAllocationFailure || calls != 0 || queue.poolStatistics().currentCapacity != 2)
    {
        return false;
    }
#endif
    for (int32_t i = 1; i < 7; ++i)
    {
        if (queue.tryPush(i, true, false) != LockFreeQueueSuccess)
        {
            return false;
        }
    }
    if (calls != 3 || slots != 6 || last != 3 || queue.poolStatistics().currentCapacity != 8 ||
        queue.tryPush(7, true, false) != LockFreeQueueCapacityLimit || calls != 3)
    {
        return false;
    }
    for (int32_t i = 0; i < 7; ++i)
    {
        int32_t value = -1;
        if (!queue.pop(&value) || value != i)
        {
            return false;
        }
    }
    if (!queue.push(9) || !queue.pop(nullptr) || calls != 3)
    {
        return false;
    }
    queue.destroy();
    bool copyFailure = false;
    LockFreeQueuePoolConfig candidate(config);
    candidate.initialCapacity = 4;
    candidate.m_onGrowth = LockFreeQueueRetryObserver(copyFailure);
    copyFailure = true;
    const LockFreeQueueResult configured = queue.configurePool(candidate);
    copyFailure = false;
    if (configured != LockFreeQueueAllocationFailure || !queue.init() || queue.poolStatistics().currentCapacity != 2)
    {
        return false;
    }
    // 原观察者和旧容量仍生效，初始分配不通知。
    return calls == 3 && queue.push(1) && queue.push(2) && calls == 4;
}

bool LockFreeQueueRetryContract::growthConcurrent()
{
    std::atomic<size_t> calls(0), slots(0);
    std::atomic<bool> stop(false), start(false), failed(false);
    LockFreeQueue<int32_t> queue(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 512;
    config.maximumProducers = 8;
    config.m_onGrowth = [&](size_t amount, uint64_t count)
    {
        calls.fetch_add(1);
        slots.fetch_add(amount);
        if (amount == 0 || count == 0)
        {
            failed.store(true);
        }
    };
    if (queue.configurePool(config) != LockFreeQueueSuccess || !queue.init() || calls.load() != 0 ||
        queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(8); }) != LockFreeQueueSuccess)
    {
        return false;
    }
    LockFreeQueueThreadGroup group(stop, &start);
    group.m_threads.reserve(4);
    for (int32_t lane = 0; lane < 4; ++lane)
    {
        group.m_threads.emplace_back([&, lane]()
        {
            while (!start.load())
            {
                std::this_thread::yield();
            }
            for (int32_t i = 0; i < 64 && !stop.load(); ++i)
            {
                if (queue.tryPush(lane * 64 + i, true, false) != LockFreeQueueSuccess)
                {
                    failed.store(true);
                    break;
                }
            }
        });
    }
    start.store(true);
    group.join();
    const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
    if (failed.load() || calls.load() == 0 || calls.load() != stats.successfulGrowCount || slots.load() != stats.currentCapacity - 2)
    {
        return false;
    }
    int32_t next[4] = {0, 0, 0, 0};
    for (size_t i = 0; i < 256; ++i)
    {
        int32_t value = -1;
        if (!queue.pop(&value) || value < 0 || value >= 256 || value % 64 != next[value / 64]++)
        {
            return false;
        }
    }
    return queue.empty();
}

bool LockFreeQueueRetryContract::registration()
{
    for (size_t flags = 0; flags < 4; ++flags)
    {
        LockFreeQueue<int32_t> queue(false);
        LockFreeQueuePoolConfig config;
        config.initialCapacity = config.maximumCapacity = 8;
        config.maximumProducers = 1;
        if (queue.configurePool(config) != LockFreeQueueSuccess || !queue.init())
        {
            return false;
        }
        LockFreeQueueProducer<int32_t> held;
        if (queue.tryAcquireProducer(held) != LockFreeQueueSuccess)
        {
            return false;
        }
        if ((flags & 2) == 0)
        {
            LockFreeQueueProducer<int32_t> other;
            if (queue.tryAcquireProducer(other, (flags & 1) != 0, false) != LockFreeQueueCapacityLimit || other.attached())
            {
                return false;
            }
            continue;
        }
        std::atomic<bool> stop(false), entered(false), finished(false);
        LockFreeQueueResult result = LockFreeQueueInvalidState;
        s_capacityWaits.store(0);
        LockFreeQueueThreadGroup group(stop);
        group.m_threads.reserve(1);
        group.m_threads.emplace_back([&]()
        {
            LockFreeQueueProducer<int32_t> other;
            entered.store(true);
            result = queue.tryAcquireProducer(other, (flags & 1) != 0, true);
            finished.store(true);
        });
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
#ifdef LOCK_FREE_QUEUE_TESTING
        while (!s_capacityWaits.load() && !finished.load() && std::chrono::steady_clock::now() < deadline)
#else
        while (!entered.load() && !finished.load() && std::chrono::steady_clock::now() < deadline)
#endif
        {
            std::this_thread::yield();
        }
        bool valid = !finished.load();
#ifdef LOCK_FREE_QUEUE_TESTING
        valid = s_capacityWaits.load() != 0 && valid;
#endif
        held.release();
        group.join();
        if (!valid || result != LockFreeQueueSuccess || queue.poolStatistics().activeProducers != 0)
        {
            return false;
        }
    }
    return true;
}

bool LockFreeQueueRetryContract::prefix()
{
    for (size_t mode = 0; mode < 4; ++mode)
    {
        std::unique_ptr<LockFreeQueueEntry<int32_t>[]> entries(new LockFreeQueueEntry<int32_t>[64]);
        LockFreeQueue<int32_t> queue(false);
        LockFreeQueuePoolConfig config;
        config.initialCapacity = (mode & 1) ? 7 : 65;
        config.maximumCapacity = (mode & 1) ? 129 : 65;
        if (queue.configurePool(config) != LockFreeQueueSuccess || queue.configureEntryOnly(true) != LockFreeQueueSuccess ||
            queue.configureCancelableReservations((mode & 2) != 0) != LockFreeQueueSuccess || !queue.init())
        {
            return false;
        }
        const size_t budgets[] = {1, 3, 4, 7, 64, 65};
        for (size_t round = 0; round < 30; ++round)
        {
            for (size_t i = 0; i < 64; ++i)
            {
                entries[i].m_value = static_cast<int32_t>(i);
                if (queue.tryPushEntry(entries[i], true, false) != LockFreeQueueSuccess)
                {
                    return false;
                }
            }
            size_t count = 99;
            LockFreeQueueEntry<int32_t>* output[65];
            if (queue.probeReadyEntryPrefix(output, 0, count) || count != 0)
            {
                return false;
            }
            size_t consumed = 0, step = 0;
            while (consumed < 64)
            {
                const size_t limit = budgets[step++ % 6];
                for (size_t i = 0; i < 65; ++i)
                {
                    output[i] = nullptr;
                }
                if (!queue.probeReadyEntryPrefix(output, limit, count) || count == 0 || count > limit ||
                    count > 64 - consumed || output[count] != nullptr)
                {
                    return false;
                }
                for (size_t i = 0; i < count; ++i)
                {
                    if (output[i] != &entries[consumed + i] || output[i]->m_value != static_cast<int32_t>(consumed + i))
                    {
                        return false;
                    }
                }
                consumed += count;
            }
            if (!queue.empty() || queue.probeReadyEntryPrefix(nullptr, 64, count) || count != 0 ||
                queue.tryPushEntry(entries[0], true, false) != LockFreeQueueSuccess ||
                !queue.probeReadyEntryPrefix(nullptr, 64, count) || count != 1 || !queue.empty())
            {
                return false;
            }
        }
    }
    LockFreeQueue<int32_t> ordinary;
    size_t count = 99;
    return ordinary.push(42) && !ordinary.probeReadyEntryPrefix(nullptr, 64, count) && count == 0 && ordinary.pop(nullptr);
}

bool LockFreeQueueRetryContract::reservationProgress()
{
#ifdef LOCK_FREE_QUEUE_TESTING
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 1024;
    config.maximumCapacity = 1024;
    config.maximumProducers = 4;
    LockFreeQueue<int32_t> sealed(false);
    if (sealed.configurePool(config) != LockFreeQueueSuccess ||
        sealed.configureCancelableReservations(true) != LockFreeQueueSuccess || !sealed.init())
    {
        return false;
    }
    int32_t value = -1;
    if (sealed.pop(&value) || !sealed.push(0) || !sealed.pop(&value) || value != 0)
    {
        return false;
    }
    for (int32_t index = 1; index <= 32; ++index)
    {
        // Invoke the real seal protocol, including its pending-seal marker.
        // A single registered publisher can then publish in the successor.
        if (!sealed.testSealHead() || !sealed.push(index) || !sealed.pop(&value) ||
            value != index || sealed.pop(nullptr) || !sealed.empty())
        {
            return false;
        }
    }
    if (!sealed.init() || !sealed.push(55) || !sealed.pop(&value) || value != 55 || !sealed.empty())
    {
        return false;
    }
    std::cout << "RESERVED_EMPTY seals=32 reinitialized=1 valid=1" << std::endl;
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    for (size_t phase = 0; phase < 2; ++phase)
    {
        LockFreeQueueTestControl::reset();
        g_lockFreeQueueTestLane = 0;
        LockFreeQueue<int32_t> queue(false);
        if (queue.configurePool(config) != LockFreeQueueSuccess ||
            queue.configureCancelableReservations(true) != LockFreeQueueSuccess || !queue.init() ||
            !queue.push(0) || !queue.pop(&value) || value != 0)
        {
            return false;
        }
        LockFreeQueueProducer<int32_t> firstLease, secondLease;
        if (queue.tryAcquireProducer(firstLease) != LockFreeQueueSuccess ||
            queue.tryAcquireProducer(secondLease) != LockFreeQueueSuccess)
        {
            return false;
        }
        firstLease.release();
        secondLease.release();
        LockFreeQueueTestControl::m_pausePoint.store(34);
        std::atomic<int32_t> firstResult(-1);
        std::atomic<bool> stop(false);
        LockFreeQueueThreadGroup threads(stop, &LockFreeQueueTestControl::m_release);
        // Pause the owning thread after reservation and before its publication CAS.
        threads.m_threads.emplace_back([&]()
        {
            g_lockFreeQueueTestLane = 1;
            try
            {
                firstResult.store(queue.tryPush(111, true, false));
            }
            catch (...)
            {
                firstResult.store(-2);
            }
        });
        bool valid = LockFreeQueueTestControl::waitReached();
        value = -1;
        valid = !queue.pop(&value) && value == -1 && valid;
        if (phase != 0)
        {
            // The general reserved path must reach222 while111 remains paused.
            const LockFreeQueueResult secondResult = queue.tryPush(222, true, false);
            valid = secondResult == LockFreeQueueSuccess && queue.pop(&value) && value == 222 && valid;
        }
        LockFreeQueueTestControl::m_release.store(true);
        threads.join();
        valid = !LockFreeQueueTestControl::m_timedOut.load() && firstResult.load() == LockFreeQueueSuccess &&
            queue.pop(&value) && value == 111 && !queue.pop(nullptr) && queue.empty() && valid;
        LockFreeQueueTestControl::reset();
        std::cout << "RESERVED_EMPTY phase=" << phase << " second_publisher=" << (phase != 0)
                  << " valid=" << valid << std::endl;
        if (!valid)
        {
            return false;
        }
    }
#endif
#endif
    return true;
}

bool LockFreeQueueRetryContract::cancelledConsumer()
{
    for (size_t mode = 0; mode < 2; ++mode)
    {
        LockFreeQueue<int32_t> queue(false);
        LockFreeQueuePoolConfig config;
        config.initialCapacity = config.maximumCapacity = 2;
        config.maximumProducers = 4;
        if (queue.configurePool(config) != LockFreeQueueSuccess || !queue.init() || !queue.push(1))
        {
            return false;
        }
        std::atomic<bool> stop(false), start(false);
        std::atomic<size_t> entered(0), completed(0), accepted(0);
        LockFreeQueueThreadGroup workers(stop, &start);
        workers.m_threads.reserve(3);
        s_capacityWaits.store(0);
        s_cancelledMask.store(0);
        // Create behind the gate. A partial creation failure exits before any blocking call.
        try
        {
            for (size_t lane = 0; lane < 3; ++lane)
            {
                workers.m_threads.emplace_back([&, lane]()
                {
                    while (!start.load()) { std::this_thread::yield(); }
                    if (!stop.load())
                    {
                        entered.fetch_add(1);
                        const bool result = mode == 0 ? queue.push(static_cast<int32_t>(lane + 10)) :
                            queue.tryPush(static_cast<int32_t>(lane + 10), true, true) == LockFreeQueueSuccess;
                        if (result) { accepted.fetch_add(1); }
                    }
                    completed.fetch_add(1);
                });
                s_cancelledWorkers[lane] = workers.m_threads.back().get_id();
            }
        }
        catch (...)
        {
            workers.cancelAndDrain(queue, completed);
            return false;
        }
        s_observeCancelled.store(true);
        start.store(true);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (s_cancelledMask.load() != 7 &&
            std::chrono::steady_clock::now() < deadline)
        {
            std::this_thread::yield();
        }
        const bool waiting = entered.load() == 3 && s_cancelledMask.load() == 7 && completed.load() == 0;
        // Simulated consumer validation failure: accepted items must be discarded during cancellation.
        workers.cancelAndDrain(queue, completed);
        s_observeCancelled.store(false);
        if (!waiting || !stop.load() || completed.load() != 3 || accepted.load() != 3 || !queue.empty())
        {
            return false;
        }
    }
    return true;
}

bool LockFreeQueueRetryContract::valuePrefix()
{
    const size_t bursts[] = {1, 2, 3, 4, 7, 31, 64, 65, 257};
    const size_t budgets[] = {1, 2, 3, 4, 7, 64, 65};
    for (size_t mode = 0; mode < 4; ++mode)
    {
        LockFreeQueue<int32_t> queue(false);
        LockFreeQueuePoolConfig config;
        config.initialCapacity = (mode & 1) ? 7 : 512;
        config.maximumCapacity = (mode & 1) ? 1024 : 512;
        config.maximumProducers = 4;
        if (queue.configurePool(config) != LockFreeQueueSuccess ||
            queue.configureCancelableReservations((mode & 2) != 0) != LockFreeQueueSuccess || !queue.init())
        {
            return false;
        }
        for (size_t round = 0; round < 3; ++round)
        {
            for (size_t burst = 0; burst < sizeof(bursts) / sizeof(bursts[0]); ++burst)
            {
                for (size_t budget = 0; budget < sizeof(budgets) / sizeof(budgets[0]); ++budget)
                {
                    for (size_t item = 0; item < bursts[burst]; ++item)
                    {
                        if (queue.tryPush(static_cast<int32_t>(item), true, false) != LockFreeQueueSuccess)
                        {
                            return false;
                        }
                    }
                    size_t count = 99;
                    if (queue.popReadyPrefix(nullptr, 0, count) || count != 0 || queue.size() != bursts[burst])
                    {
                        return false;
                    }
                    size_t consumed = 0;
                    while (consumed < bursts[burst])
                    {
                        int32_t output[65];
                        for (size_t index = 0; index < 65; ++index)
                        {
                            output[index] = -1;
                        }
                        if (!queue.popReadyPrefix(output, budgets[budget], count) || count == 0 ||
                            count > budgets[budget] || count > 64 || count > bursts[burst] - consumed)
                        {
                            return false;
                        }
                        for (size_t index = 0; index < 65; ++index)
                        {
                            const int32_t expected = index < count ? static_cast<int32_t>(consumed + index) : -1;
                            if (output[index] != expected)
                            {
                                return false;
                            }
                        }
                        consumed += count;
                    }
                    if (!queue.empty() || queue.popReadyPrefix(nullptr, 64, count) || count != 0 ||
                        queue.tryPush(42, true, false) != LockFreeQueueSuccess ||
                        !queue.popReadyPrefix(nullptr, 64, count) || count != 1 || !queue.empty())
                    {
                        return false;
                    }
                }
            }
        }
    }
#ifdef LOCK_FREE_QUEUE_TESTING
    const int32_t liveBefore = LockFreeQueueTestValue::m_live.load();
    const bool oldCopy = LockFreeQueueTestValue::m_throwCopy;
    const bool oldAssign = LockFreeQueueTestValue::m_throwAssign;
    bool valid = true;
    LockFreeQueueTestValue::m_throwCopy = false;
    LockFreeQueueTestValue::m_throwAssign = false;
    try
    {
        for (size_t mode = 0; mode < 4; ++mode)
        {
            LockFreeQueue<LockFreeQueueTestValue> queue(false);
            LockFreeQueuePoolConfig config;
            config.initialCapacity = (mode & 1) ? 7 : 512;
            config.maximumCapacity = (mode & 1) ? 1024 : 512;
            LockFreeQueueTestValue input(321);
            LockFreeQueueTestValue output[2] = {LockFreeQueueTestValue(-1), LockFreeQueueTestValue(-2)};
            if (queue.configurePool(config) != LockFreeQueueSuccess ||
                queue.configureCancelableReservations((mode & 2) != 0) != LockFreeQueueSuccess ||
                !queue.init() || !queue.push(input))
            {
                valid = false;
                break;
            }
            size_t count = 99;
            bool threw = false;
            LockFreeQueueTestValue::m_throwAssign = true;
            try
            {
                queue.popReadyPrefix(output, 2, count);
            }
            catch (const std::runtime_error&)
            {
                threw = true;
            }
            LockFreeQueueTestValue::m_throwAssign = false;
            valid = threw && count == 0 && queue.size() == 1 && output[0].m_value == -999 &&
                output[1].m_value == -2 && valid;
            valid = queue.popReadyPrefix(output, 2, count) && count == 1 && output[0].m_value == 321 &&
                output[1].m_value == -2 && queue.empty() && valid;
        }
    }
    catch (...)
    {
        valid = false;
    }
    LockFreeQueueTestValue::m_throwCopy = oldCopy;
    LockFreeQueueTestValue::m_throwAssign = oldAssign;
    if (!valid || LockFreeQueueTestValue::m_live.load() != liveBefore)
    {
        return false;
    }
#endif
    std::cout << "VALUE_PREFIX modes=4 bursts=9 budgets=7 rounds=3 valid=1" << std::endl;
    return true;
}

bool LockFreeQueueRetryContract::publisherCapacity()
{
    const size_t capacities[] = {1, 1048575, 1048576, 1048577, 2097151, 2097152, 2097153, 4194305};
    for (size_t mode = 0; mode != 2; ++mode)
    {
        for (size_t index = 0; index != sizeof(capacities) / sizeof(capacities[0]); ++index)
        {
            LockFreeQueueArena<int32_t> arena;
            LockFreeQueuePoolConfig config;
            config.initialCapacity = capacities[index];
            config.maximumCapacity = mode == 0 ? static_cast<int64_t>(capacities[index]) : -1;
            config.maximumProducers = 4;
            if (arena.configure(config) != LockFreeQueueSuccess || arena.initialize() != LockFreeQueueSuccess)
            {
                return false;
            }
            LockFreeQueueArena<int32_t>::Record* records[4] = {};
            bool valid = true;
            size_t acquired = 0;
            for (size_t count = 0; count <= 4; ++count)
            {
                const size_t participants = count == 0 ? 1 : count;
                const bool expected = arena.statistics().reusableFreeCapacity / participants >= 1048576;
                valid = (arena.amplePublisherCapacity() == expected) && valid;
                if (count != 4)
                {
                    LockFreeQueueResult result = LockFreeQueueSuccess;
                    records[count] = arena.enter(false, &result);
                    if (records[count] == nullptr || result != LockFreeQueueSuccess)
                    {
                        valid = false;
                        break;
                    }
                    ++acquired;
                }
            }
            for (size_t count = 0; count < acquired; ++count)
            {
                arena.leave(*records[count], true);
            }
            // Registration history remains monotone even after the leases end.
            const size_t participants = acquired == 0 ? 1 : acquired;
            valid = (arena.amplePublisherCapacity() == (arena.statistics().reusableFreeCapacity / participants >= 1048576)) && valid;
            if (!valid || arena.statistics().activeProducers != 0)
            {
                return false;
            }
        }
    }
    std::cout << "PUBLISHER_CAPACITY modes=2 capacities=8 registered=0..4 valid=1" << std::endl;
    return true;
}

bool LockFreeQueueRetryContract::solePublisherTransition()
{
#if defined(LOCK_FREE_QUEUE_TESTING) && (!defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__))
    const int32_t warmups[] = {0, 1, 15, 16, 255, 256, 257};
    for (size_t counters = 0; counters < 2; ++counters)
    {
        for (size_t warm = 0; warm < sizeof(warmups) / sizeof(warmups[0]); ++warm)
        {
            for (int32_t point = 2; point <= 3; ++point)
            {
                for (size_t joining = 0; joining < 2; ++joining)
                {
                    LockFreeQueueTestControl::reset();
                    g_lockFreeQueueTestLane = 0;
                    LockFreeQueue<int32_t> queue(false);
                    LockFreeQueuePoolConfig config;
                    config.initialCapacity = config.maximumCapacity = 1024;
                    config.maximumProducers = 4;
                    if (queue.configurePool(config) != LockFreeQueueSuccess ||
                        queue.configureProducerCounters(counters != 0) != LockFreeQueueSuccess ||
                        queue.configureCancelableReservations(true) != LockFreeQueueSuccess || !queue.init())
                    {
                        return false;
                    }
                    int32_t value = -1;
                    for (int32_t index = 0; index < warmups[warm]; ++index)
                    {
                        if (!queue.push(index) || !queue.pop(&value) || value != index)
                        {
                            return false;
                        }
                    }
                    LockFreeQueueTestControl::m_pausePoint.store(point);
                    std::atomic<int32_t> firstResult(-1);
                    std::atomic<bool> stop(false);
                    LockFreeQueueThreadGroup threads(stop, &LockFreeQueueTestControl::m_release);
                    // Point2 is before the owner's slot CAS; point3 is after its LP.
                    threads.m_threads.emplace_back([&]()
                    {
                        g_lockFreeQueueTestLane = 1;
                        try
                        {
                            firstResult.store(queue.tryPush(111, true, false));
                        }
                        catch (...)
                        {
                            firstResult.store(-2);
                        }
                    });
                    bool valid = LockFreeQueueTestControl::waitReached();
                    if (joining != 0)
                    {
                        valid = queue.tryPush(222, true, false) == LockFreeQueueSuccess && valid;
                    }
                    if (point == 3)
                    {
                        valid = queue.pop(&value) && value == 111 && valid;
                    }
                    if (joining != 0)
                    {
                        valid = queue.pop(&value) && value == 222 && valid;
                    }
                    valid = !queue.pop(nullptr) && firstResult.load() == -1 && valid;
                    LockFreeQueueTestControl::m_release.store(true);
                    threads.join();
                    valid = !LockFreeQueueTestControl::m_timedOut.load() &&
                        firstResult.load() == LockFreeQueueSuccess && valid;
                    if (point == 2)
                    {
                        valid = queue.pop(&value) && value == 111 && valid;
                    }
                    valid = !queue.pop(nullptr) && queue.empty() && valid;
                    LockFreeQueueTestControl::reset();
                    if (!valid)
                    {
                        std::cout << "SOLE_TRANSITION counters=" << counters << " warm=" << warmups[warm]
                            << " point=" << point << " joining=" << joining << " valid=0" << std::endl;
                        return false;
                    }
                }
            }
        }
    }
    std::cout << "SOLE_TRANSITION scenarios=56 valid=1" << std::endl;
#endif
    return true;
}

bool LockFreeQueueRetryContract::solePublisherHole()
{
#if defined(LOCK_FREE_QUEUE_TESTING) && (!defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__))
    const int32_t warmups[] = {0, 16, 256};
    for (size_t counters = 0; counters < 2; ++counters)
    {
        for (size_t warm = 0; warm < sizeof(warmups) / sizeof(warmups[0]); ++warm)
        {
            LockFreeQueueTestControl::reset();
            g_lockFreeQueueTestLane = 0;
            LockFreeQueue<int32_t> queue(false);
            LockFreeQueuePoolConfig config;
            config.initialCapacity = config.maximumCapacity = 1024;
            config.maximumProducers = 4;
            if (queue.configurePool(config) != LockFreeQueueSuccess ||
                queue.configureProducerCounters(counters != 0) != LockFreeQueueSuccess ||
                queue.configureCancelableReservations(true) != LockFreeQueueSuccess || !queue.init())
            {
                return false;
            }
            int32_t value = -1;
            for (int32_t index = 0; index < warmups[warm]; ++index)
            {
                if (!queue.push(index) || !queue.pop(&value) || value != index)
                {
                    return false;
                }
            }
            std::atomic<int32_t> firstResult(-1);
            std::atomic<int32_t> secondResult(-1);
            std::atomic<bool> stop(false);
            LockFreeQueueThreadGroup first(stop, &LockFreeQueueTestControl::m_release);
            LockFreeQueueThreadGroup second(stop, &LockFreeQueueTestControl::m_secondRelease);
            LockFreeQueueTestControl::m_pausePoint.store(1);
            first.m_threads.emplace_back([&]()
            {
                g_lockFreeQueueTestLane = 1;
                try
                {
                    firstResult.store(queue.tryPush(111, true, false));
                }
                catch (...)
                {
                    firstResult.store(-2);
                }
            });
            bool valid = LockFreeQueueTestControl::waitReached();
            LockFreeQueueTestControl::m_secondPause.store(true);
            second.m_threads.emplace_back([&]()
            {
                g_lockFreeQueueTestLane = 2;
                try
                {
                    secondResult.store(queue.tryPush(222, true, false));
                }
                catch (...)
                {
                    secondResult.store(-2);
                }
            });
            const uint64_t deadline = LockFreeQueueTestControl::ticks() + 3000000000ULL;
            while (!LockFreeQueueTestControl::m_secondReached.load() && LockFreeQueueTestControl::ticks() < deadline)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            valid = LockFreeQueueTestControl::m_secondReached.load() && valid;
            LockFreeQueueTestControl::m_release.store(true);
            first.join();
            valid = firstResult.load() == LockFreeQueueSuccess && secondResult.load() == -1 && valid;
            valid = queue.pop(&value) && value == 111 && valid;
            valid = !queue.pop(nullptr) && secondResult.load() == -1 && valid;
            LockFreeQueueTestControl::m_secondRelease.store(true);
            second.join();
            valid = secondResult.load() == LockFreeQueueSuccess &&
                !LockFreeQueueTestControl::m_timedOut.load() && valid;
            valid = queue.pop(&value) && value == 222 && valid;
            valid = !queue.pop(nullptr) && queue.empty() && valid;
            LockFreeQueueTestControl::reset();
            if (!valid)
            {
                std::cout << "SOLE_HOLE counters=" << counters << " warm=" << warmups[warm] << " valid=0" << std::endl;
                return false;
            }
        }
    }
    std::cout << "SOLE_HOLE scenarios=6 valid=1" << std::endl;
#endif
    return true;
}

bool LockFreeQueueRetryContract::reservationReuse()
{
    const size_t recordBytes = sizeof(LockFreeQueueArenaRecord<int32_t>);
    if (recordBytes != (sizeof(void*) == 4 ? 64 : 128))
    {
        std::cout << "RESERVATION_REUSE record_bytes=" << recordBytes << " valid=0" << std::endl;
        return false;
    }
    for (size_t counters = 0; counters != 2; ++counters)
    {
        LockFreeQueue<int32_t> queue(false);
        LockFreeQueuePoolConfig config;
        config.initialCapacity = config.maximumCapacity = 65;
        config.maximumProducers = 4;
        if (queue.configurePool(config) != LockFreeQueueSuccess ||
            queue.configureProducerCounters(counters != 0) != LockFreeQueueSuccess ||
            queue.configureCancelableReservations(true) != LockFreeQueueSuccess)
        {
            return false;
        }
        for (int32_t cycle = 0; cycle != 16; ++cycle)
        {
            if (queue.tryInit() != LockFreeQueueSuccess)
            {
                return false;
            }
            int32_t output = -1;
            // A newly initialized first record must remain usable before joining.
            if (!queue.push(cycle) || !queue.pop(&output) || output != cycle)
            {
                return false;
            }
            LockFreeQueueProducer<int32_t> first, second;
            if (queue.tryAcquireProducer(first) != LockFreeQueueSuccess ||
                queue.tryAcquireProducer(second) != LockFreeQueueSuccess)
            {
                return false;
            }
            for (int32_t round = 0; round != 64; ++round)
            {
                const int32_t value = cycle * 10000 + round * 3;
                if (!first.push(value) || !second.push(value + 1) ||
                    !queue.pop(&output) || output != value ||
                    !queue.pop(&output) || output != value + 1)
                {
                    return false;
                }
                // Release and reacquire a real slot; no internal state is seeded.
                first.release();
                if (!queue.push(value + 2) || !queue.pop(&output) || output != value + 2 ||
                    queue.tryAcquireProducer(first) != LockFreeQueueSuccess)
                {
                    return false;
                }
            }
            first.release();
            second.release();
            if (queue.pop(nullptr) || !queue.empty() || queue.size() != 0 ||
                queue.poolStatistics().activeProducers != 0)
            {
                return false;
            }
        }
    }
    std::cout << "RESERVATION_REUSE counters=2 cycles=16 rounds=64 record_bytes=" << recordBytes << " valid=1" << std::endl;
    return true;
}

bool LockFreeQueueRetryContract::run()
{
    if (!publisherCapacity() || !reservationReuse())
    {
        return false;
    }
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    LockFreeQueue<int32_t> full(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = config.maximumCapacity = 2;
    if (full.configurePool(config) != LockFreeQueueSuccess || !full.init() || !full.push(1) ||
        full.tryPush(2, true, false) != LockFreeQueueCapacityLimit ||
        full.tryPush(2, true, true) != LockFreeQueueResourceUnavailable || full.push(2))
    {
        return false;
    }
#else
    for (size_t mode = 0; mode < 10; ++mode)
    {
        for (size_t flags = 0; flags < 4; ++flags)
        {
            if (!capacity(mode, (flags & 1) != 0, (flags & 2) != 0))
            {
                std::cout << "FAIL capacity mode=" << mode << " flags=" << flags << std::endl;
                return false;
            }
        }
    }
    if (!concurrent() || !registration() || !growthConcurrent() || !cancelledConsumer())
    {
        return false;
    }
#endif
    LockFreeQueue<int32_t> uninitialized(false);
    if (uninitialized.push(1) || uninitialized.tryPush(1, true, true) != LockFreeQueueNotInitialized)
    {
        return false;
    }
    return delay() && exceptions() && growth() && prefix() && reservationProgress() && valuePrefix() && solePublisherTransition() && solePublisherHole();
}

#ifdef LOCK_FREE_QUEUE_TESTING
#define LFQ_NEW_API_CHECK(expression) \
    do \
    { \
        if (!(expression)) \
        { \
            std::cerr << "FAIL new_api line=" << __LINE__ << " condition=" << #expression << std::endl; \
            return false; \
        } \
    } while (0)

bool LockFreeQueueRetryContract::consumerYield()
{
    const uint32_t intervals[] = {1, 2, 3, 4096, 65536};
    for (size_t i = 0; i < sizeof(intervals) / sizeof(intervals[0]); ++i)
    {
        LockFreeQueue<int> queue(false);
        LFQ_NEW_API_CHECK(queue.configureConsumerYieldInterval(intervals[i]) == LockFreeQueueSuccess);
        LFQ_NEW_API_CHECK(queue.configureConsumerYieldInterval(0) == LockFreeQueueInvalidConfiguration);
        LFQ_NEW_API_CHECK(queue.configureConsumerYieldInterval(65537) == LockFreeQueueInvalidConfiguration);
        LFQ_NEW_API_CHECK(queue.configureConsumerYieldInterval(UINT32_MAX) == LockFreeQueueInvalidConfiguration);
        for (size_t lifecycle = 0; lifecycle < 2; ++lifecycle)
        {
            LFQ_NEW_API_CHECK(queue.tryInit() == LockFreeQueueSuccess);
            LFQ_NEW_API_CHECK(queue.configureConsumerYieldInterval(1) == LockFreeQueueInvalidState);
            LockFreeQueueTestControl::m_scheduleYields = 0;
            for (size_t n = 0; n < 2 * size_t(intervals[i]) + 1; ++n)
            {
                LFQ_NEW_API_CHECK(!queue.pop(nullptr));
            }
            LFQ_NEW_API_CHECK(LockFreeQueueTestControl::m_scheduleYields == (2 * size_t(intervals[i]) + 1) / intervals[i]);
            queue.destroy();
        }
    }
    return consumerBatchBudget();
}

bool LockFreeQueueRetryContract::entrySpan()
{
    typedef LockFreeQueueEntry<uint64_t> Entry;
    const size_t budgets[] = {0, 1, 2, 3, 4, 5, 15, 16, 63, 64, 65, 127, 128, 255, 256, 257, SIZE_MAX};
    size_t scenarios = 0;
    for (size_t mode = 0; mode < 4; ++mode)
    {
        for (size_t offset = 0; offset < 256; ++offset)
        {
            for (size_t b = 0; b < sizeof(budgets) / sizeof(budgets[0]); ++b)
            {
                std::unique_ptr<Entry[]> entries(new Entry[513]);
                LockFreeQueue<uint64_t> queue(false);
                LockFreeQueuePoolConfig config;
                config.initialCapacity = (mode & 1) ? 65 : 1025;
                config.maximumCapacity = (mode & 1) ? -1 : 1025;
                config.maximumProducers = 2;
                LFQ_NEW_API_CHECK(queue.configurePool(config) == LockFreeQueueSuccess);
                LFQ_NEW_API_CHECK(queue.configureCancelableReservations((mode & 2) != 0) == LockFreeQueueSuccess);
                LFQ_NEW_API_CHECK(queue.configureEntryOnly(true) == LockFreeQueueSuccess);
                LFQ_NEW_API_CHECK(queue.tryInit() == LockFreeQueueSuccess);
                // Two leases also exercise the reservation implementation rather than its sole-publisher shortcut.
                LockFreeQueueProducer<uint64_t> first, second;
                LFQ_NEW_API_CHECK(queue.tryAcquireProducer(first) == LockFreeQueueSuccess);
                LFQ_NEW_API_CHECK(queue.tryAcquireProducer(second) == LockFreeQueueSuccess);
                for (size_t n = 0; n < 512; ++n)
                {
                    entries[n].m_value = n;
                    LFQ_NEW_API_CHECK((n & 1 ? first : second).tryPushEntry(entries[n], true, false) == LockFreeQueueSuccess);
                }
                for (size_t n = 0; n < offset; ++n)
                {
                    uint64_t value = UINT64_MAX;
                    LFQ_NEW_API_CHECK(queue.pop(&value) && value == n);
                }
                Entry* output[258];
                std::fill(output, output + 258, &entries[512]);
                size_t consumed = SIZE_MAX;
                const bool discard = ((offset + b) & 1) != 0;
                const size_t expected = (std::min)(budgets[b], (std::min)(size_t(256) - offset, size_t((mode & 1) ? 64 : 256)));
                const bool result = queue.probeReadyEntrySpan(discard ? nullptr : output + 1, budgets[b], consumed);
                LFQ_NEW_API_CHECK(result == (expected != 0) && consumed == expected);
                LFQ_NEW_API_CHECK(output[0] == &entries[512] && output[257] == &entries[512]);
                for (size_t n = 0; n < 256; ++n)
                {
                    LFQ_NEW_API_CHECK(output[n + 1] == ((!discard && n < consumed) ? &entries[offset + n] : &entries[512]));
                }
                for (size_t n = offset + consumed; n < 512; ++n)
                {
                    uint64_t value = UINT64_MAX;
                    LFQ_NEW_API_CHECK(queue.pop(&value) && value == n);
                }
                LFQ_NEW_API_CHECK(queue.empty());
                LFQ_NEW_API_CHECK(!queue.probeReadyEntrySpan(nullptr, SIZE_MAX, consumed) && consumed == 0);
                LFQ_NEW_API_CHECK(first.tryPushEntry(entries[0], true, false) == LockFreeQueueSuccess);
                LFQ_NEW_API_CHECK(queue.probeReadyEntrySpan(output, SIZE_MAX, consumed) && consumed == 1 && output[0] == &entries[0]);
                ++scenarios;
            }
        }
    }
    LockFreeQueue<uint64_t> ordinary;
    LFQ_NEW_API_CHECK(ordinary.push(7));
    size_t consumed = SIZE_MAX;
    LFQ_NEW_API_CHECK(!ordinary.probeReadyEntrySpan(nullptr, 256, consumed) && consumed == 0);
    uint64_t value = 0;
    LFQ_NEW_API_CHECK(ordinary.pop(&value) && value == 7);
    std::cout << "SPAN_SCENARIOS=" << scenarios << std::endl;
    return true;
}

bool LockFreeQueueRetryContract::consumerBatchBudget()
{
    LockFreeQueueEntry<int> entries[256];
    LockFreeQueue<int> queue(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 513;
    config.maximumCapacity = 513;
    config.maximumProducers = 1;
    LFQ_NEW_API_CHECK(queue.configurePool(config) == LockFreeQueueSuccess);
    LFQ_NEW_API_CHECK(queue.configureEntryOnly(true) == LockFreeQueueSuccess);
    LFQ_NEW_API_CHECK(queue.configureConsumerYieldInterval(1) == LockFreeQueueSuccess);
    LFQ_NEW_API_CHECK(queue.tryInit() == LockFreeQueueSuccess);
    for (size_t n = 0; n < 256; ++n)
    {
        LFQ_NEW_API_CHECK(queue.tryPushEntry(entries[n], true, false) == LockFreeQueueSuccess);
    }
    LockFreeQueueTestControl::m_scheduleYields = 0;
    size_t consumed = 0;
    LFQ_NEW_API_CHECK(queue.probeReadyEntrySpan(nullptr, 256, consumed) && consumed == 256);
    LFQ_NEW_API_CHECK(LockFreeQueueTestControl::m_scheduleYields.load() == 2);
    std::cout << "BATCH_YIELD_OBSERVATION interval=1 consumed=" << consumed << " yields=" << LockFreeQueueTestControl::m_scheduleYields << std::endl;
    return true;
}

#undef LFQ_NEW_API_CHECK
#endif

#ifdef LOCK_FREE_QUEUE_RETRY_MAIN
/** Run the independently selectable retry-contract checks
@return Zero if all selected contract checks pass; otherwise one
*/
int main()
{
    try
    {
        LockFreeQueueRetryContract test;
        const bool result = test.run();
        std::cout << "RETRY_CONTRACT result=" << result << std::endl;
        return result ? 0 : 1;
    }
    catch (...)
    {
        std::cout << "FAIL unexpected exception" << std::endl;
        return 2;
    }
}
#endif