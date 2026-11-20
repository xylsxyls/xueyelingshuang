#include "LockFreeHashMapEpochTests.h"
#include "LockFreeHashMapTestThread.h"
#include "LockFreeHashMapTestState.h"
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
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
    bool initialized = domain.initialize(2);
    check(initialized, "initialization succeeds");
    if (!initialized)
    {
        return;
    }
    check(!domain.initialize(2), "one-time initialization");
    {
        // 辅助维护借用另一条记录，但不把下一次业务操作的提示迁移过去。
        LockFreeHashMapEpoch::remember(0);
        LockFreeHashMapEpochGuard outer(&domain);
        check(outer.record() == domain.record(0), "outer operation uses its remembered record");
        {
            LockFreeHashMapEpochGuard auxiliary(&domain, true, false);
            LockFreeHashMapEpochGuard full(&domain, true, false);
            check(auxiliary.record() == domain.record(1) && full.record() == nullptr &&
                LockFreeHashMapEpoch::hint() == 0 && LockFreeHashMapEpochGuard::heldByCurrentThread(&domain),
                "auxiliary admission preserves hint while still enforcing protection and reentry capacity");
        }
        check(LockFreeHashMapEpoch::hint() == 0 && LockFreeHashMapEpochGuard::heldByCurrentThread(&domain),
            "auxiliary exit restores the outer callback chain without replacing its scan hint");
    }
    {
        // 非二次幂域从末槽开始，覆盖直接接纳失败后的回绕及两种保护的共同容量边界。
        LockFreeHashMapEpoch odd;
        bool ready = odd.initialize(3);
        check(ready, "non-power-of-two domain initialization");
        if (!ready)
        {
            return;
        }
        LockFreeHashMapEpoch inactive;
        check(odd.owns(odd.record(0)) && odd.owns(odd.record(2)) && !odd.owns(domain.record(0)) &&
            !odd.owns(nullptr) && !inactive.owns(odd.record(0)) && !LockFreeHashMapEpochGuard::heldByCurrentThread(nullptr),
            "record ownership distinguishes first and last slots, other domains, and empty domains");
        size_t hint = (std::numeric_limits<size_t>::max)() - 1;
        LockFreeHashMapEpoch::remember(hint);
        {
            LockFreeHashMapReadGuard first(&odd);
            LockFreeHashMapEpochGuard second(&odd);
            LockFreeHashMapReadGuard third(&odd);
            LockFreeHashMapEpochGuard rejected(&odd);
            check(first.record() == odd.record(hint % 3) && second.record() != nullptr && third.record() != nullptr &&
                first.record() != second.record() && first.record() != third.record() && second.record() != third.record() &&
                rejected.record() == nullptr, "mixed guards scan all three records exactly once without aliasing");
        }
        check(odd.minimum() == 0 && !LockFreeHashMapEpochGuard::heldByCurrentThread(&odd),
            "non-power-of-two domain releases all records and callback links");
    }
    check(domain.record(2) == nullptr && domain.minimum() == 0, "bounds and empty domain");
    check((reinterpret_cast<uintptr_t>(domain.record(0)) & 63) == 0 &&
        (reinterpret_cast<uintptr_t>(domain.record(1)) & 63) == 0, "cache-line alignment");
    LockFreeHashMapEpoch other;
    check(other.initialize(1) && !LockFreeHashMapEpochGuard::heldByCurrentThread(&domain), "inactive domain is not held");
    {
        // 一条记录的域没有第二次接纳余量，转交必须全程保留原epoch和独占责任。
        LockFreeHashMapReadGuard leaf(&other);
        LockFreeHashMapEpochRecord* admitted = leaf.record();
        uint64_t epoch = other.minimum();
        {
            LockFreeHashMapEpochGuard callback(leaf);
            leaf.release();
            LockFreeHashMapReadGuard rejected(&other);
            check(admitted != nullptr && leaf.record() == nullptr && callback.record() == admitted &&
                epoch != 0 && other.minimum() == epoch && rejected.record() == nullptr &&
                LockFreeHashMapEpochGuard::heldByCurrentThread(&other),
                "read-to-callback transfer retains the same protected record without a second admission");
        }
        check(other.minimum() == 0 && !LockFreeHashMapEpochGuard::heldByCurrentThread(&other),
            "transferred protection is released once by the callback owner");
        LockFreeHashMapEpochGuard empty(leaf);
        check(empty.record() == nullptr && !LockFreeHashMapEpochGuard::heldByCurrentThread(&other),
            "transferring an empty source cannot relink or acquire a record");
    }
    LockFreeHashMapEpoch::remember(1);
    {
        LockFreeHashMapEpochGuard first(&domain);
        check(first.record() == domain.record(1) && domain.minimum() == 1, "remembered hint still admits through epoch CAS");
        check(LockFreeHashMapEpochGuard::heldByCurrentThread(&domain), "outer guard is linked");
        {
            LockFreeHashMapReadGuard empty(nullptr);
            empty.release();
            LockFreeHashMapEpochGuard emptyOrdinary(nullptr);
            emptyOrdinary.release();
            check(empty.record() == nullptr && emptyOrdinary.record() == nullptr,
                "null guards do not acquire or release another record");
            std::unique_ptr<LockFreeHashMapReadGuard> leaf(new LockFreeHashMapReadGuard(&other));
            check(leaf->record() != nullptr && other.minimum() != 0 &&
                !LockFreeHashMapEpochGuard::heldByCurrentThread(&other),
                "compact read guard publishes full protection without a callback chain");
            LockFreeHashMapReadGuard rejected(&other);
            check(rejected.record() == nullptr, "compact read guard respects domain capacity");
            leaf->release();
            LockFreeHashMapEpochGuard successor(&other);
            leaf->release();
            leaf.reset();
            check(successor.record() != nullptr && other.minimum() != 0,
                "compact read release is idempotent after ordinary admission reuses the record");
        }
        check(other.minimum() == 0 && LockFreeHashMapEpochGuard::heldByCurrentThread(&domain),
            "compact read and ordinary guard nesting preserve the outer callback chain");
        {
            LockFreeHashMapEpochGuard leaf(&other, false);
            check(leaf.record() != nullptr && other.minimum() != 0 &&
                LockFreeHashMapEpochGuard::heldByCurrentThread(&domain) &&
                !LockFreeHashMapEpochGuard::heldByCurrentThread(&other),
                "callback-free leaf retains epoch protection without altering the outer chain");
        }
        check(other.minimum() == 0 && LockFreeHashMapEpochGuard::heldByCurrentThread(&domain),
            "callback-free leaf releases its record and preserves the outer chain");
        {
            // 测试独占保护对象，显式控制其析构时机；先退出者不能清除后来接纳者的同一条记录。
            std::unique_ptr<LockFreeHashMapEpochGuard> early(new LockFreeHashMapEpochGuard(&other));
            check(early->record() != nullptr && LockFreeHashMapEpochGuard::heldByCurrentThread(&other),
                "early release fixture admitted and linked");
            early->release();
            early->track();
            check(early->record() == nullptr && other.minimum() == 0 &&
                !LockFreeHashMapEpochGuard::heldByCurrentThread(&other),
                "released guard cannot relink without ownership");
            LockFreeHashMapEpochGuard replacement(&other);
            early->release();
            early.reset();
            check(replacement.record() != nullptr && other.minimum() != 0 &&
                LockFreeHashMapEpochGuard::heldByCurrentThread(&other),
                "repeated release and destruction do not clear a successor admission");
        }
        check(other.minimum() == 0 && LockFreeHashMapEpochGuard::heldByCurrentThread(&domain),
            "early and ordinary destruction restore the original outer chain");
        try
        {
            LockFreeHashMapEpoch absent;
            LockFreeHashMapEpochGuard delayed(&other, false);
            check(delayed.record() != nullptr && !LockFreeHashMapEpochGuard::heldByCurrentThread(&other),
                "deferred tracking initially protects without linking");
            delayed.track();
            delayed.track();
            check(LockFreeHashMapEpochGuard::heldByCurrentThread(&other) &&
                LockFreeHashMapEpochGuard::heldByCurrentThread(&domain) &&
                !LockFreeHashMapEpochGuard::heldByCurrentThread(&absent),
                "tracking twice retains both domains without linking a cycle");
            throw 8;
        }
        catch (int error)
        {
            check(error == 8 && !LockFreeHashMapEpochGuard::heldByCurrentThread(&other) &&
                LockFreeHashMapEpochGuard::heldByCurrentThread(&domain),
                "deferred tracking unwinds to the original outer guard");
        }
        try
        {
            LockFreeHashMapEpochGuard different(&other);
            check(different.record() != nullptr && LockFreeHashMapEpochGuard::heldByCurrentThread(&domain) &&
                LockFreeHashMapEpochGuard::heldByCurrentThread(&other), "cross-domain nesting preserves both guards");
            throw 7;
        }
        catch (int error)
        {
            check(error == 7 && !LockFreeHashMapEpochGuard::heldByCurrentThread(&other) &&
                LockFreeHashMapEpochGuard::heldByCurrentThread(&domain), "unwinding restores the outer guard");
        }
        domain.advance();
        {
            LockFreeHashMapEpochGuard nested(&domain);
            LockFreeHashMapEpochGuard rejected(&domain);
            check(nested.record() != nullptr && nested.record() != first.record(), "nested reader gets distinct record");
            check(rejected.record() == nullptr && domain.minimum() == 1, "full domain refuses without waiting");
            check(LockFreeHashMapEpochGuard::heldByCurrentThread(&domain), "rejected admission does not unlink accepted guards");
        }
        LockFreeHashMapEpochGuard reused(&domain);
        check(reused.record() != nullptr && reused.record()->m_epoch.load() == 2, "released record reused at fresh epoch");
    }
    check(domain.minimum() == 0, "all records released");
    check(!LockFreeHashMapEpochGuard::heldByCurrentThread(&domain) &&
        !LockFreeHashMapEpochGuard::heldByCurrentThread(&other), "no guard remains after all nested calls return");
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
        LockFreeHashMapReadGuard guard(&domain);
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
    std::atomic<bool> owners[16];
    LockFreeHashMapEpochRecord* records[16];
    for (size_t slot = 0; slot < 16; ++slot)
    {
        owners[slot].store(false);
        records[slot] = domain.record(slot);
    }
    std::vector<LockFreeHashMapTestThread> threads;
    threads.reserve(8);
    for (size_t i = 0; i < 8; ++i)
    {
        // 每个线程在已接纳记录中写入独占提示，再核对未被其他线程覆盖
        threads.push_back(LockFreeHashMapTestThread([&, i]()
        {
            for (size_t round = 0; round < 100000; ++round)
            {
                // 两种保护共同竞争同一个域；空域实例不占记录，核对各自协议没有分离。
                LockFreeHashMapReadGuard leaf((i & 1) == 0 ? &domain : nullptr);
                LockFreeHashMapEpochGuard guard((i & 1) != 0 ? &domain : nullptr);
                LockFreeHashMapEpochRecord* record = (i & 1) == 0 ? leaf.record() : guard.record();
                if (record == nullptr)
                {
                    // 有界扫描允许竞争期间未接纳；拒绝不算已经取得保护，也不访问记录
                    refused.fetch_add(1);
                    continue;
                }
                completed.fetch_add(1, std::memory_order_relaxed);
                size_t slot = 0;
                while (slot < 16 && records[slot] != record)
                {
                    ++slot;
                }
                bool empty = false;
                // 独立原子所有权先判重，错误实现重复接纳时也不让测试自身产生非原子数据竞争。
                if (slot == 16 || !owners[slot].compare_exchange_strong(empty, true))
                {
                    errors.fetch_add(1);
                    continue;
                }
                record->m_blockHint = i;
                record->m_wordHint = round;
                if (record->m_epoch.load() == 0 || record->m_blockHint != i || record->m_wordHint != round)
                {
                    errors.fetch_add(1);
                }
                owners[slot].store(false, std::memory_order_release);
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    check(errors.load() == 0 && completed.load() > 0 && completed.load() + refused.load() == 800000 &&
        domain.minimum() == 0, "800000 concurrent admission attempts");
    bool detached = true;
    for (size_t slot = 0; slot < 16; ++slot)
    {
        detached = detached && !owners[slot].load();
    }
    check(detached, "all joined borrowers clear independent test ownership");
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
    std::cout << "MODULE_RESULT id=epoch checks=" << s_checks << " failed=" << s_failed << std::endl;
    return s_failed == 0 ? 0 : 1;
}

int main()
{
    return LockFreeHashMapEpochTests::run();
}