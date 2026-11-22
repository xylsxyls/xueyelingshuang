#include "CTaskStorageTests.h"
#include "CTaskRedoTests.h"
#ifndef CTTM_TEST_DLL
#include <memory>
#include "CTaskFaultAllocation.h"
#include "CTaskPreparedBenchmark.h"
#include "CTaskThreadManager/CTaskReadDomain.h"
#include "CTaskThreadManager/CTaskStorage.h"
#include "CTaskThreadManager/CTaskReadyIndex.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

/** 检查独立预期；失败抛到测试入口，不使用运行时 assert 或 abort
@param [in] condition 实际条件
@param [in] message 失败诊断
*/
static void StorageCheck(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

CTaskStorageTestLifetimeTask::CTaskStorageTestLifetimeTask(std::atomic<size_t>& live) :
m_live(&live)
{
    ++*m_live;
}

CTaskStorageTestLifetimeTask::~CTaskStorageTestLifetimeTask()
{
    --*m_live;
}

void CTaskStorageTestLifetimeTask::doTask()
{

}

void taskStorageTest()
{
    taskReclaimRoutingTest();
    // 只检查可表示的边界，不分配巨型数组。所有者编号还须为消费者保留一个值。
    const size_t cacheSlots = ((std::numeric_limits<size_t>::max)() - 63) / sizeof(CTaskStorageCache);
    const size_t firstInvalid = (std::min)(cacheSlots, static_cast<size_t>(UINT32_MAX));
    StorageCheck(!CTaskStorage::validProtectionCount(0) && CTaskStorage::validProtectionCount(1) &&
        CTaskStorage::validProtectionCount(firstInvalid - 1) && !CTaskStorage::validProtectionCount(firstInvalid),
        "protection budget lost the last addressable cache or admitted one too many");
    std::atomic<size_t> live(0);
    CTaskReadDomain domain;
    StorageCheck(domain.initialize(8), "storage domain initialization");
    std::unique_ptr<CTaskStorage> storageOwner(new CTaskStorage(domain));
    CTaskStorage& storage = *storageOwner;
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 4;
    config.maximumCapacity = 4;
    StorageCheck(storage.initialize(config).m_committed, "storage initialization");
    // 验证实际分配地址，不只检查类型布局；旧x86数组cookie曾使整个缓存偏移4字节。
    for (size_t i = 0; i <= domain.capacity(); ++i)
    {
        CTaskStorageCache& current = storage.cache(i);
        StorageCheck(reinterpret_cast<uintptr_t>(&current) % std::alignment_of<CTaskStorageCache>::value == 0 &&
                reinterpret_cast<uintptr_t>(&current.m_reserved) % sizeof(uint64_t) == 0 &&
                reinterpret_cast<uintptr_t>(&current.m_published) % sizeof(uint64_t) == 0,
            "storage cache allocation misaligned an object or 64-bit counter");
        for (size_t level = 0; level < 16; ++level)
        {
            StorageCheck(reinterpret_cast<uintptr_t>(&current.m_levels[level]) % sizeof(uint64_t) == 0,
                "storage cache allocation misaligned a priority counter");
        }
    }
    CTaskReadPosition* held = domain.enter();
    StorageCheck(held != nullptr, "storage reader allocation");
    CTaskStorageCache& cache = storage.cache(domain.indexOf(*held));
    CTaskThreadResult result;
    CTaskStorageSlot* slots[4] = {nullptr, nullptr, nullptr, nullptr};
    for (size_t i = 0; i < 4; ++i)
    {
        result = CTaskThreadResult(CTaskThreadSystemFailure, true, 123);
        slots[i] = storage.acquire(cache, result);
        StorageCheck(slots[i] != nullptr && result.m_status == CTaskThreadOk && !result.m_committed &&
            result.m_nativeError == 0, "bounded storage allocation retained a previous result");
        slots[i]->m_task = std::make_shared<CTaskStorageTestLifetimeTask>(live);
        slots[i]->m_state->m_bits.store(0, std::memory_order_release);
        size_t beforePublication = 0;
        storage.forEach([&beforePublication](CTaskStorageSlot&)
        {
            ++beforePublication;
        });
        StorageCheck(beforePublication == i, "uncommitted prepared slot became observable");
        slots[i]->m_page->m_publications[slots[i]->index()].store(true, std::memory_order_release);
    }
    StorageCheck(storage.acquire(cache, result) == nullptr && result.m_status == CTaskThreadCapacity, "storage exceeded physical maximum");
    size_t visited = 0;
    // 只在保护范围内借用任务；返回累计访问数量。
    storage.forEach([&visited](CTaskStorageSlot& slot)
    {
        StorageCheck(slot.m_task.get() != nullptr, "published slot has no task");
        ++visited;
    });
    StorageCheck(visited == 4, "published storage enumeration");
    for (size_t i = 0; i < 4; ++i)
    {
        slots[i]->m_page->m_publications[slots[i]->index()].store(false, std::memory_order_relaxed);
        slots[i]->m_state->m_bits.store(3, std::memory_order_release);
        storage.retire(*slots[i]);
        // 队列可在投递者返回前执行完；迟到的发布位不能使完成记录重新出现在遍历中。
        slots[i]->m_page->m_publications[slots[i]->index()].store(true, std::memory_order_release);
    }
    visited = 0;
    storage.forEach([&visited](CTaskStorageSlot&)
    {
        ++visited;
    });
    StorageCheck(visited == 0, "late publication resurrected completed records");
    StorageCheck(storage.collect(100) == 0 && live.load() == 4, "old reader did not preserve retired tasks");
    domain.leave(*held);
    StorageCheck(storage.collect(2) == 2 && live.load() == 2 && storage.collect(2) == 2 && live.load() == 0,
        "bounded collection or task lifetime");
    held = domain.enter();
    CTaskStorageSlot* reused = storage.acquire(storage.cache(domain.indexOf(*held)), result);
    StorageCheck(reused != nullptr && !reused->m_task, "retired slot not reusable");
    storage.abandon(*reused);
    domain.leave(*held);

    CTaskReadDomain growingDomain;
    StorageCheck(growingDomain.initialize(8), "growing domain initialization");
    std::unique_ptr<CTaskStorage> growingOwner(new CTaskStorage(growingDomain));
    CTaskStorage& growing = *growingOwner;
    config.initialCapacity = 3;
    config.maximumCapacity = 13;
    std::atomic<size_t> grows(0);
    std::atomic<size_t> growthEvents(0), growthSlots(0);
    // 观察实际任务记录池的增长；观察异常不能撤销已发布的记录块。
    config.m_onGrowth = [&growthEvents, &growthSlots](size_t count, uint64_t sequence)
    {
        if (sequence != 0)
        {
            growthEvents.fetch_add(1);
            growthSlots.fetch_add(count);
        }
        throw std::runtime_error("storage growth observer");
    };
    // 验证回调只负责算法；1000 的请求由池截断为剩余预算。
    config.growPolicy = [&grows](const LockFreeHashMapGrowContext& context) -> size_t
    {
        ++grows;
        StorageCheck(context.initialCapacity == 3 && context.currentCapacity == 3 && context.successfulGrowCount == 0,
            "growth context changed");
        return 1000;
    };
    // 返回期望释放量和连续空闲毫秒，由池保证容量下限。
    config.shrinkPolicy = [](const LockFreeHashMapShrinkContext& context) -> LockFreeHashMapShrinkDecision
    {
        return LockFreeHashMapShrinkDecision(context.currentCapacity - context.initialCapacity, 1);
    };
    StorageCheck(growing.initialize(config).m_committed, "growing pool initialization");
    held = growingDomain.enter();
    std::vector<CTaskStorageSlot*> allocated;
    for (size_t i = 0; i < 13; ++i)
    {
        CTaskStorageSlot* slot = growing.acquire(growing.cache(growingDomain.indexOf(*held)), result);
        StorageCheck(slot != nullptr && grows == (i < 3 ? 0u : 1u),
            "zero threshold did not wait for the first request beyond initial capacity");
        allocated.push_back(slot);
    }
    StorageCheck(growing.capacity() == 13 && grows.load() == 1, "growth exceeded maximum or wrong count");
    StorageCheck(growthEvents == 1 && growthSlots == 10, "storage notification missing, speculative or included initial reserve");
    for (size_t i = 0; i < allocated.size(); ++i)
    {
        growing.abandon(*allocated[i]);
    }
    growingDomain.leave(*held);
    growing.maintain();
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    growing.maintain();
    growing.maintain();
    StorageCheck(growing.capacity() == 3, "shrink did not preserve initial capacity");

    // 三个初始槽，剩一个时回调。发布到硬上限后继续领槽，策略及通知次数必须保持不变。
    CTaskReadDomain thresholdDomain;
    StorageCheck(thresholdDomain.initialize(4), "threshold domain initialization");
    std::unique_ptr<CTaskStorage> thresholdPool(new CTaskStorage(thresholdDomain));
    config.growFreeThreshold = 1;
    grows = 0;
    growthEvents = 0;
    growthSlots = 0;
    StorageCheck(thresholdPool->initialize(config).m_committed, "threshold pool initialization");
    CTaskReadPosition* thresholdReader = thresholdDomain.enter();
    StorageCheck(thresholdReader != nullptr, "threshold reader registration");
    CTaskStorageCache& thresholdCache = thresholdPool->cache(thresholdDomain.indexOf(*thresholdReader));
    CTaskStorageSlot* thresholdSlots[13];
    for (size_t i = 0; i < 13; ++i)
    {
        thresholdSlots[i] = thresholdPool->acquire(thresholdCache, result);
        const size_t expectedGrows = i == 0 ? 0 : 1;
        StorageCheck(thresholdSlots[i] != nullptr && result.m_status == CTaskThreadOk &&
            grows == expectedGrows && growthEvents == expectedGrows && growthSlots == expectedGrows * 10,
            "threshold growth fired early, fired late or repeated at the hard limit");
    }
    StorageCheck(thresholdPool->acquire(thresholdCache, result) == nullptr && result.m_status == CTaskThreadCapacity &&
        grows == 1 && growthEvents == 1, "exhausted threshold pool invoked a policy at its hard limit");
    for (size_t i = 0; i < 13; ++i)
    {
        thresholdPool->abandon(*thresholdSlots[i]);
    }
    thresholdDomain.leave(*thresholdReader);

    CTaskReadDomain parallelDomain;
    StorageCheck(parallelDomain.initialize(16), "parallel storage domain");
    std::unique_ptr<CTaskStorage> parallelOwner(new CTaskStorage(parallelDomain));
    CTaskStorage& parallel = *parallelOwner;
    config = LockFreeHashMapPoolConfig();
    config.initialCapacity = 1024;
    config.maximumCapacity = 1024;
    StorageCheck(parallel.initialize(config).m_committed, "parallel storage initialization");
    std::vector<CTaskStorageSlot*> simultaneous(256, nullptr);
    std::vector<std::thread> producers;
    std::atomic<size_t> errors(0);
    producers.reserve(8);
    try
    {
        for (size_t i = 0; i < 8; ++i)
        {
            // 每个线程保留 32 个槽，不归还，以便事后检查是否重复分配。
            producers.push_back(std::thread([&parallel, &parallelDomain, &simultaneous, &errors, i]()
            {
                CTaskReadPosition* reader = nullptr;
                try
                {
                    reader = parallelDomain.enter();
                    StorageCheck(reader != nullptr, "parallel reader exhaustion");
                    CTaskThreadResult localResult;
                    for (size_t j = 0; j < 32; ++j)
                    {
                        simultaneous[i * 32 + j] = parallel.acquire(parallel.cache(parallelDomain.indexOf(*reader)), localResult);
                        StorageCheck(simultaneous[i * 32 + j] != nullptr, "parallel allocation failed");
                    }
                }
                catch (...)
                {
                    ++errors;
                }
                if (reader != nullptr)
                {
                    parallelDomain.leave(*reader);
                }
            }));
        }
    }
    catch (...)
    {
        for (size_t i = 0; i < producers.size(); ++i)
        {
            producers[i].join();
        }
        throw;
    }
    for (size_t i = 0; i < producers.size(); ++i)
    {
        producers[i].join();
    }
    std::sort(simultaneous.begin(), simultaneous.end(), std::less<CTaskStorageSlot*>());
    StorageCheck(errors.load() == 0 && simultaneous[0] != nullptr &&
            std::adjacent_find(simultaneous.begin(), simultaneous.end()) == simultaneous.end(),
        "parallel pool issued duplicate slots");
    for (size_t i = 0; i < simultaneous.size(); ++i)
    {
        parallel.abandon(*simultaneous[i]);
    }

    // 同一读侧位置跨级别分配到真实容量上限，空闲分级缓存必须可归还再利用。
    held = parallelDomain.enter();
    StorageCheck(held != nullptr, "priority allocation reader");
    allocated.clear();
    for (size_t i = 0; i < 1024; ++i)
    {
        CTaskStorageSlot* slot = parallel.acquire(parallel.cache(parallelDomain.indexOf(*held)), result,
            static_cast<int32_t>(i % 16) + 1);
        StorageCheck(slot != nullptr, "priority caches hid free capacity");
        allocated.push_back(slot);
    }
    std::sort(allocated.begin(), allocated.end(), std::less<CTaskStorageSlot*>());
    StorageCheck(std::adjacent_find(allocated.begin(), allocated.end()) == allocated.end() &&
            parallel.acquire(parallel.cache(parallelDomain.indexOf(*held)), result, 16) == nullptr &&
            result.m_status == CTaskThreadCapacity,
        "priority cache duplicated slots or exceeded capacity");
    for (size_t i = 0; i < allocated.size(); ++i)
    {
        parallel.abandon(*allocated[i]);
    }
    parallel.flush(parallel.cache(parallelDomain.indexOf(*held)));
    parallelDomain.leave(*held);

    CTaskReadDomain delayedDomain;
    StorageCheck(delayedDomain.initialize(4), "retirement window domain");
    std::unique_ptr<CTaskStorage> delayedOwner(new CTaskStorage(delayedDomain));
    CTaskStorage& delayed = *delayedOwner;
    config = LockFreeHashMapPoolConfig();
    config.initialCapacity = 2048;
    config.maximumCapacity = 2048;
    StorageCheck(delayed.initialize(config).m_committed, "retirement window storage");
    for (size_t round = 0; round < 3; ++round)
    {
        held = delayedDomain.enter();
        StorageCheck(held != nullptr, "retirement window reader");
        for (size_t i = 0; i < 1500; ++i)
        {
            CTaskStorageSlot* slot = delayed.acquire(delayed.cache(delayedDomain.indexOf(*held)), result,
                static_cast<int32_t>(i % 16) + 1);
            StorageCheck(slot != nullptr, "retirement window allocation");
            slot->m_state->m_bits.store(3, std::memory_order_release);
            delayed.retire(*slot);
        }
        StorageCheck(delayed.collect(2048) == 0, "window released records protected by paused reader");
        delayedDomain.leave(*held);
        StorageCheck(delayed.collect(0) == 0 && delayed.collect(1) == 1 && delayed.collect(17) == 17,
            "retirement window partial budget changed");
        held = delayedDomain.enter();
        StorageCheck(held != nullptr, "retirement overflow append reader");
        CTaskStorageSlot* appended = delayed.acquire(delayed.cache(delayedDomain.indexOf(*held)), result);
        StorageCheck(appended != nullptr, "retirement overflow append allocation");
        appended->m_state->m_bits.store(3, std::memory_order_release);
        delayed.retire(*appended);
        delayedDomain.leave(*held);
        StorageCheck(delayed.collect(1007) == 1007 && delayed.collect(2048) == 476 && delayed.collect(1) == 0,
            "retirement window wrap/overflow/budget lost or duplicated records");
    }
    for (size_t round = 0; round < 40; ++round)
    {
        held = delayedDomain.enter();
        StorageCheck(held != nullptr, "rolling retirement reader");
        for (size_t i = 0; i < 63; ++i)
        {
            CTaskStorageSlot* slot = delayed.acquire(delayed.cache(delayedDomain.indexOf(*held)), result);
            StorageCheck(slot != nullptr, "rolling retirement allocation");
            slot->m_state->m_bits.store(3, std::memory_order_release);
            delayed.retire(*slot);
        }
        delayedDomain.leave(*held);
        StorageCheck(delayed.collect(64) == 63 && delayed.collect(1) == 0, "rolling retirement count changed");
    }
    // 超过边界表容量的不同 epoch 必须继续使用精确的溢出链，不覆盖仍受保护的记录。
    for (size_t round = 0; round < 3; ++round)
    {
        held = delayedDomain.enter();
        StorageCheck(held != nullptr, "epoch boundary overflow reader");
        for (size_t i = 0; i < 130; ++i)
        {
            CTaskStorageSlot* slot = delayed.acquire(delayed.cache(0), result);
            StorageCheck(slot != nullptr, "epoch boundary overflow allocation");
            slot->m_task = std::make_shared<CTaskStorageTestLifetimeTask>(live);
            slot->m_state->m_bits.store(3, std::memory_order_release);
            delayed.retire(*slot);
            delayedDomain.advance();
        }
        StorageCheck(delayed.collect(512) == 0 && live.load() == 130,
            "epoch boundary overflow released a protected lifetime");
        delayedDomain.leave(*held);
        size_t remaining = 130;
        while (remaining != 0)
        {
            const size_t amount = (std::min)(remaining, size_t(7));
            StorageCheck(delayed.collect(amount) == amount,
                "epoch boundary overflow changed the partial collection budget");
            remaining -= amount;
            StorageCheck(live.load() == remaining, "epoch boundary overflow lost a task lifetime");
        }
        StorageCheck(delayed.collect(1) == 0, "epoch boundary overflow repeated a release");
    }
    // 后进入的读者只保护新 epoch；合并记账不能把旧记录也滞留到这个读者退出。
    for (size_t round = 0; round < 80; ++round)
    {
        CTaskStorageSlot* old = delayed.acquire(delayed.cache(0), result);
        StorageCheck(old != nullptr, "epoch boundary old allocation");
        old->m_task = std::make_shared<CTaskStorageTestLifetimeTask>(live);
        old->m_state->m_bits.store(3, std::memory_order_release);
        delayed.retire(*old);
        delayedDomain.advance();
        held = delayedDomain.enter();
        StorageCheck(held != nullptr, "epoch boundary new reader");
        CTaskStorageSlot* recent = delayed.acquire(delayed.cache(0), result);
        StorageCheck(recent != nullptr, "epoch boundary recent allocation");
        recent->m_task = std::make_shared<CTaskStorageTestLifetimeTask>(live);
        recent->m_state->m_bits.store(3, std::memory_order_release);
        delayed.retire(*recent);
        StorageCheck(delayed.collect(2) == 1 && live.load() == 1 && delayed.collect(1) == 0,
            "epoch boundary delayed old work or released new protected work");
        delayedDomain.leave(*held);
        StorageCheck(delayed.collect(1) == 1 && live.load() == 0,
            "epoch boundary did not release the remaining lifetime");
    }
}

void taskStorageFailureTest()
{
    for (uint64_t failure = 1; failure <= 2; ++failure)
    {
        CTaskReadDomain domain;
        StorageCheck(domain.initialize(4), "failure domain");
        std::unique_ptr<CTaskStorage> storageOwner(new CTaskStorage(domain));
        CTaskStorage& storage = *storageOwner;
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 0;
        config.maximumCapacity = 3;
        StorageCheck(storage.initialize(config).m_committed, "zero initial pool");
        CTaskReadPosition* reader = domain.enter();
        CTaskThreadResult result;
        CTaskFaultAllocation::begin(failure);
        CTaskStorageSlot* failed = storage.acquire(storage.cache(domain.indexOf(*reader)), result);
        CTaskFaultAllocation::end();
        StorageCheck(failed == nullptr && result.m_status == CTaskThreadNoMemory &&
                CTaskFaultAllocation::triggered() && storage.capacity() == 0,
            "failed growth leaked reserved budget");
        CTaskStorageSlot* recovered = storage.acquire(storage.cache(domain.indexOf(*reader)), result);
        StorageCheck(recovered != nullptr && storage.capacity() == 3, "growth did not recover after allocation failure");
        storage.abandon(*recovered);
        domain.leave(*reader);
    }
    for (size_t throwing = 0; throwing < 2; ++throwing)
    {
        CTaskReadDomain domain;
        StorageCheck(domain.initialize(4), "policy domain");
        std::unique_ptr<CTaskStorage> storageOwner(new CTaskStorage(domain));
        CTaskStorage& storage = *storageOwner;
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 0;
        // 只有算法拒绝和算法异常两种输入，不依赖库内部实现分支。
        config.growPolicy = [throwing](const LockFreeHashMapGrowContext&) -> size_t
        {
            if (throwing != 0)
            {
                throw std::runtime_error("intentional growth failure");
            }
            return 0;
        };
        StorageCheck(storage.initialize(config).m_committed, "policy pool");
        CTaskReadPosition* reader = domain.enter();
        CTaskThreadResult result;
        StorageCheck(storage.acquire(storage.cache(domain.indexOf(*reader)), result) == nullptr &&
                result.m_status == (throwing != 0 ? CTaskThreadPolicyFailure : CTaskThreadCapacity) && storage.capacity() == 0,
            "policy failure committed resources");
        domain.leave(*reader);
    }
    CTaskReadDomain domain;
    StorageCheck(domain.initialize(4), "shrink floor domain");
    std::unique_ptr<CTaskStorage> storageOwner(new CTaskStorage(domain));
    CTaskStorage& storage = *storageOwner;
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 12;
    config.growPolicy = [](const LockFreeHashMapGrowContext&) -> size_t
    {
        return 4;
    };
    bool failShrink = false;
    config.shrinkPolicy = [&failShrink](const LockFreeHashMapShrinkContext& context) -> LockFreeHashMapShrinkDecision
    {
        if (failShrink)
        {
            throw std::runtime_error("intentional shrink failure");
        }
        return LockFreeHashMapShrinkDecision(context.currentCapacity, 1);
    };
    StorageCheck(storage.initialize(config).m_committed, "shrink floor pool");
    CTaskReadPosition* held = domain.enter();
    CTaskThreadResult result;
    std::vector<CTaskStorageSlot*> slots;
    for (size_t i = 0; i < 12; ++i)
    {
        CTaskStorageSlot* slot = storage.acquire(storage.cache(domain.indexOf(*held)), result);
        StorageCheck(slot != nullptr, "multiple growth allocation");
        slots.push_back(slot);
    }
    for (size_t i = 0; i < slots.size(); ++i)
    {
        storage.abandon(*slots[i]);
    }
    storage.maintain();
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    for (size_t cycle = 0; cycle < 4; ++cycle)
    {
        storage.maintain();
        StorageCheck(storage.capacity() == 12, "closed block reclaimed while old reader held");
        // 新读者允许看到关闭块的容量信息，不应解引用其退休载荷。
        CTaskReadPosition* recent = domain.enter();
        StorageCheck(recent != nullptr, "recent reader failed");
        CTaskStorageSlot* available = storage.acquire(storage.cache(domain.indexOf(*recent)), result);
        StorageCheck(available != nullptr, "shrink closed all initial capacity");
        storage.abandon(*available);
        domain.leave(*recent);
    }
    // 已退役的块仍受旧读者保护。后续策略持续失败不能阻断这些块的最终回收。
    failShrink = true;
    StorageCheck(storage.maintain().m_status == CTaskThreadPolicyFailure && storage.capacity() == 12,
        "throwing shrink policy reclaimed a block still protected by a reader");
    domain.leave(*held);
    StorageCheck(storage.maintain().m_status == CTaskThreadPolicyFailure && storage.capacity() == 2,
        "throwing shrink policy stranded retired blocks or violated the shrink floor");
    failShrink = false;
    StorageCheck(storage.maintain().m_status == CTaskThreadOk && storage.capacity() == 2,
        "retired blocks were counted twice against shrink floor");
}

/** 对两种发布顺序验证跨段、空观察和外部上下文借用期
@param [in] reservations 是否启用可取消预约模式
*/
static void RunQueueReadyTest(bool reservations)
{
    LockFreeQueue<int32_t> queue(false);
    StorageCheck(queue.configureConsumerYieldInterval(0) == LockFreeQueueInvalidConfiguration &&
        queue.configureConsumerYieldInterval(65537) == LockFreeQueueInvalidConfiguration,
        "invalid consumer cooperation interval accepted");
    StorageCheck(queue.configureConsumerYieldInterval(reservations ? 16384 : 1) == LockFreeQueueSuccess,
        "consumer cooperation interval configuration");
    StorageCheck(queue.configureCancelableReservations(reservations) == LockFreeQueueSuccess, "queue reservation configuration");
    StorageCheck(!queue.possiblyReady(), "uninitialized readiness probe was not empty");
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 4096;
    config.maximumCapacity = 4096;
    StorageCheck(queue.configureProducerCounters(true) == LockFreeQueueSuccess, "distributed producer counters configuration");
    StorageCheck(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess,
        "ready queue initialization");
    StorageCheck(queue.configureCancelableReservations(!reservations) == LockFreeQueueInvalidState,
        "live publication order was changed");
    StorageCheck(queue.configureConsumerYieldInterval(65536) == LockFreeQueueInvalidState,
        "live consumer cooperation interval changed");
    int32_t output = -1;
    StorageCheck(!queue.possiblyReady(), "fresh readiness probe was not empty");
    StorageCheck(queue.configureProducerCounters(false) == LockFreeQueueInvalidState, "live counter mode changed");
    StorageCheck(!queue.popReady(&output) && output == -1, "empty ready pop changed output");
    for (int32_t round = 0; round < 20; ++round)
    {
        for (int32_t i = 0; i < 1024; ++i)
        {
            const LockFreeQueueResult posted = i % 2 == 0 ? queue.tryPushAndFence(round * 1024 + i) : queue.tryPush(round * 1024 + i);
            StorageCheck(posted == LockFreeQueueSuccess, "ready queue push failed");
        }
        for (int32_t i = 0; i < 1024; ++i)
        {
            const uint64_t before = queue.size();
            StorageCheck(queue.possiblyReady() && queue.possiblyReady() && queue.size() == before,
                "readiness probe missed a published successor or consumed an item");
            StorageCheck(queue.popReady(&output) && output == round * 1024 + i, "ready pop changed FIFO across segments");
        }
        StorageCheck(!queue.popReady(&output) && queue.size() == 0, "ready pop lost empty boundary");
        queue.maintain(64);
    }
    StorageCheck(queue.tryPush(1) == LockFreeQueueSuccess && queue.popReady(nullptr) && !queue.pop(&output),
        "ready discard or normal pop integration");
    // possiblyReady允许将待处理段边界报告为true，不能用提示代替真实出队判空。
    // 两次探测不得消费或制造条目；随后发布的真实数据仍须可见且只交付一次。
    queue.possiblyReady();
    queue.possiblyReady();
    StorageCheck(!queue.popReady(&output) && queue.size() == 0,
        "readiness hint manufactured an item at an empty boundary");
    StorageCheck(queue.tryPush(2) == LockFreeQueueSuccess && queue.possiblyReady() &&
        queue.popReady(&output) && output == 2 && !queue.popReady(&output),
        "empty-boundary hint lost or duplicated later publication");
    queue.destroy();
    StorageCheck(!queue.possiblyReady(), "destroyed readiness probe retained a segment");
    StorageCheck(queue.configureConsumerYieldInterval(65536) == LockFreeQueueSuccess &&
        queue.tryInit() == LockFreeQueueSuccess && queue.tryPush(7) == LockFreeQueueSuccess &&
        queue.popReady(&output) && output == 7 && !queue.popReady(&output),
        "consumer cooperation configuration broke reinitialization");
    queue.destroy();

    // 暂停在途投递时维护不得释放其上下文；归还读侧位置后可由其他调用帮助清理。
    LockFreeQueue<CTaskStorageSlot*> retained(false);
    StorageCheck(retained.configureCancelableReservations(reservations) == LockFreeQueueSuccess, "queue reservation configuration");
    StorageCheck(retained.configurePool(config) == LockFreeQueueSuccess && retained.tryInit() == LockFreeQueueSuccess,
        "retained queue initialization");
    CTaskReadDomain domain;
    StorageCheck(domain.initialize(4), "retained producer domain initialization");
    std::unique_ptr<CTaskStorage> storageOwner(new CTaskStorage(domain));
    CTaskStorage& storage = *storageOwner;
    LockFreeHashMapPoolConfig storageConfig;
    storageConfig.initialCapacity = 4;
    storageConfig.maximumCapacity = 4;
    StorageCheck(storage.initialize(storageConfig).m_status == CTaskThreadOk, "retained producer storage initialization");
    CTaskReadPosition* position = domain.tryEnter(0);
    StorageCheck(position != nullptr, "retained producer position");
    CTaskStorageCache& cache = storage.cache(0);
    StorageCheck(retained.tryAcquireProducer(cache.m_producers[0]) == LockFreeQueueSuccess, "retained producer acquisition");
    cache.m_producerCount.store(1);
    StorageCheck(storage.releaseIdleProducers() == 0 && cache.m_producers[0].attached(), "active producer was released");
    domain.leave(*position);
    StorageCheck(storage.releaseIdleProducers() == 1 && !cache.m_producers[0].attached(), "idle producer not released");
    StorageCheck(retained.tryAcquireProducer(cache.m_producers[0]) == LockFreeQueueSuccess, "producer slot not reusable");
    cache.m_producerCount.store(1);
    StorageCheck(cache.m_producers[0].tryPush(nullptr) == LockFreeQueueSuccess && retained.popReady(nullptr),
        "retained producer FIFO integration");
    storage.clear();
    StorageCheck(!cache.m_producers[0].attached() && cache.m_producerCount.load() == 0,
        "storage clear retained a pointer to its queue");
}

void taskQueueReadyTest()
{
    taskChannelPublicationTest();
    RunQueueReadyTest(false);
    RunQueueReadyTest(true);
}

bool CTaskMoveAssignmentValue::m_failCopy = false;

CTaskMoveAssignmentValue::CTaskMoveAssignmentValue(int value) :
m_value(value)
{

}

CTaskMoveAssignmentValue::CTaskMoveAssignmentValue(const CTaskMoveAssignmentValue& source) :
m_value(source.m_value)
{
    if (m_failCopy)
    {
        throw std::runtime_error("copy failure");
    }
}

CTaskMoveAssignmentValue& CTaskMoveAssignmentValue::operator=(CTaskMoveAssignmentValue&& source) throw()
{
    m_value = source.m_value;
    source.m_value = -1;
    return *this;
}

CTaskStorageTestAssignmentValue::CTaskStorageTestAssignmentValue(int value, bool fail) :
m_value(value),
m_fail(fail)
{

}

CTaskStorageTestAssignmentValue::CTaskStorageTestAssignmentValue(const CTaskStorageTestAssignmentValue& source) :
m_value(source.m_value),
m_fail(source.m_fail)
{

}

CTaskStorageTestAssignmentValue& CTaskStorageTestAssignmentValue::operator=(const CTaskStorageTestAssignmentValue& source)
{
    if (source.m_fail)
    {
        throw std::runtime_error("planned batch assignment failure");
    }
    m_value = source.m_value;
    m_fail = source.m_fail;
    return *this;
}

/** 对两种发布顺序验证输出异常时的精确提交前缀
@param [in] reservations 是否启用可取消预约模式
*/
static void RunQueueBatchTest(bool reservations)
{
    LockFreeQueue<CTaskStorageTestAssignmentValue> queue(false);
    StorageCheck(queue.configureCancelableReservations(reservations) == LockFreeQueueSuccess &&
        queue.tryInit() == LockFreeQueueSuccess, "batch reservation configuration");
    StorageCheck(queue.tryPush(CTaskStorageTestAssignmentValue(1)) == LockFreeQueueSuccess &&
            queue.tryPush(CTaskStorageTestAssignmentValue(2, true)) == LockFreeQueueSuccess &&
            queue.tryPush(CTaskStorageTestAssignmentValue(3)) == LockFreeQueueSuccess,
        "batch assignment fixture setup");
    CTaskStorageTestAssignmentValue output[16];
    size_t consumed = 100;
    StorageCheck(!queue.popReadyBatch(output, 0, consumed) && consumed == 0 && queue.size() == 3, "zero batch budget consumed work");
    bool failed = false;
    try
    {
        queue.popReadyBatch(output, 10, consumed);
    }
    catch (const std::runtime_error&)
    {
        failed = true;
    }
    StorageCheck(failed && consumed == 1 && output[0].m_value == 1 && output[1].m_value == -1 && queue.size() == 2,
        "batch exception lost its exact committed prefix");
    StorageCheck(queue.pop(nullptr) && queue.popReadyBatch(output, 10, consumed) && consumed == 1 && output[0].m_value == 3 && queue.empty(),
        "batch exception removed failing or later item");
    for (int i = 0; i < 1024; ++i)
    {
        StorageCheck(queue.tryPush(CTaskStorageTestAssignmentValue(i)) == LockFreeQueueSuccess, "batch segment fixture push");
    }
    int expected = 0;
    while (queue.popReadyBatch(output, (std::numeric_limits<size_t>::max)(), consumed))
    {
        StorageCheck(consumed <= 16, "batch exceeded fixed work limit");
        for (size_t i = 0; i < consumed; ++i)
        {
            StorageCheck(output[i].m_value == expected++, "batch crossing segment reordered or duplicated work");
        }
    }
    StorageCheck(expected == 1024 && consumed == 0 && queue.size() == 0, "batch final exact count");
    StorageCheck(queue.tryPush(CTaskStorageTestAssignmentValue(7, true)) == LockFreeQueueSuccess &&
            queue.popReadyBatch(nullptr, 10, consumed) && consumed == 1 && queue.empty(),
        "batch discard unnecessarily assigned output");

    size_t maintenanceCalls = 0;
    LockFreeQueue<CTaskStorageTestAssignmentValue> partial(false);
    StorageCheck(partial.configureCancelableReservations(reservations) == LockFreeQueueSuccess, "queue reservation configuration");
    // 策略只统计被调用次数，返回不缩容；验证异常前的成功项仍推动自动维护。
    StorageCheck(partial.setShrinkPolicy([&maintenanceCalls](const LockFreeQueueShrinkContext&) -> LockFreeQueueShrinkDecision
    {
        ++maintenanceCalls;
        const LockFreeQueueShrinkDecision decision = {0, 0};
        return decision;
    }) == LockFreeQueueSuccess &&
            partial.tryInit() == LockFreeQueueSuccess,
        "partial batch maintenance setup");
    for (int32_t i = 0; i < 128; ++i)
    {
        StorageCheck(partial.tryPush(CTaskStorageTestAssignmentValue(i, i % 16 == 15)) == LockFreeQueueSuccess,
            "partial batch maintenance input rejected");
    }
    const size_t before = maintenanceCalls;
    for (size_t round = 0; round < 8; ++round)
    {
        failed = false;
        try
        {
            partial.popReadyBatch(output, 16, consumed);
        }
        catch (const std::runtime_error&)
        {
            failed = true;
        }
        StorageCheck(failed && consumed == 15, "partial maintenance prefix changed");
        StorageCheck(partial.popReadyBatch(nullptr, 1, consumed) && consumed == 1, "partial maintenance failing head lost");
    }
    StorageCheck(partial.empty() && maintenanceCalls > before, "assignment failures starved automatic maintenance");
}

void taskQueueBatchTest()
{
    RunQueueBatchTest(false);
    RunQueueBatchTest(true);
}

/** 对两种发布顺序验证八生产者并发交付、有限容量和逐生产者 FIFO
@param [in] reservations 是否启用可取消预约模式
*/
static void RunQueueCounterTest(bool reservations)
{
    LockFreeQueue<size_t> queue(false);
    StorageCheck(queue.configureCancelableReservations(reservations) == LockFreeQueueSuccess, "queue reservation configuration");
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 1024;
    config.maximumCapacity = 1024;
    config.maximumProducers = 8;
    StorageCheck(queue.configureProducerCounters(true) == LockFreeQueueSuccess &&
            queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess,
        "concurrent counter queue initialization");
    const size_t count = 25000;
    std::atomic<bool> start(false);
    std::atomic<bool> stop(false);
    std::atomic<size_t> errors(0);
    std::vector<std::thread> threads;
    threads.reserve(8);
    size_t expected[8] = {};
    size_t consumed = 0;
    try
    {
        for (size_t p = 0; p < 8; ++p)
        {
            threads.push_back(std::thread([&queue, &start, &stop, &errors, p, count]()
            {
                try
                {
                    LockFreeQueueProducer<size_t> producer;
                    if (queue.tryAcquireProducer(producer) != LockFreeQueueSuccess)
                    {
                        ++errors;
                        stop.store(true);
                        return;
                    }
                    while (!start.load(std::memory_order_acquire) && !stop.load())
                    {
                        std::this_thread::yield();
                    }
                    for (size_t i = 0; i < count && !stop.load();)
                    {
                        const LockFreeQueueResult result = producer.tryPush(p * count + i);
                        if (result == LockFreeQueueSuccess)
                        {
                            ++i;
                        }
                        else if (result == LockFreeQueueCapacityLimit || result == LockFreeQueueTemporaryUnavailable || result == LockFreeQueueResourceUnavailable)
                        {
                            std::this_thread::yield();
                        }
                        else
                        {
                            ++errors;
                            stop.store(true);
                        }
                    }
                }
                catch (...)
                {
                    ++errors;
                    stop.store(true);
                }
            }));
        }
        start.store(true, std::memory_order_release);
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (consumed != count * 8 && !stop.load())
        {
            size_t values[16];
            size_t received = 0;
            if (queue.popReadyBatch(values, 16, received))
            {
                for (size_t i = 0; i < received; ++i)
                {
                    const size_t value = values[i];
                    StorageCheck(value / count < 8 && value % count == expected[value / count]++, "distributed counter FIFO or duplication");
                    ++consumed;
                }
            }
            else
            {
                queue.maintain(8);
                std::this_thread::yield();
            }
            StorageCheck(std::chrono::steady_clock::now() < deadline, "distributed counter progress timeout");
        }
    }
    catch (...)
    {
        stop.store(true);
        start.store(true);
        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].join();
        }
        throw;
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    StorageCheck(errors.load() == 0 && consumed == count * 8 && queue.size() == 0 && !queue.pop(nullptr),
        "distributed counter total or quiescent count");
    queue.destroy();
    StorageCheck(queue.tryInit() == LockFreeQueueSuccess && queue.size() == 0 &&
            queue.tryPush(7) == LockFreeQueueSuccess && queue.size() == 1 && queue.pop(nullptr) && queue.size() == 0,
        "distributed counter lifecycle reset");
}

void taskQueueCounterTest()
{
    RunQueueCounterTest(false);
    RunQueueCounterTest(true);
}

void taskBenchmarkValidatorTest()
{
    CTaskPreparedState valid(2, 10, 2, false);
    const size_t priorityOrder[] = {0, 2, 4, 1, 3, 5, 7, 9, 6, 8};
    for (size_t i = 0; i < 10; ++i)
    {
        valid.m_tasks[priorityOrder[i]]->doTask();
    }
    StorageCheck(valid.m_errors.load() == 0 && valid.m_completed == 10 && valid.m_finish.load() != 0 &&
            valid.m_expected[0] == 3 && valid.m_expected[1] == 2 && valid.m_expected[2] == 3 && valid.m_expected[3] == 2,
        "benchmark rejected valid per-producer priority FIFO");
    CTaskPreparedState duplicate(1, 4, 1, false);
    duplicate.m_tasks[0]->doTask();
    duplicate.m_tasks[0]->doTask();
    duplicate.m_tasks[2]->doTask();
    duplicate.m_tasks[3]->doTask();
    StorageCheck(duplicate.m_completed == 4 && duplicate.m_errors.load() != 0,
        "benchmark missed duplicate balanced by missing task");
    CTaskPreparedState reversed(1, 4, 1, false);
    reversed.m_tasks[1]->doTask();
    reversed.m_tasks[0]->doTask();
    reversed.m_tasks[2]->doTask();
    reversed.m_tasks[3]->doTask();
    StorageCheck(reversed.m_completed == 4 && reversed.m_errors.load() != 0, "benchmark missed reordered tasks");
    CTaskPreparedState missing(1, 4, 1, false);
    for (size_t i = 0; i < 3; ++i)
    {
        missing.m_tasks[i]->doTask();
    }
    StorageCheck(missing.m_completed == 3 && missing.m_expected[0] == 3 && missing.m_finish.load() == 0,
        "benchmark completed with a missing tail");
    CTaskPreparedState repeated(32, 32000, 10, false, false, true);
    StorageCheck(repeated.m_tasks.size() == 2000 && repeated.m_taskOffsets.back() == 2000,
        "reused benchmark did not precreate exactly 2000 task objects");
    for (size_t producer = 0; producer < 32; ++producer)
    {
        const size_t begin = repeated.m_taskOffsets[producer];
        const size_t count = repeated.m_taskOffsets[producer + 1] - begin;
        StorageCheck(count != 0 && count % 10 == 0, "reused producer range changed task priorities");
        for (size_t sequence = 0; sequence < 1000; ++sequence)
        {
            repeated.m_tasks[begin + sequence % count]->doTask();
        }
    }
    StorageCheck(repeated.m_completed == 32000 && repeated.m_errors.load() == 0 && repeated.m_finish.load() != 0,
        "reused validator rejected valid cycles and partial final cycles");
    for (size_t i = 0; i < repeated.m_expected.size(); ++i)
    {
        StorageCheck(repeated.m_expected[i] == 100, "reused per-producer per-priority total changed");
    }
    CTaskPreparedState reusedDuplicate(1, 4000, 1, false, false, true);
    for (size_t i = 0; i < 2000; ++i)
    {
        reusedDuplicate.m_tasks[i]->doTask();
    }
    reusedDuplicate.m_tasks[0]->doTask();
    reusedDuplicate.m_tasks[0]->doTask();
    for (size_t i = 2; i < 2000; ++i)
    {
        reusedDuplicate.m_tasks[i]->doTask();
    }
    StorageCheck(reusedDuplicate.m_completed == 4000 && reusedDuplicate.m_errors.load() != 0,
        "reused validator missed duplicate/missing identities after a cycle boundary");
}

void taskMapMaintenanceTest()
{
    LockFreeMap<int, int> values;
    for (int round = 0; round < 3; ++round)
    {
        for (int key = 0; key < 256; ++key)
        {
            StorageCheck(values.lockFreeMapSet(key, round * 1000 + key) == LOCK_FREE_MAP_COMMITTED, "map maintenance setup");
        }
        std::atomic<bool> stop(false);
        std::atomic<bool> failure(false);
        std::thread maintenance([&values, &stop, &failure]()
        {
            // 参数由引用捕获；维护与更新并行，异常记录后退出，不穿出线程入口。
            try
            {
                while (!stop.load(std::memory_order_acquire))
                {
                    size_t released = 0;
                    if (values.lockFreeMapMaintain(16, released) != LOCK_FREE_MAP_OK)
                    {
                        failure.store(true, std::memory_order_release);
                    }
                    std::this_thread::yield();
                }
            }
            catch (...)
            {
                failure.store(true, std::memory_order_release);
            }
        });
        bool correct = true;
        try
        {
            for (int key = 0; key < 256; ++key)
            {
                int actual = -1;
                LockFreeMapToken token;
                correct = correct && values.lockFreeMapGet(key, actual, token) == LOCK_FREE_MAP_OK && actual == round * 1000 + key;
                if (key % 2 == 0)
                {
                    correct = correct && values.lockFreeMapErase(key, token) == LOCK_FREE_MAP_COMMITTED;
                }
            }
        }
        catch (...)
        {
            correct = false;
        }
        stop.store(true, std::memory_order_release);
        maintenance.join();
        StorageCheck(correct && !failure.load(std::memory_order_acquire) && values.size() == 128, "concurrent map maintenance changed content");
        for (int key = 0; key < 256; ++key)
        {
            int actual = -1;
            LockFreeMapToken token;
            const LockFreeMapStatus status = values.lockFreeMapGet(key, actual, token);
            StorageCheck(key % 2 == 0 ? status == LOCK_FREE_MAP_NOT_FOUND : status == LOCK_FREE_MAP_OK && actual == round * 1000 + key, "maintenance lost or resurrected key");
        }
        values.clear();
        for (size_t pass = 0; pass < 100; ++pass)
        {
            size_t released = 0;
            StorageCheck(values.lockFreeMapMaintain(1024, released) == LOCK_FREE_MAP_OK, "empty map maintenance failed");
        }
        StorageCheck(values.lockFreeMapStats().indexBytes == 0, "empty map retained unused index buffers");
        values.clearAndFreeMemory();
    }
}

/** 验证显式前缀扫描绕过短项提示，仍保留预算、所有权和跨段 FIFO
@param [in] dynamic 是否使用可扩容额度，另一路验证固定额度的直接输出
@param [in] span 是否验证最多256项接口；false验证原64项接口的输出边界
*/
static void RunEntryProbeTest(bool dynamic, bool span)
{
    LockFreeQueueEntry<int> entries[600];
    LockFreeQueueEntry<int>* output[256];
    LockFreeQueue<int> queue(false);
    const size_t limit = span ? 256 : 64;
    /** 按待测接口提取已发布前缀，不改变队列的消费预算
    @param [out] destination 输出条目数组，可空以验证参数拒绝
    @param [in] budget 调用方允许消费的最多条目数
    @param [out] received 本次实际消费数
    @return 是否消费了至少一个条目
    */
    const auto probe = [&](LockFreeQueueEntry<int>** destination, size_t budget, size_t& received) -> bool
    {
        return span ? queue.probeReadyEntrySpan(destination, budget, received) :
            queue.probeReadyEntryPrefix(destination, budget, received);
    };
    size_t count = 99;
    StorageCheck(!probe(output, limit, count) && count == 0,
        "uninitialized explicit prefix changed output count");
    LockFreeQueuePoolConfig config;
    config.initialCapacity = dynamic ? 2 : 1024;
    config.maximumCapacity = 1024;
    config.maximumProducers = 4;
    StorageCheck(queue.configureEntryOnly(true) == LockFreeQueueSuccess &&
            queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess,
        "explicit prefix initialization");
    entries[0].m_value = -1;
    StorageCheck(queue.tryPushEntry(entries[0]) == LockFreeQueueSuccess &&
            queue.popReadyEntryPrefix(output, 64, count) && count == 1 && output[0] == &entries[0],
        "short prefix fixture");
    for (int i = 0; i < 600; ++i)
    {
        entries[i].m_value = i;
        StorageCheck(queue.tryPushEntry(entries[i]) == LockFreeQueueSuccess, "explicit prefix fixture publication");
    }
    StorageCheck(!probe(output, 0, count) && count == 0 && queue.size() == 600,
        "zero explicit prefix budget consumed entries");
    StorageCheck(probe(output, 4, count) && count == 4,
        "explicit scan retained the single-item hint");
    for (size_t i = 0; i < count; ++i)
    {
        StorageCheck(output[i] == &entries[i] && output[i]->m_value == static_cast<int>(i),
            "explicit prefix changed entry identity");
    }
    size_t consumed = count;
    size_t largest = count;
    while (consumed < 600)
    {
        for (size_t i = 0; i < 256; ++i)
        {
            output[i] = &entries[599];
        }
        const size_t maximum = consumed % 2 != 0 ? 1 : (std::numeric_limits<size_t>::max)();
        StorageCheck(probe(output, maximum, count) && count != 0 &&
                count <= (std::min)(maximum, limit), "explicit prefix budget or segment boundary");
        largest = (std::max)(largest, count);
        for (size_t i = 0; i < count; ++i)
        {
            StorageCheck(output[i] == &entries[consumed] && output[i]->m_value == static_cast<int>(consumed),
                "explicit prefix FIFO or caller-owned value changed");
            ++consumed;
        }
        for (size_t i = count; i < 256; ++i)
        {
            StorageCheck(output[i] == &entries[599], "explicit prefix wrote beyond its returned count");
        }
    }
    StorageCheck(!span || dynamic || largest > 64, "fixed span never exercised its extended output range");
    StorageCheck(!probe(output, limit, count) && count == 0 && queue.size() == 0 &&
            queue.tryPushEntry(entries[0]) == LockFreeQueueSuccess && probe(nullptr, 1, count) &&
            count == 1 && queue.size() == 0 && entries[0].m_value == 0,
        "explicit prefix empty, discard or credit reuse");
    // 不足消费预算的短尾只能输出实际存在项，其后输出槽必须保持调用方原值。
    for (size_t available = 1; available <= 7; ++available)
    {
        for (size_t i = 0; i < 256; ++i)
        {
            output[i] = &entries[599];
        }
        for (size_t i = 0; i < available; ++i)
        {
            StorageCheck(queue.tryPushEntry(entries[i]) == LockFreeQueueSuccess, "short tail fixture");
        }
        StorageCheck(probe(output, limit, count) && count == available && queue.size() == 0,
            "short tail consumed an unpublished position");
        for (size_t i = 0; i < 256; ++i)
        {
            StorageCheck(output[i] == (i < available ? &entries[i] : &entries[599]),
                "short tail changed FIFO or wrote beyond the committed prefix");
        }
    }
}

/** 验证各段内起点及读取预算下的外部条目前缀，预期仅来自投递身份序列
@param [in] dynamic 是否使用动态额度路径，初始容量始终足够
@param [in] span 是否使用最多256项入口；false验证原64项入口
*/
static void RunEntryOffsetTest(bool dynamic, bool span)
{
    const size_t kBudgets[] = {0, 1, 2, 3, 4, 5, 15, 16, 17, 63, 64, 65, 255, 256, (std::numeric_limits<size_t>::max)()};
    for (size_t offset = 0; offset < 256; ++offset)
    {
        LockFreeQueueEntry<int> entries[600];
        LockFreeQueue<int> queue(false);
        LockFreeQueuePoolConfig config;
        config.initialCapacity = 1024;
        config.maximumCapacity = dynamic ? -1 : 1024;
        config.maximumProducers = 2;
        StorageCheck(queue.configureEntryOnly(true) == LockFreeQueueSuccess &&
                queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess,
            "offset prefix initialization");
        LockFreeQueueProducer<int> producer;
        StorageCheck(queue.tryAcquireProducer(producer) == LockFreeQueueSuccess, "offset producer registration");
        for (size_t i = 0; i < 600; ++i)
        {
            entries[i].m_value = static_cast<int>(i);
            StorageCheck(producer.tryPushEntry(entries[i]) == LockFreeQueueSuccess, "offset fixture publication");
        }
        // 单项推进到每个可能的初始偏移，不读取或修改队列私有成功状态。
        for (size_t i = 0; i < offset; ++i)
        {
            int value = -1;
            StorageCheck(queue.pop(&value) && value == static_cast<int>(i), "offset fixture FIFO");
        }
        size_t consumed = offset;
        size_t attempt = offset;
        while (consumed < 600)
        {
            LockFreeQueueEntry<int>* output[258] = {};
            const size_t budget = kBudgets[attempt++ % (sizeof(kBudgets) / sizeof(kBudgets[0]))];
            size_t count = 999;
            const bool received = span ? queue.probeReadyEntrySpan(output + 1, budget, count) :
                queue.probeReadyEntryPrefix(output + 1, budget, count);
            StorageCheck(output[0] == nullptr && output[257] == nullptr,
                "offset prefix wrote outside output storage");
            if (budget == 0)
            {
                StorageCheck(!received && count == 0 && queue.size() == 600 - consumed,
                    "offset zero budget changed consumption");
                continue;
            }
            const size_t limit = span && !dynamic ? 256 : 64;
            StorageCheck(received && count != 0 && count <= (std::min)(budget, limit) && count <= 600 - consumed,
                "offset prefix lost data or exceeded budget");
            for (size_t i = 0; i < count; ++i)
            {
                StorageCheck(output[i + 1] == &entries[consumed] && entries[consumed].m_value == static_cast<int>(consumed),
                    "offset prefix lost, duplicated or reordered identity");
                ++consumed;
            }
            for (size_t i = count + 1; i < 258; ++i)
            {
                StorageCheck(output[i] == nullptr, "offset prefix wrote beyond returned count");
            }
        }
        size_t count = 999;
        StorageCheck(queue.empty() && queue.size() == 0 &&
                !queue.probeReadyEntrySpan(nullptr, 256, count) && count == 0,
            "offset prefix retained or fabricated entries");
    }
}

/** 用相同独立预期验证两种外部载荷额度实现
@param [in] entryOnly 是否启用只借用模式；关闭时另验证普通载荷混用
*/
static void RunQueueEntryTest(bool entryOnly)
{
    LockFreeQueueEntry<int> entries[600];
    LockFreeQueue<int> mixed(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 1024;
    config.maximumCapacity = 1024;
    StorageCheck(mixed.configureEntryOnly(entryOnly) == LockFreeQueueSuccess &&
            mixed.configurePool(config) == LockFreeQueueSuccess && mixed.tryInit() == LockFreeQueueSuccess,
        "entry mixed initialization");
    StorageCheck(mixed.configureEntryOnly(!entryOnly) == LockFreeQueueInvalidState,
        "live queue allowed entry mode change");
    if (entryOnly)
    {
        StorageCheck(mixed.tryPush(7) == LockFreeQueueInvalidConfiguration && mixed.size() == 0,
            "entry-only queue accepted an owned payload");
    }
    LockFreeQueueProducer<int> mixedProducer;
    StorageCheck(mixed.tryAcquireProducer(mixedProducer) == LockFreeQueueSuccess, "entry mixed producer");
    for (int i = 0; i < 600; ++i)
    {
        entries[i].m_value = i;
        const LockFreeQueueResult result = entryOnly || i % 2 == 0 ? mixedProducer.tryPushEntryAndFence(entries[i]) : mixedProducer.tryPush(i);
        StorageCheck(result == LockFreeQueueSuccess, "mixed entry publication");
    }
    size_t consumed = 0;
    size_t none = 7;
    StorageCheck(!mixed.popReadyPrefix(nullptr, 0, none) && none == 0 && mixed.size() == 600,
        "zero prefix budget consumed an entry");
    while (consumed < 600)
    {
        int values[64];
        size_t count = 0;
        const bool large = consumed % 3 != 1;
        StorageCheck(large ? mixed.popReadyPrefix(values, 1000, count) : mixed.popReadyBatch(values, 1000, count),
            "mixed entry missing prefix");
        StorageCheck(count <= (large ? size_t(64) : size_t(16)) && (consumed != 0 || count == 64),
            "prefix exceeded its public budget or failed to collect the ready prefix");
        for (size_t i = 0; i < count; ++i)
        {
            StorageCheck(values[i] == static_cast<int>(consumed++) && entries[values[i]].m_value == values[i],
                "mixed FIFO or borrowed source modified");
        }
    }
    StorageCheck(mixed.size() == 0 && mixed.tryPushEntry(entries[0]) == LockFreeQueueSuccess && mixed.pop(nullptr),
        "entry reuse after completed pop");
    mixedProducer.release();

    if (entryOnly)
    {
        RunEntryProbeTest(false, false);
        RunEntryProbeTest(true, false);
        RunEntryProbeTest(false, true);
        RunEntryProbeTest(true, true);
        RunEntryOffsetTest(false, false);
        RunEntryOffsetTest(true, false);
        RunEntryOffsetTest(false, true);
        RunEntryOffsetTest(true, true);
    }
    else
    {
        LockFreeQueueEntry<int>* unexpected[1];
        size_t count = 99;
        StorageCheck(mixed.tryPush(42) == LockFreeQueueSuccess &&
                !mixed.probeReadyEntryPrefix(unexpected, 1, count) && count == 0 && mixed.size() == 1 &&
                !mixed.probeReadyEntrySpan(unexpected, 1, count) && count == 0 && mixed.size() == 1 &&
                mixed.pop(nullptr), "explicit Entry prefix consumed an owned payload");
    }

    LockFreeQueue<int> bounded(false);
    config.initialCapacity = 3;
    config.maximumCapacity = 3;
    StorageCheck(bounded.configureEntryOnly(entryOnly) == LockFreeQueueSuccess &&
            bounded.configurePool(config) == LockFreeQueueSuccess && bounded.tryInit() == LockFreeQueueSuccess,
        "entry bounded initialization");
    StorageCheck(bounded.tryPushEntry(entries[0]) == LockFreeQueueSuccess &&
            (entryOnly ? bounded.tryPushEntry(entries[1]) : bounded.tryPush(1)) == LockFreeQueueSuccess &&
            bounded.tryPushEntry(entries[2]) == LockFreeQueueCapacityLimit && bounded.size() == 2,
        "entry bypassed physical maximum");
    StorageCheck(bounded.pop(nullptr) && bounded.tryPushEntryAndFence(entries[2]) == LockFreeQueueSuccess,
        "entry capacity did not recover");
    bounded.clear();
    StorageCheck(bounded.size() == 0, "entry clear count");

    LockFreeQueue<int> growing(false);
    config.initialCapacity = 2;
    config.maximumCapacity = 20;
    StorageCheck(growing.configureEntryOnly(entryOnly) == LockFreeQueueSuccess && growing.configurePool(config) == LockFreeQueueSuccess &&
            growing.setGrowPolicy([](const LockFreeQueueGrowContext&) -> size_t
    {
        // 固定请求超过上限的增长量，由池截断；策略不持有队列状态。
        return 100;
    }) == LockFreeQueueSuccess &&
            growing.tryInit() == LockFreeQueueSuccess,
        "entry growing initialization");
    StorageCheck(growing.tryPushEntry(entries[0]) == LockFreeQueueSuccess, "entry first bounded allocation");
    CTaskFaultAllocation::begin(1);
    const LockFreeQueueResult allocationFailure = growing.tryPushEntry(entries[1]);
    CTaskFaultAllocation::end();
    StorageCheck(CTaskFaultAllocation::triggered() && allocationFailure == LockFreeQueueAllocationFailure &&
            growing.size() == 1 && growing.poolStatistics().currentCapacity == 2,
        "entry failed growth committed payload or retained budget");
    for (size_t i = 1; i < 19; ++i)
    {
        StorageCheck(growing.tryPushEntry(entries[i]) == LockFreeQueueSuccess, "entry growth did not recover");
    }
    StorageCheck(growing.tryPushEntry(entries[19]) == LockFreeQueueCapacityLimit &&
            growing.poolStatistics().currentCapacity == 20,
        "entry growth exceeded physical maximum");
    int grownValues[64];
    size_t grownCount = 0;
    StorageCheck(growing.popReadyPrefix(grownValues, 64, grownCount) && grownCount == 19,
        "entry grown prefix lost payload");
    for (size_t i = 0; i < grownCount; ++i)
    {
        StorageCheck(grownValues[i] == static_cast<int>(i), "entry grown prefix changed FIFO");
    }

    LockFreeQueue<int> fragmented(false);
    config.maximumCapacity = 65;
    StorageCheck(fragmented.configureEntryOnly(entryOnly) == LockFreeQueueSuccess && fragmented.configurePool(config) == LockFreeQueueSuccess &&
            fragmented.setGrowPolicy([](const LockFreeQueueGrowContext&) -> size_t
    {
        // 每批只增加一个额度，强制一次消费跨越 64 个独立节点块。
        return 1;
    }) == LockFreeQueueSuccess &&
            fragmented.setShrinkPolicy([](const LockFreeQueueShrinkContext& context) -> LockFreeQueueShrinkDecision
    {
        // 空闲一毫秒后请求归还全部扩容量；具体安全摘除由节点池完成。
        const LockFreeQueueShrinkDecision decision = {context.currentCapacity - context.initialCapacity, 1};
        return decision;
    }) == LockFreeQueueSuccess &&
            fragmented.tryInit() == LockFreeQueueSuccess,
        "fragmented entry initialization");
    for (size_t i = 0; i < 64; ++i)
    {
        StorageCheck(fragmented.tryPushEntry(entries[i]) == LockFreeQueueSuccess, "fragmented entry growth");
    }
    StorageCheck(fragmented.popReadyPrefix(grownValues, 64, grownCount) && grownCount == 64,
        "fragmented entry return lost prefix");
    for (size_t i = 0; i < 64; ++i)
    {
        StorageCheck(grownValues[i] == static_cast<int>(i), "fragmented entry FIFO changed");
    }
    for (size_t pass = 0; pass < 100 && fragmented.poolStatistics().currentCapacity != 2; ++pass)
    {
        fragmented.maintain(256);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    StorageCheck(fragmented.poolStatistics().currentCapacity == 2 && fragmented.size() == 0 &&
            fragmented.tryPushEntry(entries[0]) == LockFreeQueueSuccess && fragmented.pop(nullptr),
        "fragmented entry shrink retained released blocks or broke reuse");

    LockFreeQueueEntry<CTaskStorageTestAssignmentValue> bad(CTaskStorageTestAssignmentValue(2, true));
    LockFreeQueueEntry<CTaskStorageTestAssignmentValue> good(CTaskStorageTestAssignmentValue(3));
    LockFreeQueueEntry<CTaskStorageTestAssignmentValue> first(CTaskStorageTestAssignmentValue(1));
    LockFreeQueue<CTaskStorageTestAssignmentValue> failures(false);
    StorageCheck(failures.configureEntryOnly(entryOnly) == LockFreeQueueSuccess && failures.tryInit() == LockFreeQueueSuccess &&
            (entryOnly ? failures.tryPushEntry(first) : failures.tryPush(CTaskStorageTestAssignmentValue(1))) == LockFreeQueueSuccess &&
            failures.tryPushEntry(bad) == LockFreeQueueSuccess && failures.tryPushEntry(good) == LockFreeQueueSuccess,
        "entry exception setup");
    CTaskStorageTestAssignmentValue output[16];
    size_t prefix = 0;
    bool caught = false;
    try
    {
        failures.popReadyPrefix(output, 16, prefix);
    }
    catch (const std::runtime_error&)
    {
        caught = true;
    }
    StorageCheck(caught && prefix == 1 && output[0].m_value == 1 && failures.size() == 2 && bad.m_value.m_value == 2,
        "entry exception changed commit boundary");
    StorageCheck(failures.pop(nullptr) && failures.pop(&output[0]) && output[0].m_value == 3 && good.m_value.m_value == 3,
        "entry exception recovery or copied source ownership");

    std::shared_ptr<int> owned(new int(7));
    std::weak_ptr<int> observed = owned;
    LockFreeQueueEntry<std::shared_ptr<int>> lifetime(owned);
    owned.reset();
    LockFreeQueue<std::shared_ptr<int>> ownership(false);
    StorageCheck(ownership.configureEntryOnly(entryOnly) == LockFreeQueueSuccess && ownership.tryInit() == LockFreeQueueSuccess &&
            ownership.tryPushEntry(lifetime) == LockFreeQueueSuccess && lifetime.m_value.use_count() == 1,
        "borrowed publication copied ownership");
    ownership.destroy();
    StorageCheck(!observed.expired() && *lifetime.m_value == 7, "destroy destroyed caller-owned payload");
    lifetime.m_value.reset();
    StorageCheck(observed.expired(), "borrowed ownership leaked after owner release");

    const size_t perProducer = 20000;
    std::unique_ptr<LockFreeQueueEntry<size_t>[]> parallelEntries(new LockFreeQueueEntry<size_t>[perProducer * 8]);
    for (size_t i = 0; i < perProducer * 8; ++i)
    {
        parallelEntries[i].m_value = i;
    }
    LockFreeQueue<size_t> parallel(false);
    config.initialCapacity = 4096;
    config.maximumCapacity = 4096;
    StorageCheck(parallel.configureEntryOnly(entryOnly) == LockFreeQueueSuccess &&
            parallel.configurePool(config) == LockFreeQueueSuccess && parallel.configureProducerCounters(true) == LockFreeQueueSuccess &&
            parallel.tryInit() == LockFreeQueueSuccess,
        "entry parallel initialization");
    std::atomic<bool> start(false), stop(false);
    std::atomic<size_t> errors(0);
    std::vector<std::thread> threads;
    threads.reserve(8);
    size_t expected[8] = {};
    consumed = 0;
    try
    {
        for (size_t p = 0; p < 8; ++p)
        {
            threads.push_back(std::thread([&, p]()
            {
                try
                {
                    LockFreeQueueProducer<size_t> producer;
                    StorageCheck(parallel.tryAcquireProducer(producer) == LockFreeQueueSuccess, "entry parallel producer");
                    while (!start.load() && !stop.load())
                    {
                        std::this_thread::yield();
                    }
                    for (size_t i = 0; i < perProducer && !stop.load();)
                    {
                        const LockFreeQueueResult result = producer.tryPushEntryAndFence(parallelEntries[p * perProducer + i]);
                        if (result == LockFreeQueueSuccess)
                        {
                            ++i;
                        }
                        else
                        {
                            StorageCheck(result == LockFreeQueueTemporaryUnavailable || result == LockFreeQueueCapacityLimit || result == LockFreeQueueResourceUnavailable,
                                "entry unexpected concurrent publication failure");
                            std::this_thread::yield();
                        }
                    }
                }
                catch (...)
                {
                    ++errors;
                    stop.store(true);
                }
            }));
        }
        start.store(true);
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (consumed < perProducer * 8 && !stop.load())
        {
            size_t values[16], count = 0;
            parallel.popReadyBatch(values, 16, count);
            for (size_t i = 0; i < count; ++i)
            {
                StorageCheck(values[i] / perProducer < 8 && values[i] % perProducer == expected[values[i] / perProducer]++,
                    "entry concurrent duplicate or FIFO violation");
                ++consumed;
            }
            if (count == 0)
            {
                parallel.maintain(8);
                std::this_thread::yield();
            }
            StorageCheck(std::chrono::steady_clock::now() < deadline, "entry concurrent progress timeout");
        }
    }
    catch (...)
    {
        stop.store(true);
        start.store(true);
        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].join();
        }
        throw;
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    StorageCheck(errors.load() == 0 && consumed == perProducer * 8 && parallel.size() == 0 && !parallel.pop(nullptr),
        "entry concurrent total or final count");
}

void taskQueueEntryTest()
{
    RunQueueEntryTest(false);
    CTaskMoveAssignmentValue output;
    LockFreeQueueEntry<CTaskMoveAssignmentValue> entry(CTaskMoveAssignmentValue(19));
    // 断言或赋值抛异常时，队列也必须先于被借用的 Entry 析构。
    LockFreeQueue<CTaskMoveAssignmentValue> queue;
    StorageCheck(queue.tryPush(CTaskMoveAssignmentValue(17)) == LockFreeQueueSuccess &&
            queue.tryPushEntry(entry) == LockFreeQueueSuccess && queue.size() == 2 &&
            queue.pop(&output) && output.m_value == 17,
        "move-only assignment setup");
    CTaskMoveAssignmentValue::m_failCopy = true;
    bool caught = false;
    size_t copied = 99;
    try
    {
        queue.popReadyPrefix(&output, 1, copied);
    }
    catch (const std::runtime_error& error)
    {
        caught = std::string(error.what()) == "copy failure";
    }
    catch (...)
    {
        CTaskMoveAssignmentValue::m_failCopy = false;
        throw;
    }
    CTaskMoveAssignmentValue::m_failCopy = false;
    StorageCheck(caught && copied == 0 && output.m_value == 17 && queue.size() == 1 && entry.m_value.m_value == 19,
        "borrowed copy-constructor failure committed or changed the source");
    StorageCheck(queue.pop(&output) && output.m_value == 19 &&
            entry.m_value.m_value == 19 && queue.empty(),
        "entry output broke ordinary move assignment or changed caller data");
}

void taskEntryQuotaTest()
{
    RunQueueEntryTest(true);
    // 静止迁移累计序号，保持领取减归还为零；下一额度必须可用，之后必须拒绝回绕。
    LockFreeQueueArena<int> arena;
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 2;
    StorageCheck(arena.configureEntryOnly(true) == LockFreeQueueSuccess &&
            arena.configure(config) == LockFreeQueueSuccess && arena.initialize() == LockFreeQueueSuccess,
        "external quota boundary initialization");
    LockFreeQueueArena<int>::Record* record = arena.enter();
    StorageCheck(record != nullptr, "external quota boundary registration");
    LockFreeQueueResult result = LockFreeQueueSuccess;
    LockFreeQueueArena<int>::Node* credit = arena.acquireNode(*record, result);
    StorageCheck(credit != nullptr && result == LockFreeQueueSuccess, "external quota first credit");
    LockFreeQueueArena<int>::Block* block = arena.creditOwner(credit);
    arena.releaseNode(credit, record);
    block->m_externalClaims.store(UINT64_MAX - 1);
    block->m_externalRefunds.store(UINT64_MAX - 1);
    credit = arena.acquireNode(*record, result);
    StorageCheck(credit != nullptr && result == LockFreeQueueSuccess && block->m_externalClaims.load() == UINT64_MAX,
        "external quota final sequence was not usable");
    arena.releaseNode(credit, record);
    StorageCheck(arena.acquireNode(*record, result) == nullptr && result == LockFreeQueueSequenceExhausted &&
            block->m_externalClaims.load() == UINT64_MAX && block->m_externalRefunds.load() == UINT64_MAX,
        "external quota sequence wrapped or exhaustion was hidden");
    arena.leave(*record, true);
}

void taskReadyIndexTest()
{
    CTaskReadyIndex index;
    StorageCheck(index.highest() == 1 && !index.lower(1), "ready index initial floor");
    index.publish(16);
    index.publish(3);
    StorageCheck(index.highest() == 16 && !index.lower(15), "lower priority decreased highest index");
    for (int32_t level = 16; level > 1; --level)
    {
        StorageCheck(index.lower(level) && index.highest() == level - 1, "ready index skipped a level");
    }
    std::atomic<uint64_t> pending[16];
    for (size_t i = 0; i < 16; ++i)
    {
        pending[i].store(0);
    }
    std::atomic<bool> stop(false);
    std::atomic<size_t> errors(0);
    std::vector<std::thread> producers;
    producers.reserve(8);
    uint64_t consumed = 0;
    try
    {
        for (size_t producer = 0; producer < 8; ++producer)
        {
            // 原子计数模拟已发布队列，随机交错最高位置发布；消费者按提示逐级查找。
            producers.push_back(std::thread([producer, &pending, &index, &stop, &errors]()
            {
                try
                {
                    for (size_t operation = 0; operation < 20000 && !stop.load(); ++operation)
                    {
                        const size_t level = (operation * 7 + producer * 11) & 15;
                        pending[level].fetch_add(1, std::memory_order_release);
                        if ((operation & 127) == 0)
                        {
                            std::this_thread::yield();
                        }
                        if ((operation & 1) != 0)
                        {
                            // 与Queue已完成提交屏障的接入相同，不能无屏障直接传true。
                            CTaskReadyIndex::fence();
                            index.publish(static_cast<int32_t>(level + 1), true);
                        }
                        else
                        {
                            index.publish(static_cast<int32_t>(level + 1));
                        }
                    }
                }
                catch (...)
                {
                    ++errors;
                    stop = true;
                }
            }));
        }
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        while (consumed < 160000 && !stop.load())
        {
            const int32_t level = index.highest();
            if (pending[level - 1].load(std::memory_order_acquire) != 0)
            {
                pending[level - 1].fetch_sub(1, std::memory_order_relaxed);
                ++consumed;
            }
            else if (index.lower(level) && pending[level - 1].load(std::memory_order_acquire) != 0)
            {
                index.publish(level);
            }
            StorageCheck(std::chrono::steady_clock::now() < deadline, "ready index lost a publication");
        }
    }
    catch (...)
    {
        stop = true;
        for (size_t i = 0; i < producers.size(); ++i)
        {
            producers[i].join();
        }
        throw;
    }
    for (size_t i = 0; i < producers.size(); ++i)
    {
        producers[i].join();
    }
    StorageCheck(errors.load() == 0 && consumed == 160000, "ready index failed concurrent publication");
    for (size_t i = 0; i < 16; ++i)
    {
        StorageCheck(pending[i].load() == 0, "ready index left unconsumed work");
    }
}

/** 验证稀疏登记和同时扩展范围不会漏掉旧读者，关闭仍保留全部在途借用
*/
static void RunSparseReadDomainTest()
{
    const size_t indices[8] = {0, 1, 7, 15, 31, 63, 127, 255};
    for (size_t round = 0; round < 16; ++round)
    {
        CTaskReadDomain domain;
        StorageCheck(domain.initialize(256) && domain.active() == 0 && domain.oldest() == UINT64_MAX,
            "sparse domain initial state");
        StorageCheck(domain.tryEnter(256) == nullptr && domain.tryEnter(SIZE_MAX) == nullptr,
            "invalid sparse position changed protection range");
        std::atomic<bool> start(false);
        std::atomic<bool> release(false);
        std::atomic<size_t> entered(0);
        std::atomic<size_t> errors(0);
        std::atomic<uint64_t> epochs[8];
        std::vector<std::thread> readers;
        readers.reserve(8);
        for (size_t i = 0; i < 8; ++i)
        {
            epochs[i].store(UINT64_MAX);
        }
        try
        {
            for (size_t i = 0; i < 8; ++i)
            {
                // 每个读者独占一个稀疏位置；公布后保持借用，直到主线程完成回收及关闭检查。
                readers.push_back(std::thread([&, i]()
                {
                    while (!start.load(std::memory_order_acquire))
                    {
                        std::this_thread::yield();
                    }
                    CTaskReadPosition* position = domain.tryEnter(indices[i]);
                    if (position == nullptr)
                    {
                        errors.fetch_add(1);
                    }
                    else
                    {
                        epochs[i].store(position->m_epoch.load(std::memory_order_relaxed), std::memory_order_release);
                    }
                    entered.fetch_add(1, std::memory_order_release);
                    while (!release.load(std::memory_order_acquire))
                    {
                        std::this_thread::yield();
                    }
                    if (position != nullptr)
                    {
                        domain.leave(*position);
                    }
                }));
            }
            start.store(true, std::memory_order_release);
            const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (entered.load(std::memory_order_acquire) != 8)
            {
                StorageCheck(std::chrono::steady_clock::now() < deadline, "sparse reader publication stalled");
                domain.advance();
                std::this_thread::yield();
            }
            uint64_t minimum = UINT64_MAX;
            for (size_t i = 0; i < 8; ++i)
            {
                minimum = (std::min)(minimum, epochs[i].load(std::memory_order_acquire));
            }
            StorageCheck(errors.load() == 0 && domain.active() == 8 && domain.oldest() == minimum,
                "sparse/concurrent range omitted an active reader");
            StorageCheck(domain.tryEnter(255) == nullptr, "occupied last position was borrowed twice");
            domain.advance();
            StorageCheck(domain.oldest() == minimum, "epoch advance released a paused sparse reader");
            domain.close();
            StorageCheck(domain.tryEnter(200) == nullptr && domain.active() == 8,
                "close admitted a new sparse reader or lost an old lease");
        }
        catch (...)
        {
            domain.close();
            start.store(true, std::memory_order_release);
            release.store(true, std::memory_order_release);
            for (size_t i = 0; i < readers.size(); ++i)
            {
                readers[i].join();
            }
            throw;
        }
        release.store(true, std::memory_order_release);
        for (size_t i = 0; i < readers.size(); ++i)
        {
            readers[i].join();
        }
        StorageCheck(domain.active() == 0 && domain.oldest() == UINT64_MAX,
            "sparse domain retained a returned lease");
    }
}

void taskReadDomainTest()
{
    RunSparseReadDomainTest();
    CTaskReadDomain domain;
    StorageCheck(domain.enter() == nullptr && !domain.initialize(0), "uninitialized domain accepted a reader");
    StorageCheck(domain.initialize(4) && !domain.initialize(4), "domain initialization/reinitialization contract");
    CTaskReadPosition* first = domain.enter();
    StorageCheck(first != nullptr && domain.active() == 1, "reader not registered");
    const uint64_t retired = domain.current();
    StorageCheck(domain.advance() == retired && domain.oldest() <= retired, "paused reader did not pin retired epoch");
    CTaskReadPosition* second = domain.enter();
    CTaskReadPosition* third = domain.enter();
    CTaskReadPosition* fourth = domain.enter();
    StorageCheck(second != nullptr && third != nullptr && fourth != nullptr && domain.enter() == nullptr,
        "protection budget exceeded or reentrant readers share a position");
    domain.leave(*first);
    StorageCheck(domain.oldest() > retired, "new readers unnecessarily pin old retirement");
    domain.close();
    StorageCheck(domain.closed() && domain.enter() == nullptr && domain.active() == 3, "close lost existing leases");
    domain.leave(*second);
    domain.leave(*third);
    domain.leave(*fourth);
    StorageCheck(domain.active() == 0 && domain.oldest() == UINT64_MAX, "closed domain failed to quiesce");

    CTaskReadDomain concurrent;
    StorageCheck(concurrent.initialize(16), "concurrent domain initialization");
    std::atomic<bool> start(false);
    std::atomic<uint64_t> accepted(0);
    std::atomic<uint64_t> errors(0);
    std::vector<std::thread> readers;
    readers.reserve(8);
    try
    {
        for (size_t i = 0; i < 8; ++i)
        {
            readers.push_back(std::thread([&concurrent, &start, &accepted, &errors]()
            {
                try
                {
                    while (!start.load(std::memory_order_acquire))
                    {
                        std::this_thread::yield();
                    }
                    for (size_t operation = 0; operation < 100000; ++operation)
                    {
                        CTaskReadPosition* position = concurrent.enter();
                        if (position != nullptr)
                        {
                            if (position->m_epoch.load(std::memory_order_relaxed) == 0)
                            {
                                ++errors;
                            }
                            ++accepted;
                            concurrent.leave(*position);
                        }
                        else if (concurrent.closed())
                        {
                            break;
                        }
                    }
                }
                catch (...)
                {
                    ++errors;
                }
            }));
        }
        start.store(true, std::memory_order_release);
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (accepted.load(std::memory_order_acquire) < 10000)
        {
            StorageCheck(std::chrono::steady_clock::now() < deadline, "readers made no progress");
            concurrent.advance();
            std::this_thread::yield();
        }
        concurrent.close();
    }
    catch (...)
    {
        concurrent.close();
        start.store(true, std::memory_order_release);
        for (size_t i = 0; i < readers.size(); ++i)
        {
            readers[i].join();
        }
        throw;
    }
    for (size_t i = 0; i < readers.size(); ++i)
    {
        readers[i].join();
    }
    StorageCheck(accepted.load() >= 10000 && errors.load() == 0 && concurrent.active() == 0 && concurrent.enter() == nullptr,
        "concurrent entry/advance/close failed");
}

#ifdef CTTM_STORAGE_MODULE_MAIN
/** 独立构建时执行保护域、记录池及队列模块检查
@return 0表示所选检查成功，非零表示失败或当前组合不适用
*/
int main()
{
    try
    {
        taskReadDomainTest();
        taskStorageTest();
        taskStorageFailureTest();
        taskQueueReadyTest();
        taskReadyIndexTest();
        taskQueueCounterTest();
        taskQueueBatchTest();
        std::cout << "PASS read-domain boundary, reentrancy, paused-reader, concurrent-close\n";
        std::cout << "PASS storage capacity, retirement, growth, shrink, concurrent-allocation\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "FAIL unknown exception\n";
        return 1;
    }
}

#endif
#endif