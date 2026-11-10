#include "LockFreeHashMapPoolTests.h"
#include "LockFreeHashMapTestThread.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<int64_t> LockFreeHashMapPoolTestValue::s_live(0);
std::atomic<bool> LockFreeHashMapPoolTestValue::s_throw(false);
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<int64_t> LockFreeHashMapPoolTestValue::s_pause(-1);
std::atomic<bool> LockFreeHashMapPoolTestValue::s_entered(false);
std::atomic<bool> LockFreeHashMapPoolTestValue::s_resume(false);
bool LockFreeHashMapPoolTestAllocation::s_tracking = false;
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<int64_t> LockFreeHashMapPoolTestAllocation::s_live(0);
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<int64_t> LockFreeHashMapPoolTestAllocation::s_fail(-1);
size_t LockFreeHashMapPoolTests::s_checks = 0;
size_t LockFreeHashMapPoolTests::s_failed = 0;

/** 统计实际成功的全局分配，故障只发生在测试显式开启的范围
@param [in] size 申请字节数
@return 已分配地址；故障以 bad_alloc 传到被测捕获边界
*/
void* operator new(size_t size)
{
    int64_t remaining = LockFreeHashMapPoolTestAllocation::s_tracking ? LockFreeHashMapPoolTestAllocation::s_fail.load() : -1;
    if (remaining >= 0 && LockFreeHashMapPoolTestAllocation::s_fail.fetch_sub(1) == 0)
    {
        throw std::bad_alloc();
    }
    if (size > (std::numeric_limits<size_t>::max)() - 32)
    {
        throw std::bad_alloc();
    }
    unsigned char* result = static_cast<unsigned char*>(std::malloc(size + 32));
    if (result == nullptr)
    {
        throw std::bad_alloc();
    }
    result[0] = LockFreeHashMapPoolTestAllocation::s_tracking ? 1 : 0;
    if (result[0] != 0)
    {
        LockFreeHashMapPoolTestAllocation::s_live.fetch_add(1);
    }
    return result + 32;
}

/** 数组申请使用相同计账
@param [in] size 字节数
@return 数组存储地址
*/
void* operator new[](size_t size)
{
    return ::operator new(size);
}

/** 非抛异常申请捕获故障，不把空指针伪装成成功
@param [in] size 字节数
@param [in] tag 非抛异常重载标签
@return 地址或空指针
*/
void* operator new(size_t size, const std::nothrow_t& tag) throw()
{
    (void)tag;
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return nullptr;
    }
}

/** 非抛异常数组申请使用相同计账
@param [in] size 字节数
@param [in] tag 非抛异常重载标签
@return 地址或空指针
*/
void* operator new[](size_t size, const std::nothrow_t& tag) throw()
{
    return ::operator new(size, tag);
}

/** 对已成功分配的地址归还计账
@param [in] pointer 待释放地址，可为空
*/
// 计账头属于底层 malloc 块；保留分配器边界，避免将调用方 new 的载荷大小误当成整个底层块大小
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
void operator delete(void* pointer) throw()
{
    if (pointer != nullptr)
    {
        unsigned char* allocation = static_cast<unsigned char*>(pointer) - 32;
        if (allocation[0] != 0)
        {
            LockFreeHashMapPoolTestAllocation::s_live.fetch_sub(1);
        }
        std::free(allocation);
    }
}

/** 数组释放与数组申请配对
@param [in] pointer 待释放地址
*/
void operator delete[](void* pointer) throw()
{
    ::operator delete(pointer);
}

/** 非抛异常构造失败的释放重载
@param [in] pointer 已申请地址
@param [in] tag 非抛异常重载标签
*/
void operator delete(void* pointer, const std::nothrow_t& tag) throw()
{
    (void)tag;
    ::operator delete(pointer);
}

/** 非抛异常数组构造失败的释放重载
@param [in] pointer 已申请地址
@param [in] tag 非抛异常重载标签
*/
void operator delete[](void* pointer, const std::nothrow_t& tag) throw()
{
    (void)tag;
    ::operator delete(pointer);
}

LockFreeHashMapPoolTestValue::LockFreeHashMapPoolTestValue(uint64_t value) :
m_value(value)
{
    if (s_throw.load())
    {
        throw 19;
    }
    s_live.fetch_add(1);
}

LockFreeHashMapPoolTestValue::~LockFreeHashMapPoolTestValue()
{
    if (s_pause.load() >= 0 && static_cast<uint64_t>(s_pause.load()) == m_value)
    {
        s_entered.store(true);
        while (!s_resume.load())
        {
            std::this_thread::yield();
        }
    }
    s_live.fetch_sub(1);
}

void LockFreeHashMapPoolTests::check(bool condition, const char* name)
{
    ++s_checks;
    if (!condition)
    {
        ++s_failed;
        std::cerr << "POOL_FAILURE " << name << std::endl;
        throw 91;
    }
}

void LockFreeHashMapPoolTests::warmRuntime()
{
    int64_t previous = LockFreeHashMapPoolTestAllocation::s_live.load();
    size_t stable = 0;
    for (size_t round = 0; round < 32; ++round)
    {
        // 单独触发运行库异常设施，不触及任何被测池对象
        try
        {
            throw std::bad_alloc();
        }
        catch (const std::bad_alloc&)
        {

        }
#ifndef LOCKFREEHASHMAP_TEST_SINGLE_THREAD
        {
            std::vector<LockFreeHashMapTestThread> workers;
            workers.reserve(8);
            for (size_t i = 0; i < 8; ++i)
            {
                // 工作线程仅调用标准库时间、线程标识和休眠，作为独立资源对照
                workers.push_back(LockFreeHashMapTestThread([]()
                {
                    std::hash<std::thread::id>()(std::this_thread::get_id());
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }));
            }
            for (size_t i = 0; i < workers.size(); ++i)
            {
                workers[i].join();
            }
        }
#endif
        int64_t current = LockFreeHashMapPoolTestAllocation::s_live.load();
        std::cout << "POOL_RUNTIME_CONTROL round=" << round << " live=" << current << std::endl;
        stable = current == previous ? stable + 1 : 0;
        previous = current;
        if (stable == 8)
        {
            return;
        }
    }
    check(false, "runtime-only allocation control stabilizes");
}

void LockFreeHashMapPoolTests::boundaries()
{
    typedef LockFreeHashMapPoolTestValue Value;
    typedef LockFreeHashMapPoolSlot<Value> Slot;
    const size_t capacities[] = { 1, 2, 63, 64, 65, 127, 128, 129, 65535, 65536 };
    for (size_t test = 0; test < sizeof(capacities) / sizeof(capacities[0]); ++test)
    {
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = capacities[test];
        config.maximumCapacity = static_cast<int64_t>(config.initialCapacity);
        LockFreeHashMapPool<Value> pool;
        check(pool.initialize(config) == LockFreeHashMapOk, "boundary initialization");
        std::vector<Slot*> slots;
        slots.reserve(config.initialCapacity);
        {
            LockFreeHashMapEpochGuard guard(pool.epoch());
            LockFreeHashMapStatus status;
            for (size_t i = 0; i < config.initialCapacity; ++i)
            {
                Slot* slot = pool.acquire(*guard.record(), status);
                if (slot == nullptr || status != LockFreeHashMapOk)
                {
                    check(false, "every configured slot is usable");
                    break;
                }
                new (slot->payload()) Value(i);
                slot->m_kind.store(1);
                slots.push_back(slot);
            }
            check(pool.acquire(*guard.record(), status) == nullptr && status == LockFreeHashMapCapacityLimit, "full pool refuses without extra allocation");
            for (size_t i = 0; i < slots.size(); ++i)
            {
                check(slots[i]->payload()->m_value == i, "simultaneous slots are distinct");
                pool.retire(slots[i]);
            }
            check(pool.reclaim() == 0 && Value::s_live.load() == static_cast<int64_t>(slots.size()), "own reader prevents premature reuse");
        }
        check(pool.reclaim() == slots.size() && Value::s_live.load() == 0, "all retired payloads reclaimed after exit");
        LockFreeHashMapPoolStats stats = pool.stats();
        check(stats.capacity == config.initialCapacity && stats.reusableFree == stats.capacity && stats.occupied == 0 &&
            stats.retired == 0 && stats.peakBudget == config.initialCapacity, "exact quiescent bitmap accounting");
        pool.freeBlocks();
        check(pool.stats().budget == 0 && pool.stats().blockCount == 0, "explicit release bypasses automatic floor");
        {
            LockFreeHashMapEpochGuard guard(pool.epoch());
            LockFreeHashMapStatus status;
            Slot* slot = pool.acquire(*guard.record(), status);
            check(slot != nullptr && status == LockFreeHashMapOk, "pool reusable after explicit release");
            pool.release(slot);
        }
    }
    {
        // 外部静止时注入不可自然到达的块借用计数边界；位图空闲不等于借用预算可用。
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 1;
        config.maximumCapacity = 1;
        LockFreeHashMapPool<Value> pool;
        LockFreeHashMapStatus initialized = pool.initialize(config);
        check(initialized == LockFreeHashMapOk, "lease exhaustion fixture initialization");
        if (initialized != LockFreeHashMapOk)
        {
            return;
        }
        LockFreeHashMapEpochGuard guard(pool.epoch());
        check(guard.record() != nullptr, "lease exhaustion fixture protection");
        if (guard.record() == nullptr)
        {
            return;
        }
        LockFreeHashMapStatus status;
        Slot* slot = pool.acquire(*guard.record(), status);
        check(slot != nullptr, "lease exhaustion fixture slot");
        if (slot == nullptr)
        {
            return;
        }
        LockFreeHashMapPoolBlock<Value>* block = slot->m_block;
        pool.release(slot);
        uint64_t saved = block->m_leases.load();
        const uint64_t exhausted[] = { UINT64_C(0xfffffffc), UINT64_C(0xffffffff) };
        for (size_t i = 0; i < sizeof(exhausted) / sizeof(exhausted[0]); ++i)
        {
            block->m_leases.store(saved | exhausted[i]);
            Slot* rejected = nullptr;
            try
            {
                rejected = pool.acquire(*guard.record(), status);
            }
            catch (...)
            {
                block->m_leases.store(saved);
                throw;
            }
            block->m_leases.store(saved);
            check(rejected == nullptr && status == LockFreeHashMapCapacityLimit, "lease budget exhaustion at hard capacity reports capacity limit without spinning");
            if (rejected != nullptr)
            {
                // 错误路径由外层池析构收拢空槽，不继续使用已被错误改动的位图。
                return;
            }
        }
        slot = pool.acquire(*guard.record(), status);
        check(slot != nullptr && status == LockFreeHashMapOk, "restored lease budget permits immediate reuse");
        pool.release(slot);
    }
    for (size_t spare = 0; spare < 2; ++spare)
    {
        // 同时持有两份操作保护，确定第一份缓存仍在使用；有余量时第二份必须能独立扩容。
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 4;
        config.maximumCapacity = spare == 0 ? 4 : 8;
        // context 是增长时的观测；固定申请四槽，便于核对预留上限及另一个公共块。
        config.growPolicy = [](const LockFreeHashMapGrowContext& context)
        {
            (void)context;
            return size_t(4);
        };
        LockFreeHashMapPool<Value> pool;
        check(pool.initialize(config) == LockFreeHashMapOk, "active cache fixture initialization");
        {
            LockFreeHashMapEpochGuard owner(pool.epoch());
            LockFreeHashMapEpochGuard competitor(pool.epoch());
            if (owner.record() == nullptr || competitor.record() == nullptr)
            {
                check(false, "active cache fixture protection");
                return;
            }
            LockFreeHashMapStatus status;
            Slot* first = pool.acquire(*owner.record(), status);
            if (first == nullptr || status != LockFreeHashMapOk)
            {
                check(false, "active cache fixture first slot");
                return;
            }
            check(owner.record()->m_cachedCount.load() == 3, "active owner retains three cached free slots");
            Slot* second = pool.acquire(*competitor.record(), status);
            if (spare == 0)
            {
                check(second == nullptr && status == LockFreeHashMapCapacityLimit,
                    "active cached free slots at hard capacity report capacity limit");
            }
            else
            {
                check(second != nullptr && status == LockFreeHashMapOk && second->m_block != first->m_block,
                    "active cached free slots do not prevent independent growth below capacity");
            }
            check(owner.record()->m_cachedCount.load() == 3, "competing allocation does not steal an active cache");
            pool.release(second);
            pool.release(first);
        }
        {
            // 原持有者退出后，缓存可以再次借用，不把一次容量不足变成永久拒绝。
            LockFreeHashMapEpochGuard guard(pool.epoch());
            if (guard.record() == nullptr)
            {
                check(false, "active cache recovery protection");
                return;
            }
            LockFreeHashMapStatus status;
            Slot* recovered = pool.acquire(*guard.record(), status);
            check(recovered != nullptr && status == LockFreeHashMapOk, "cached capacity becomes reusable after owner exits");
            pool.release(recovered);
        }
        LockFreeHashMapPoolStats stats = pool.stats();
        check(stats.budget == static_cast<size_t>(config.maximumCapacity) && stats.occupied == 0 && stats.retired == 0,
            "capacity status fixtures return all occupied slots without exceeding budget");
        pool.freeBlocks();
        check(pool.stats().budget == 0, "capacity status fixture releases all blocks");
    }
    LockFreeHashMapPoolConfig invalid;
    invalid.maximumCapacity = -2;
    LockFreeHashMapPool<Value> bad;
    check(bad.initialize(invalid) == LockFreeHashMapInvalidArgument, "negative configuration rejected");
    invalid.maximumCapacity = 1;
    check(bad.initialize(invalid) == LockFreeHashMapInvalidArgument, "initial exceeds maximum rejected");
    {
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 4096;
        config.maximumCapacity = 4096;
        LockFreeHashMapPool<Value> pool;
        check(pool.initialize(config) == LockFreeHashMapOk, "bounded reclaim pool initialization");
        {
            LockFreeHashMapEpochGuard guard(pool.epoch());
            LockFreeHashMapStatus status;
            for (size_t i = 0; i < 4096; ++i)
            {
                Slot* slot = pool.acquire(*guard.record(), status);
                if (slot == nullptr)
                {
                    check(false, "bounded reclaim initial slots");
                    break;
                }
                new (slot->payload()) Value(i);
                slot->m_kind.store(1);
                pool.retire(slot);
            }
        }
        size_t reclaimed = 0;
        for (size_t i = 0; i < 64; ++i)
        {
            reclaimed += pool.reclaim(1);
        }
        check(reclaimed == 4096 && pool.stats().occupied == 0, "bounded maintenance eventually visits every word of a large block");
    }
}

void LockFreeHashMapPoolTests::exceptions()
{
    typedef LockFreeHashMapPoolTestValue Value;
    // 初始化五个申请点：保护记录、目录、块对象、槽数组、位图数组
    for (int64_t fail = 0; fail < 5; ++fail)
    {
        int64_t before = LockFreeHashMapPoolTestAllocation::s_live.load();
        {
            LockFreeHashMapPool<Value> pool;
            LockFreeHashMapPoolConfig config;
            LockFreeHashMapPoolTestAllocation::s_fail.store(fail);
            LockFreeHashMapStatus status = pool.initialize(config);
            LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
            check(status == LockFreeHashMapNoMemory, "each initialization allocation failure is reported");
        }
        check(LockFreeHashMapPoolTestAllocation::s_live.load() == before, "failed initialization has zero allocation delta");
    }
    for (int64_t fail = 0; fail < 3; ++fail)
    {
        LockFreeHashMapPool<Value> pool;
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 0;
        check(pool.initialize(config) == LockFreeHashMapOk, "empty pool initialization");
        int64_t before = LockFreeHashMapPoolTestAllocation::s_live.load();
        LockFreeHashMapEpochGuard guard(pool.epoch());
        LockFreeHashMapStatus status;
        LockFreeHashMapPoolTestAllocation::s_fail.store(fail);
        LockFreeHashMapPoolSlot<Value>* slot = pool.acquire(*guard.record(), status);
        LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
        check(slot == nullptr && status == LockFreeHashMapNoMemory && pool.stats().budget == 0, "growth failure returns every reservation");
        check(LockFreeHashMapPoolTestAllocation::s_live.load() == before, "failed growth has zero allocation delta");
        slot = pool.acquire(*guard.record(), status);
        check(slot != nullptr, "growth succeeds on retry");
        if (slot != nullptr)
        {
            Value::s_throw.store(true);
            bool caught = false;
            try
            {
                new (slot->payload()) Value(7);
                slot->m_kind.store(1);
            }
            catch (...)
            {
                caught = true;
            }
            Value::s_throw.store(false);
            pool.release(slot);
            check(caught && Value::s_live.load() == 0 && pool.stats().occupied == 0, "construction failure returns raw slot once");
        }
    }
}

void LockFreeHashMapPoolTests::policies()
{
    typedef LockFreeHashMapPoolTestValue Value;
    for (size_t threshold = 0; threshold <= 1; ++threshold)
    {
        size_t calls = 0;
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 1;
        config.maximumCapacity = 2;
        config.growFreeThreshold = threshold;
        // 只在当前串行夹具中调用，核对共同契约的容量上下文
        config.growPolicy = [&](const LockFreeHashMapGrowContext& context) -> size_t
        {
            ++calls;
            check(context.initialCapacity == 1 && context.currentCapacity == 1 && context.successfulGrowCount == 0, "growth context before first publication");
            return 100;
        };
        LockFreeHashMapPool<Value> pool;
        check(pool.initialize(config) == LockFreeHashMapOk, "policy pool initialization");
        LockFreeHashMapEpochGuard guard(pool.epoch());
        LockFreeHashMapStatus status;
        LockFreeHashMapPoolSlot<Value>* first = pool.acquire(*guard.record(), status);
        check(first != nullptr && calls == threshold && pool.stats().budget == 1 + threshold, "zero and positive threshold trigger differ");
        // 第二次申请时正阈值会再次评估；改用默认固定上下文检查之外的独立夹具
        if (threshold == 0)
        {
            LockFreeHashMapPoolSlot<Value>* second = pool.acquire(*guard.record(), status);
            check(second != nullptr && calls == 1 && pool.stats().capacity == 2 && pool.stats().successfulGrowCount == 1, "growth is clipped to hard maximum");
            pool.release(second);
        }
        pool.release(first);
    }
    for (int mode = 0; mode < 2; ++mode)
    {
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 0;
        // 模式零拒绝增长，模式一抛可捕获异常，均不得偷偷改用默认增长
        config.growPolicy = [mode](const LockFreeHashMapGrowContext& context) -> size_t
        {
            (void)context;
            if (mode != 0)
            {
                throw 23;
            }
            return 0;
        };
        LockFreeHashMapPool<Value> pool;
        check(pool.initialize(config) == LockFreeHashMapOk, "refusing policy initialization");
        LockFreeHashMapEpochGuard guard(pool.epoch());
        LockFreeHashMapStatus status;
        check(pool.acquire(*guard.record(), status) == nullptr && status == (mode == 0 ? LockFreeHashMapGrowthDeclined : LockFreeHashMapUserException) &&
            pool.stats().budget == 0, "policy refusal and exception remain distinct");
    }
}

void LockFreeHashMapPoolTests::paused()
{
    typedef LockFreeHashMapPoolTestValue Value;
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 1;
    config.maximumCapacity = 1;
    LockFreeHashMapPool<Value> pool;
    check(pool.initialize(config) == LockFreeHashMapOk, "paused pool initialization");
    std::atomic<bool> ready(false), resume(false), correct(false);
    LockFreeHashMapPoolSlot<Value>* slot = nullptr;
    {
        LockFreeHashMapEpochGuard guard(pool.epoch());
        LockFreeHashMapStatus status;
        slot = pool.acquire(*guard.record(), status);
        if (slot == nullptr)
        {
            check(false, "paused initial allocation");
            return;
        }
        new (slot->payload()) Value(73);
        slot->m_kind.store(1);
    }
    // 子线程先持保护再读槽；主线程只在 ready 后移除来源并退役
    LockFreeHashMapTestThread reader([&]()
    {
        LockFreeHashMapEpochGuard guard(pool.epoch());
        ready.store(true);
        while (!resume.load())
        {
            std::this_thread::yield();
        }
        correct.store(guard.record() != nullptr && slot->payload()->m_value == 73);
    });
    try
    {
        while (!ready.load())
        {
            std::this_thread::yield();
        }
        {
            LockFreeHashMapEpochGuard guard(pool.epoch());
            pool.retire(slot);
            check(pool.reclaim() == 0 && Value::s_live.load() == 1, "paused reader pins old payload");
            LockFreeHashMapStatus status;
            check(pool.acquire(*guard.record(), status) == nullptr && status == LockFreeHashMapCapacityLimit, "bounded pool refuses while paused reader remains");
        }
        resume.store(true);
        reader.join();
    }
    catch (...)
    {
        resume.store(true);
        reader.join();
        throw;
    }
    check(correct.load() && pool.reclaim() == 1 && Value::s_live.load() == 0, "reader exits before payload destruction");
}

void LockFreeHashMapPoolTests::contention()
{
    typedef LockFreeHashMapPoolTestValue Value;
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 0;
    config.maximumCapacity = 129;
    config.protectionRecordCount = 32;
    LockFreeHashMapPool<Value> pool;
    check(pool.initialize(config) == LockFreeHashMapOk, "concurrent pool initialization");
    LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<uint64_t> completed(0), refused(0), errors(0);
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(8);
    for (size_t thread = 0; thread < 8; ++thread)
    {
        // 每次申请保留独立值直至退役；回收与其他线程的申请、退役交叠
        workers.push_back(LockFreeHashMapTestThread([&, thread]()
        {
            for (size_t round = 0; round < 20000; ++round)
            {
                {
                    LockFreeHashMapEpochGuard guard(pool.epoch());
                    if (guard.record() == nullptr)
                    {
                        refused.fetch_add(1);
                        continue;
                    }
                    LockFreeHashMapStatus status;
                    LockFreeHashMapPoolSlot<Value>* slot = pool.acquire(*guard.record(), status);
                    if (slot == nullptr)
                    {
                        refused.fetch_add(1);
                    }
                    else
                    {
                        uint64_t marker = (uint64_t(thread) << 32) | round;
                        new (slot->payload()) Value(marker);
                        slot->m_kind.store(2);
                        if (slot->payload()->m_value != marker)
                        {
                            errors.fetch_add(1);
                        }
                        pool.retire(slot);
                        completed.fetch_add(1);
                    }
                }
                if ((round & 31) == 0)
                {
                    pool.reclaim();
                }
            }
        }));
    }
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    pool.reclaim();
    LockFreeHashMapPoolStats stats = pool.stats();
    check(errors.load() == 0 && completed.load() > 0 && completed.load() + refused.load() == 160000, "160000 attempts preserve exclusive slot ownership");
    check(stats.budget <= 129 && stats.peakBudget <= 129 && stats.reservedCapacity == 0 && stats.occupied == 0 &&
        stats.retired == 0 && Value::s_live.load() == 0, "concurrent growth never exceeds hard budget and all values reclaimed");
}

void LockFreeHashMapPoolTests::shrinking()
{
    typedef LockFreeHashMapPoolTestValue Value;
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = 512;
    // 每次只按六十四槽增长，以便验证多个独立批次的释放
    config.growPolicy = [](const LockFreeHashMapGrowContext& context) -> size_t
    {
        (void)context;
        return 64;
    };
    // 连续空闲至少两毫秒，允许多次维护释放完整批次
    config.shrinkPolicy = [](const LockFreeHashMapShrinkContext& context) -> LockFreeHashMapShrinkDecision
    {
        return LockFreeHashMapShrinkDecision(context.currentCapacity, 2);
    };
    LockFreeHashMapPool<Value> pool;
    check(pool.initialize(config) == LockFreeHashMapOk, "shrink pool initialization");
    std::vector<LockFreeHashMapPoolSlot<Value>*> slots;
    slots.reserve(256);
    {
        LockFreeHashMapEpochGuard guard(pool.epoch());
        LockFreeHashMapStatus status;
        for (size_t i = 0; i < 256; ++i)
        {
            slots.push_back(pool.acquire(*guard.record(), status));
            check(slots.back() != nullptr, "four blocks published");
        }
        for (size_t i = 0; i < slots.size(); ++i)
        {
            pool.release(slots[i]);
        }
    }
    check(pool.stats().capacity == 256, "capacity before automatic shrink");
    pool.shrink();
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
    {
        LockFreeHashMapEpochGuard guard(pool.epoch());
        LockFreeHashMapStatus status;
        for (size_t i = 0; i < slots.size(); ++i)
        {
            slots[i] = pool.acquire(*guard.record(), status);
        }
        for (size_t i = 0; i < slots.size(); ++i)
        {
            pool.release(slots[i]);
        }
    }
    pool.shrink();
    check(pool.stats().capacity == 256 && pool.stats().closedCapacity == 0, "reuse resets continuous idle interval even when occupancy returns to zero");
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
    pool.shrink();
    pool.shrink();
    check(pool.stats().capacity == 64 && pool.stats().budget == 64 && pool.stats().successfulGrowCount == 3, "three idle blocks freed with floor and history preserved");
    std::atomic<bool> finish(false);
    LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<uint64_t> completed(0), errors(0);
    // 主线程是唯一 shrink 调用者；工作线程并发申请、归还和读取统计
    LockFreeHashMapTestThread worker([&]()
    {
        LockFreeHashMapPoolSlot<Value>* batch[129] = {};
        while (!finish.load())
        {
            LockFreeHashMapEpochGuard guard(pool.epoch());
            if (guard.record() == nullptr)
            {
                continue;
            }
            LockFreeHashMapStatus status;
            for (size_t i = 0; i < 129; ++i)
            {
                batch[i] = pool.acquire(*guard.record(), status);
                if (batch[i] != nullptr)
                {
                    new (batch[i]->payload()) Value(i + 91);
                    batch[i]->m_kind.store(1);
                }
            }
            for (size_t i = 0; i < 129; ++i)
            {
                if (batch[i] != nullptr)
                {
                    if (batch[i]->payload()->m_value != i + 91)
                    {
                        errors.fetch_add(1);
                    }
                    pool.release(batch[i]);
                    completed.fetch_add(1);
                }
            }
            if (pool.stats().budget > 512)
            {
                errors.fetch_add(1);
            }
        }
    });
    try
    {
        for (size_t i = 0; i < 1000; ++i)
        {
            pool.shrink();
        }
        finish.store(true);
        worker.join();
    }
    catch (...)
    {
        finish.store(true);
        worker.join();
        throw;
    }
    check(completed.load() != 0 && errors.load() == 0 && pool.stats().capacity >= 64, "allocation and stats progress during maintenance");
}

void LockFreeHashMapPoolTests::pausedDestructor()
{
    typedef LockFreeHashMapPoolTestValue Value;
    typedef LockFreeHashMapPoolSlot<Value> Slot;
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = 128;
    LockFreeHashMapPool<Value> pool;
    check(pool.initialize(config) == LockFreeHashMapOk, "paused collector pool initialized");
    Slot* retired[64] = {};
    Slot* active[64] = {};
    {
        LockFreeHashMapEpochGuard guard(pool.epoch());
        check(guard.record() != nullptr, "paused collector setup guard available");
        for (size_t i = 0; i < 64; ++i)
        {
            LockFreeHashMapStatus status;
            retired[i] = pool.acquire(*guard.record(), status);
            check(retired[i] != nullptr, "paused collector initial allocation");
            new (retired[i]->payload()) Value(i);
            retired[i]->m_kind.store(1);
            pool.retire(retired[i]);
        }
    }
    LockFreeHashMapTestThread collector;
    Value::s_entered.store(false);
    Value::s_resume.store(false);
    Value::s_pause.store(0);
    try
    {
        collector = LockFreeHashMapTestThread([&]()
        {
            pool.reclaim();
        });
        while (!Value::s_entered.load())
        {
            std::this_thread::yield();
        }
        {
            LockFreeHashMapEpochGuard guard(pool.epoch());
            check(guard.record() != nullptr, "other allocator enters while collector pauses");
            for (size_t i = 0; i < 64; ++i)
            {
                LockFreeHashMapStatus status;
                active[i] = pool.acquire(*guard.record(), status);
                check(active[i] != nullptr, "other allocator completes while collector pauses");
                for (size_t j = 0; j < 64; ++j)
                {
                    check(active[i] != retired[j], "claimed retirement is never reused before destruction");
                }
                new (active[i]->payload()) Value(i + 1000);
                active[i]->m_kind.store(1);
            }
            LockFreeHashMapStatus status;
            check(pool.acquire(*guard.record(), status) == nullptr && pool.stats().budget == 128,
                "paused collector consumes hard budget and full pool returns");
        }
        Value::s_resume.store(true);
        collector.join();
        Value::s_pause.store(-1);
        for (size_t i = 0; i < 64; ++i)
        {
            check(active[i]->payload()->m_value == i + 1000, "new block payload stays intact after collector resumes");
            pool.release(active[i]);
        }
        pool.reclaim();
        check(Value::s_live.load() == 0 && pool.stats().occupied == 0, "resumed collector releases every old value");
    }
    catch (...)
    {
        Value::s_resume.store(true);
        Value::s_pause.store(-1);
        collector.join();
        throw;
    }
}

void LockFreeHashMapPoolTests::overlappingReclaim()
{
    typedef LockFreeHashMapPoolTestValue Value;
    typedef LockFreeHashMapPoolSlot<Value> Slot;
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = 64;
    LockFreeHashMapPool<Value> pool;
    check(pool.initialize(config) == LockFreeHashMapOk, "overlapping reclaim pool initialized");
    Slot* first = nullptr;
    {
        LockFreeHashMapEpochGuard guard(pool.epoch());
        check(guard.record() != nullptr, "overlapping reclaim setup protected");
        for (size_t i = 0; i < 32; ++i)
        {
            LockFreeHashMapStatus status;
            Slot* slot = pool.acquire(*guard.record(), status);
            check(slot != nullptr, "old half word allocated");
            if (i == 0)
            {
                first = slot;
            }
            new (slot->payload()) Value(i);
            slot->m_kind.store(1);
            pool.retire(slot);
        }
    }
    LockFreeHashMapTestThread collector;
    Value::s_entered.store(false);
    Value::s_resume.store(false);
    Value::s_pause.store(0);
    try
    {
        // 无参数回收者独占旧半字后停在首个析构，不持有其他线程完成所需的唯一权限
        collector = LockFreeHashMapTestThread([&]()
        {
            pool.reclaim();
        });
        while (!Value::s_entered.load())
        {
            std::this_thread::yield();
        }
        {
            LockFreeHashMapEpochGuard guard(pool.epoch());
            check(guard.record() != nullptr, "new retirement publisher remains admitted");
            for (size_t i = 0; i < 32; ++i)
            {
                LockFreeHashMapStatus status;
                Slot* slot = pool.acquire(*guard.record(), status);
                check(slot != nullptr && slot->m_block == first->m_block && slot->m_index >= 32,
                    "new half word shares bitmap without reusing claimed old slots");
                new (slot->payload()) Value(i + 1000);
                slot->m_kind.store(1);
                pool.retire(slot);
            }
        }
        check(pool.reclaim() == 0 && Value::s_live.load() == 64 && pool.stats().retired == 64,
            "second collector returns every unsafe retirement bit while first collector pauses");
        Value::s_resume.store(true);
        collector.join();
        Value::s_pause.store(-1);
        check(Value::s_live.load() == 32 && pool.reclaim() == 32,
            "old word owner preserves all newly published retirement bits");
        check(Value::s_live.load() == 0 && pool.stats().occupied == 0 && pool.stats().retired == 0,
            "both word generations are reclaimed exactly once");
    }
    catch (...)
    {
        Value::s_resume.store(true);
        Value::s_pause.store(-1);
        collector.join();
        throw;
    }
}

int LockFreeHashMapPoolTests::run()
{
    try
    {
        // 先初始化标准输出，避免将流的首次内部申请算作测试泄漏
        std::cout << "POOL_BEGIN" << std::endl;
        LockFreeHashMapPoolTestAllocation::s_tracking = true;
        warmRuntime();
        int64_t before = LockFreeHashMapPoolTestAllocation::s_live.load();
        boundaries();
        std::cout << "POOL_STAGE boundaries delta=" << LockFreeHashMapPoolTestAllocation::s_live.load() - before << std::endl;
        exceptions();
        std::cout << "POOL_STAGE exceptions delta=" << LockFreeHashMapPoolTestAllocation::s_live.load() - before << std::endl;
        policies();
        std::cout << "POOL_STAGE policies delta=" << LockFreeHashMapPoolTestAllocation::s_live.load() - before << std::endl;
#ifndef LOCKFREEHASHMAP_TEST_SINGLE_THREAD
        paused();
        std::cout << "POOL_STAGE paused delta=" << LockFreeHashMapPoolTestAllocation::s_live.load() - before << std::endl;
        pausedDestructor();
        std::cout << "POOL_STAGE pausedDestructor delta=" << LockFreeHashMapPoolTestAllocation::s_live.load() - before << std::endl;
        overlappingReclaim();
        std::cout << "POOL_STAGE overlappingReclaim delta=" << LockFreeHashMapPoolTestAllocation::s_live.load() - before << std::endl;
        contention();
        std::cout << "POOL_STAGE contention delta=" << LockFreeHashMapPoolTestAllocation::s_live.load() - before << std::endl;
        shrinking();
        std::cout << "POOL_STAGE shrinking delta=" << LockFreeHashMapPoolTestAllocation::s_live.load() - before << std::endl;
#endif
        check(LockFreeHashMapPoolTestValue::s_live.load() == 0 && LockFreeHashMapTestThread::failures() == 0, "payload and thread resources released");
        check(LockFreeHashMapPoolTestAllocation::s_live.load() == before, "complete module allocation delta is zero");
        std::cout << "POOL_ALLOCATION_DELTA " << LockFreeHashMapPoolTestAllocation::s_live.load() - before << std::endl;
    }
    catch (...)
    {
        LockFreeHashMapPoolTestAllocation::s_fail.store(-1);
        LockFreeHashMapPoolTestValue::s_throw.store(false);
        ++s_failed;
        std::cerr << "POOL_FAILURE unexpected module exception" << std::endl;
    }
    std::cout << "MODULE_RESULT checks=" << s_checks << " failed=" << s_failed << std::endl;
    return s_failed == 0 ? 0 : 1;
}

#ifndef LOCKFREEHASHMAP_POOL_NO_MAIN
int main()
{
    return LockFreeHashMapPoolTests::run();
}
#endif