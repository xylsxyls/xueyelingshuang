#include "LockFreeHashMapEpochTests.h"
#include "LockFreeHashMapTestThread.h"
#include "LockFreeHashMapTestState.h"
#include <chrono>
#include <iostream>
#include <limits>
#include <vector>

size_t LockFreeHashMapEpochTests::s_checks = 0;
size_t LockFreeHashMapEpochTests::s_failed = 0;

void LockFreeHashMapEpochTestAccess::clock(LockFreeHashMapEpoch& domain, uint64_t value)
{
    LockFreeHashMapTestState::clock(domain).store(value);
}

void LockFreeHashMapEpochTests::check(bool condition, const char* name)
{
    ++s_checks;
    if (!condition)
    {
        ++s_failed;
        std::cerr << "EPOCH_FAILURE " << name << std::endl;
    }
}

void LockFreeHashMapEpochTests::admission()
{
    LockFreeHashMapEpoch domain;
    check(domain.supported(), "64-bit atomic backend");
    check(!domain.initialize(0) && domain.count() == 0, "zero capacity rejected");
    check(!domain.initialize((std::numeric_limits<size_t>::max)()), "byte overflow rejected");
    check(domain.initialize(2) && !domain.initialize(2), "one-time initialization");
    check(domain.record(2) == nullptr && domain.minimum() == 0, "bounds and empty domain");
    check((reinterpret_cast<uintptr_t>(domain.record(0)) & 63) == 0 &&
        (reinterpret_cast<uintptr_t>(domain.record(1)) & 63) == 0, "cache-line alignment");
    {
        LockFreeHashMapEpochGuard first(&domain);
        check(first.record() != nullptr && domain.minimum() == 1, "first reader publishes epoch");
        domain.advance();
        {
            LockFreeHashMapEpochGuard nested(&domain);
            LockFreeHashMapEpochGuard rejected(&domain);
            check(nested.record() != nullptr && nested.record() != first.record(), "nested reader gets distinct record");
            check(rejected.record() == nullptr && domain.minimum() == 1, "full domain refuses without waiting");
        }
        LockFreeHashMapEpochGuard reused(&domain);
        check(reused.record() != nullptr && reused.record()->m_epoch.load() == 2, "released record reused at fresh epoch");
    }
    check(domain.minimum() == 0, "all records released");
    LockFreeHashMapEpochTestAccess::clock(domain, (std::numeric_limits<uint64_t>::max)() - 1);
    check(domain.advance() == (std::numeric_limits<uint64_t>::max)() - 1 &&
        domain.advance() == (std::numeric_limits<uint64_t>::max)() && domain.current() != 0, "clock saturates without ABA");
}

void LockFreeHashMapEpochTests::paused()
{
    LockFreeHashMapEpoch domain;
    if (!domain.initialize(8))
    {
        check(false, "paused domain initialization");
        return;
    }
    int oldValue = 7;
    int newValue = 9;
    std::atomic<int*> source(&oldValue);
    std::atomic<bool> ready(false), resume(false), correct(false);
    // 子线程在保护发布和来源读取之后暂停，引用不越过保护生命周期
    LockFreeHashMapTestThread reader([&]()
    {
        LockFreeHashMapEpochGuard guard(&domain);
        int* value = guard.record() != nullptr ? source.load() : nullptr;
        ready.store(true);
        while (!resume.load())
        {
            std::this_thread::yield();
        }
        correct.store(value != nullptr && *value == 7);
    });
    try
    {
        while (!ready.load())
        {
            std::this_thread::yield();
        }
        source.store(&newValue);
        uint64_t retired = domain.advance();
        uint64_t cutoff = domain.current();
        check(retired < cutoff && domain.minimum() <= retired, "paused reader prevents old reuse");
        {
            LockFreeHashMapEpochGuard other(&domain);
            check(other.record() != nullptr && *source.load() == 9, "new reader progresses during pause");
        }
        resume.store(true);
        reader.join();
        check(correct.load() && domain.minimum() == 0, "paused reader leaves with intact data");
        {
            LockFreeHashMapEpochGuard newer(&domain);
            check(newer.record() != nullptr && domain.minimum() > retired, "newer reader does not retain older object");
        }
    }
    catch (...)
    {
        // 检查或输出异常也先解除本测试设置的暂停，避免回栈时 join 等待自己
        resume.store(true);
        reader.join();
        throw;
    }
}

void LockFreeHashMapEpochTests::retirementOrder()
{
    LockFreeHashMapEpoch domain;
    if (!domain.initialize(2))
    {
        check(false, "retirement domain initialization");
        return;
    }
    uint64_t cutoff = domain.current();
    uint64_t minimum = domain.minimum();
    LockFreeHashMapEpochGuard late(&domain);
    uint64_t laterRetirement = domain.advance();
    check(minimum == 0 && !(laterRetirement < cutoff), "old empty scan cannot reclaim later retirement");
    check(late.record() != nullptr && domain.minimum() <= laterRetirement, "late reader still protects later retirement");
}

void LockFreeHashMapEpochTests::contention()
{
    LockFreeHashMapEpoch domain;
    if (!domain.initialize(16))
    {
        check(false, "contention domain initialization");
        return;
    }
    LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<uint64_t> errors(0), completed(0), refused(0);
    std::vector<LockFreeHashMapTestThread> threads;
    threads.reserve(8);
    for (size_t i = 0; i < 8; ++i)
    {
        // 每个线程在已接纳记录中写入独占提示，再核对未被其他线程覆盖
        threads.push_back(LockFreeHashMapTestThread([&, i]()
        {
            for (size_t round = 0; round < 100000; ++round)
            {
                LockFreeHashMapEpochGuard guard(&domain);
                LockFreeHashMapEpochRecord* record = guard.record();
                if (record == nullptr)
                {
                    // 有界扫描允许竞争期间未接纳；拒绝不算已经取得保护，也不访问记录
                    refused.fetch_add(1);
                    continue;
                }
                record->m_blockHint = i;
                record->m_wordHint = round;
                if (record->m_epoch.load() == 0 || record->m_blockHint != i || record->m_wordHint != round)
                {
                    errors.fetch_add(1);
                }
                completed.fetch_add(1, std::memory_order_relaxed);
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    check(errors.load() == 0 && completed.load() > 0 && completed.load() + refused.load() == 800000 &&
        domain.minimum() == 0, "800000 concurrent admission attempts");
}

int LockFreeHashMapEpochTests::run()
{
    try
    {
        admission();
#ifndef LOCKFREEHASHMAP_TEST_SINGLE_THREAD
        paused();
#endif
        retirementOrder();
#ifndef LOCKFREEHASHMAP_TEST_SINGLE_THREAD
        contention();
#endif
        check(LockFreeHashMapTestThread::failures() == 0, "thread infrastructure errors");
    }
    catch (...)
    {
        check(false, "unexpected test exception");
    }
    std::cout << "MODULE_RESULT checks=" << s_checks << " failed=" << s_failed << std::endl;
    return s_failed == 0 ? 0 : 1;
}

int main()
{
    return LockFreeHashMapEpochTests::run();
}