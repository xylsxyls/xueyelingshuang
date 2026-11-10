#include "LockFreeMapTestThread.h"
#include "LockFreeMapTest.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include <thread>

/** 独立于有序容器算法的池模块测试
@return 任一独立预期失败时返回非零
*/
int LockFreeMapPoolModule()
{
    int failed = 0;
    try
    {
        LockFreeMapReclamation domain(4);
        LockFreeMapGuard guard(domain);
        LockFreeMapPoolConfig config;
        config.initialCapacity = 2;
        config.maximumCapacity = 3;
        config.blockRecordLimit = 4;
        config.grow = [](const LockFreeMapGrowContext&) -> size_t
        {
            return 10;
        };
        config.shrink = [](const LockFreeMapShrinkContext& context) -> LockFreeMapShrinkDecision
        {
            return LockFreeMapShrinkDecision(context.reusableCapacity, 1);
        };
        LockFreeMapPool<int> pool(config, domain);
        if (pool.initialize() != LOCK_FREE_MAP_OK)
        {
            return 2;
        }
        LockFreeMapStatus status;
        LockFreeMapPoolSlot<int>* first = pool.create(1, 1, guard, status);
        LockFreeMapPoolSlot<int>* second = pool.create(2, 2, guard, status);
        if (first == nullptr || second == nullptr || pool.stats().capacity != 2 || pool.stats().successfulGrowCount != 0)
        {
            ++failed;
        }
        LockFreeMapPoolSlot<int>* third = pool.create(3, 3, guard, status);
        if (third == nullptr || pool.stats().capacity != 3 || pool.stats().successfulGrowCount != 1)
        {
            ++failed;
        }
        if (pool.create(4, 4, guard, status) != nullptr || status != LOCK_FREE_MAP_CAPACITY)
        {
            ++failed;
        }
        pool.release(second);
        second = pool.create(22, 22, guard, status);
        if (second == nullptr || second->value() != 22 || pool.stats().capacity != 3)
        {
            ++failed;
        }
        pool.release(third);
        size_t released = 0;
        pool.maintain(10, released);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        pool.maintain(10, released);
        pool.maintain(10, released);
        if (pool.stats().capacity != 2 || pool.stats().retained != 2 || released != 1)
        {
            ++failed;
        }
        // 已发布的块保护指针必须阻止退役后的物理释放
        LockFreeMapPoolSlot<int>* held = pool.create(9, 9, guard, status);
        if (held == nullptr)
        {
            return 3;
        }
        LockFreeMapPoolBlock<int>* block = held->m_block;
        pool.release(held);
        LockFreeMapGuard heldGuard(domain);
        heldGuard.m_record->m_pointer[2].store(block);
        pool.maintain(10, released);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        pool.maintain(10, released);
        pool.maintain(10, released);
        if (pool.stats().capacity != 3 || pool.stats().reusable != 0)
        {
            ++failed;
        }
        if (pool.create(10, 10, guard, status) != nullptr || status != LOCK_FREE_MAP_CAPACITY)
        {
            ++failed;
        }
        // 解除独立保护后才允许物理释放
        heldGuard.m_record->m_pointer[2].store(nullptr);
        pool.maintain(10, released);
        if (pool.stats().capacity != 2)
        {
            ++failed;
        }
        pool.release(first);
        pool.release(second);
        pool.freeMemory();
        if (pool.stats().capacity != 0 || pool.stats().bytes != 0 || pool.stats().retained != 0)
        {
            ++failed;
        }
        // 暂停的缩容回调不能把块的连续空闲起点提前
        std::atomic<int> callbackEntered(0), callbackRelease(0), pausePolicy(1);
        LockFreeMapPoolConfig timedConfig;
        timedConfig.initialCapacity = 0;
        timedConfig.maximumCapacity = 1;
        timedConfig.blockRecordLimit = 4;
        timedConfig.grow = [](const LockFreeMapGrowContext&)
        {
            return size_t(1);
        };
        timedConfig.shrink = [&](const LockFreeMapShrinkContext&)
        {
            if (pausePolicy.exchange(0) != 0)
            {
                callbackEntered.store(1);
                const uint64_t deadline = LockFreeMapClock::milliseconds() + 10000;
                while (callbackRelease.load() == 0 && LockFreeMapClock::milliseconds() < deadline)
                {
                    std::this_thread::yield();
                }
                if (callbackRelease.load() == 0)
                {
                    LockFreeMapTestThread::m_errors.fetch_add(1);
                }
            }
            return LockFreeMapShrinkDecision(1, 50);
        };
        LockFreeMapPool<int> timed(timedConfig, domain);
        // 满池申请会触发维护，首次取得槽位后才启用暂停
        pausePolicy.store(0);
        LockFreeMapPoolSlot<int>* timedSlot = timed.create(1, 1, guard, status);
        if (timedSlot == nullptr)
        {
            return 4;
        }
        timed.release(timedSlot);
        pausePolicy.store(1);
        LockFreeMapTestThread maintenance([&]()
        {
            size_t amount = 0; timed.maintain(4, amount);
        });
        const uint64_t deadline = LockFreeMapClock::milliseconds() + 10000;
        while (callbackEntered.load() == 0 && LockFreeMapClock::milliseconds() < deadline)
        {
            std::this_thread::yield();
        }
        if (callbackEntered.load() == 0)
        {
            ++failed;
        }
        const uint64_t reuseTime = LockFreeMapClock::milliseconds() + 80;
        while (LockFreeMapClock::milliseconds() < reuseTime)
        {
            std::this_thread::yield();
        }
        timedSlot = timed.create(2, 2, guard, status);
        if (timedSlot == nullptr)
        {
            ++failed;
        }
        timed.release(timedSlot);
        callbackRelease.store(1);
        maintenance.join();
        timed.maintain(4, released);
        timed.maintain(4, released);
        if (timed.stats().capacity != 1)
        {
            ++failed;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        timed.maintain(4, released);
        timed.maintain(4, released);
        if (timed.stats().capacity != 0)
        {
            ++failed;
        }
        LockFreeMapPoolConfig ratioConfig;
        ratioConfig.initialCapacity = 2;
        ratioConfig.maximumCapacity = 5;
        // 向上取整的新增容量仍必须按实际剩余额度截断
        ratioConfig.grow = [](const LockFreeMapGrowContext& context) -> size_t
        {
            return context.currentCapacity / 2 + 1;
        };
        LockFreeMapPool<int> ratio(ratioConfig, domain);
        if (ratio.initialize() != LOCK_FREE_MAP_OK)
        {
            return 5;
        }
        LockFreeMapPoolSlot<int>* slots[5] = {};
        for (int index = 0; index < 5; ++index)
        {
            slots[index] = ratio.create(index, index + 1, guard, status);
            if (slots[index] == nullptr)
            {
                ++failed;
            }
        }
        if (ratio.stats().capacity != 5 || ratio.stats().successfulGrowCount != 2)
        {
            ++failed;
        }
        for (int index = 0; index < 5; ++index)
        {
            ratio.release(slots[index]);
        }
        LockFreeMapPoolConfig recordsConfig;
        recordsConfig.initialCapacity = 0;
        recordsConfig.blockRecordLimit = 1;
        recordsConfig.grow = [](const LockFreeMapGrowContext&) -> size_t
        {
            return 1;
        };
        LockFreeMapPool<int> records(recordsConfig, domain);
        LockFreeMapPoolSlot<int>* original = records.create(1, 1, guard, status);
        if (original == nullptr || records.create(2, 2, guard, status) != nullptr || status != LOCK_FREE_MAP_RECORD_LIMIT)
        {
            ++failed;
        }
        records.release(original);
        LockFreeMapPoolSlot<int>* reused = records.create(3, 3, guard, status);
        if (reused == nullptr || reused != original || reused->m_version != 3 || records.stats().reserved != 0)
        {
            ++failed;
        }
        records.release(reused);
        LockFreeMapPool<int> deferred(recordsConfig, domain, true);
        LockFreeMapPoolSlot<int>* borrowed = deferred.create(9, 9, guard, status);
        if (borrowed == nullptr)
        {
            ++failed;
        }
        else
        {
            // 多个独立拥有者可并发释放
            // 原拥有者仍持有时，不允许提前析构或复用
            for (size_t index = 0; index < 8; ++index)
            {
                deferred.retain(borrowed);
            }
            LockFreeMapTestThread releases[8];
            size_t started = 0;
            try
            {
                for (; started < 8; ++started)
                {
                    releases[started] = LockFreeMapTestThread([&deferred, borrowed]()
                    {
                        deferred.release(borrowed);
                    });
                }
            }
            catch (...)
            {
                ++failed;
                for (size_t index = started; index < 8; ++index)
                {
                    deferred.release(borrowed);
                }
            }
            for (size_t index = 0; index < started; ++index)
            {
                releases[index].join();
            }
            if (deferred.stats().retained != 1 || deferred.stats().reusable != 0 || borrowed->value() != 9)
            {
                ++failed;
            }
            if (deferred.tryRetain(borrowed))
            {
                deferred.release(borrowed);
            }
            else
            {
                ++failed;
            }
            guard.m_record->m_pointer[1].store(borrowed);
            deferred.release(borrowed);
            size_t reclaimed = 0;
            deferred.maintain(4096, reclaimed);
            if (deferred.tryRetain(borrowed) || borrowed->value() != 9 || deferred.stats().retained != 1)
            {
                ++failed;
            }
            guard.m_record->m_pointer[1].store(nullptr);
            deferred.maintain(4096, reclaimed);
            if (deferred.stats().retained != 0 || deferred.stats().reusable != 1)
            {
                ++failed;
            }
        }
    }
    catch (...)
    {
        ++failed;
    }

    return failed == 0 ? 0 : 1;
}