#include "LockFreeMapRetryTests.h"
#include "LockFreeMapStatusTest.h"
#include "LockFreeMapBenchmarkClock.h"
#include "LockFreeMapTest.h"
#include "LockFreeMapAPI.h"
#ifndef LOCK_FREE_MAP_TEST_SINGLE
#include "LockFreeMapTestThread.h"
#endif
#include <thread>
#include <cstdio>

LockFreeMapRetryValue::LockFreeMapRetryValue(int value, std::atomic<unsigned>* copies) :
m_value(value),
m_copies(copies)
{

}

LockFreeMapRetryValue::LockFreeMapRetryValue(const LockFreeMapRetryValue& other) :
m_value(other.m_value),
m_copies(nullptr)
{
    if (other.m_copies != nullptr)
    {
        other.m_copies->fetch_add(1);
    }
}

LockFreeMapRetryValue& LockFreeMapRetryValue::operator=(const LockFreeMapRetryValue& other)
{
    m_value = other.m_value;
    m_copies = nullptr;
    return *this;
}

/** 真实满池检查，复制次数证明调用已进入重试
@param [in] mode 0至3为开关组合，4为普通状态接口
@return 状态与最终内容符合独立预期时true
*/
static bool CapacityCase(unsigned mode)
{
    LockFreeMap<int, LockFreeMapRetryValue> map(false);
    LockFreeMapConfig config;
    config.nodes.initialCapacity = 1;
    config.nodes.maximumCapacity = 1;
    config.values.initialCapacity = 4;
    config.values.maximumCapacity = 4;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(1, LockFreeMapRetryValue(10)))
    {
        return false;
    }
    std::atomic<unsigned> copies(0);
    const LockFreeMapRetryValue candidate(20, &copies);
    LockFreeMapStatus status = LOCK_FREE_MAP_INVALID;
#ifndef LOCK_FREE_MAP_TEST_SINGLE
    if (mode == 2 || mode == 3)
    {
        std::atomic<bool> finished(false);
        /** 在独立线程显式等待容量，返回状态后发布finished
        */
        LockFreeMapTestThread worker([&]()
        {
            status = map.trySet(2, candidate, (mode & 1) != 0, true);
            finished.store(true);
        });
        const uint64_t start = LockFreeMapClock::milliseconds();
        uint64_t now = start;
        while (copies.load() < 2 && !finished.load() && start != 0 && now >= start && now - start < 5000)
        {
            std::this_thread::yield();
            now = LockFreeMapClock::milliseconds();
        }
        // 第二次复制证明第一次满池尝试已经失败，不靠睡眠猜测线程进度。
        const bool waited = copies.load() >= 2 && !finished.load();
        const bool erased = map.erase(1);
        const bool joined = worker.join();
        LockFreeMapRetryValue output;
        if (LockFreeMapTestReport::m_file != nullptr)
        {
            std::fprintf(LockFreeMapTestReport::m_file, "retry_capacity,%u,status=%d,waited=%d,erased=%d,joined=%d\n",
                mode, status, waited, erased, joined);
        }
        // 释放容量的删除也改变根；关闭竞争重试时允许BUSY，但必须证明没有提交第二个键。
        if (mode == 2 && status == LOCK_FREE_MAP_BUSY)
        {
            return waited && erased && joined && map.empty() && !map.contains(2);
        }
        return waited && erased && joined && status == LOCK_FREE_MAP_COMMITTED &&
            map.size() == 1 && map.get(2, output) && output.m_value == 20;
    }
#endif
    status = mode == 4 ? map.lockFreeMapSet(2, candidate) : map.trySet(2, candidate, (mode & 1) != 0, mode >= 2);
    return status == LOCK_FREE_MAP_CAPACITY && copies.load() == 1 && map.size() == 1 && !map.contains(2);
}

/** 外层借用仍占用唯一记录时，重入不能等待自身退出
@param [in,out] map 已填充的单记录容器
@return 返回记录不足且未修改内容时true
*/
static bool ReentryCase(LockFreeMap<int, int>& map)
{
    LockFreeMapStatus nested = LOCK_FREE_MAP_INVALID;
    /** 借用期间重入等待容量的写入，检查不会等待自身释放记录
    @note 第1个未命名参数是同步借用输入，本回调不使用其内容
    */
    return map.lockFreeMapVisit(1, [&map, &nested](const int&)
    {
        nested = map.trySet(2, 8, true, true);
    }) == LOCK_FREE_MAP_OK && nested == LOCK_FREE_MAP_RECORD_LIMIT && !map.contains(2);
}

/** 输出复制期间修改无关键，触发真实根发布竞争
@param [in] retry 是否重试竞争
@param [in] capacity 是否允许等待容量
@return 关闭竞争重试为BUSY且未删除，开启后准确删除一次
*/
static bool ContentionCase(bool retry, bool capacity)
{
    LockFreeMap<int, LockFreeMapStatusValue> map;
    if (!map.set(5, LockFreeMapStatusValue(7)))
    {
        return false;
    }
    unsigned changes = 0, errors = 0;
    /** 首次输出赋值时修改无关键，制造真实发布竞争
    */
    std::function<void()> change = [&]()
    {
        if (changes++ == 0)
        {
            errors += map.set(1, LockFreeMapStatusValue(1)) && map.erase(1) ? 0 : 1;
        }
    };
    LockFreeMapStatusValue output(0, &change);
    const LockFreeMapStatus status = map.tryExtract(5, output, retry, capacity);
    return errors == 0 && (retry ? status == LOCK_FREE_MAP_COMMITTED && map.empty() && output.m_value == 7 :
        status == LOCK_FREE_MAP_BUSY && changes == 1 && map.size() == 1 && map.contains(5));
}

/** 查找阶段每次比较都改变根，关闭重试不能被提前continue绕过
@param [in] retry 是否允许重试根验证竞争
@return 关闭时在交错耗尽前返回BUSY，开启时32次交错后返回现有条目
*/
static bool QueryContentionCase(bool retry)
{
    LockFreeMap<LockFreeMapStatusKey, int> map;
    const LockFreeMapStatusKey key(5), lower(1);
    unsigned changes = 0, errors = 0;
    if (!map.set(key, 7))
    {
        return false;
    }
    /** 至多32次改变较小无关键，检查查询竞争重试选项
    */
    std::function<void()> change = [&]()
    {
        if (changes < 32)
        {
            ++changes;
            errors += map.set(lower, 1) && map.erase(lower) ? 0 : 1;
        }
    };
    const LockFreeMapStatusKey query(5, &change);
    LockFreeMapToken token;
    int output = 0;
    const LockFreeMapStatus status = map.tryGetOrInsert(query, 8, output, token, retry, false);
    if (LockFreeMapTestReport::m_file != nullptr)
    {
        std::fprintf(LockFreeMapTestReport::m_file, "retry_query,enabled=%d,status=%d,changes=%u\n", retry, status, changes);
    }
    return errors == 0 && map.size() == 1 && (retry ?
        status == LOCK_FREE_MAP_EXISTS && changes == 32 && output == 7 : status == LOCK_FREE_MAP_BUSY && changes > 0 && changes < 32);
}

#ifndef LOCK_FREE_MAP_TEST_SINGLE
/** 在领取成功、发布新拥有者之前暂停复用者，旧亲和提示不能被当作活动借用
@return 原线程已退出后不被误判为重入，且记录最终正常归还时true
*/
static bool OwnerPublicationCase()
{
    LockFreeMapReclamation domain(1);
    LockFreeMapHazardRecord* record = domain.acquire();
    if (record == nullptr || !domain.mayBeHeldByCurrentThread())
    {
        return false;
    }
    record->release();
    std::atomic<bool> acquired(false), release(false), failed(false);
    /** 领取记录后暂停拥有者发布，检查提示与真实所有权的区分
    */
    LockFreeMapTestThread worker([&]()
    {
        // 这是 acquire 内部第一步的真实 CAS；在下一步发布归属之前停住。
        if (!record->claim())
        {
            failed.store(true);
            return;
        }
        acquired.store(true);
        while (!release.load())
        {
            std::this_thread::yield();
        }
        record->release();
    });
    const uint64_t start = LockFreeMapClock::milliseconds();
    uint64_t now = start;
    while (!acquired.load() && !failed.load() && start != 0 && now >= start && now - start < 5000)
    {
        std::this_thread::yield();
        now = LockFreeMapClock::milliseconds();
    }
    const bool observed = acquired.load();
    const bool mistaken = domain.mayBeHeldByCurrentThread();
    release.store(true);
    const bool joined = worker.join();
    if (LockFreeMapTestReport::m_file != nullptr)
    {
        std::fprintf(LockFreeMapTestReport::m_file, "retry_owner_publication,observed=%d,mistaken=%d,joined=%d\n",
            observed, mistaken, joined);
    }
    return observed && !failed.load() && !mistaken && joined && domain.active() == 0;
}

/** 多线程复用指定数量保护记录，领取后必须能识别自己的活动借用
@param [in] count 本子场景保护记录上限，至少为2
@return 每个线程均取得记录、没有拥有者提示丢失且最终全部归还时true
*/
static bool OwnerReuseCase(size_t count)
{
    LockFreeMapReclamation domain(count);
    if (count == 2)
    {
        // 受控保持全部记录被占用，证明固定尝试次数内零次领取是合法的资源不足。
        LockFreeMapGuard first(domain), second(domain);
        if (first.m_record == nullptr || second.m_record == nullptr)
        {
            return false;
        }
        std::atomic<size_t> unavailable(0), unexpected(0);
        /** 在全部记录已被占用时反复申请，分别累计资源不足和异常结果
        */
        LockFreeMapTestThread blocked([&]()
        {
            for (size_t attempt = 0; attempt < 100000; ++attempt)
            {
                LockFreeMapHazardRecord* record = domain.acquire();
                if (record == nullptr)
                {
                    unavailable.fetch_add(1, std::memory_order_relaxed);
                }
                else
                {
                    unexpected.fetch_add(1, std::memory_order_relaxed);
                    record->release();
                }
            }
        });
        const bool joined = blocked.join();
        if (LockFreeMapTestReport::m_file != nullptr)
        {
            std::fprintf(LockFreeMapTestReport::m_file, "retry_owner_saturated,unavailable=%llu,unexpected=%llu,joined=%d\n",
                static_cast<unsigned long long>(unavailable.load()), static_cast<unsigned long long>(unexpected.load()), joined);
        }
        if (!joined || unavailable.load() != 100000 || unexpected.load() != 0 || domain.active() != 2)
        {
            return false;
        }
    }
    std::atomic<bool> start(false);
    std::atomic<size_t> errors(0);
    size_t acquired[8] = {};
    std::vector<LockFreeMapTestThread> workers;
    LockFreeMapTestThreadGroup group(workers, &start);
    for (size_t worker = 0; worker < 8; ++worker)
    {
        /** 共同起跑后反复领取及归还记录，检查当前线程拥有者识别
        */
        workers.emplace_back([&, worker]()
        {
            while (!start.load())
            {
                std::this_thread::yield();
            }
            const uint64_t started = LockFreeMapClock::milliseconds();
            uint64_t now = started;
            size_t attempts = 0;
            // 所有记录均被占用时，内部尝试领取允许失败；按实际领取次数检查复用，
            // 不能把固定次数空转当成其他线程获得过调度机会。公平性另由公开接口用例验证。
            while (acquired[worker] < 20000 && started != 0 && now >= started && now - started < 5000)
            {
                LockFreeMapHazardRecord* record = domain.acquire();
                if (record != nullptr)
                {
                    ++acquired[worker];
                    if (!domain.mayBeHeldByCurrentThread())
                    {
                        errors.fetch_add(1);
                    }
                    record->release();
                }
                else
                {
                    std::this_thread::yield();
                }
                if ((++attempts & 63) == 0)
                {
                    now = LockFreeMapClock::milliseconds();
                }
            }
        });
    }
    start.store(true);
    bool passed = true;
    for (size_t worker = 0; worker < workers.size(); ++worker)
    {
        const bool joined = workers[worker].join();
        passed = joined && acquired[worker] == 20000 && passed;
        if (LockFreeMapTestReport::m_file != nullptr)
        {
            std::fprintf(LockFreeMapTestReport::m_file, "retry_owner_worker,%llu,acquired=%llu,joined=%d\n",
                static_cast<unsigned long long>(worker), static_cast<unsigned long long>(acquired[worker]), joined);
        }
    }
    if (LockFreeMapTestReport::m_file != nullptr)
    {
        std::fprintf(LockFreeMapTestReport::m_file, "retry_owner_reuse,records=%llu,errors=%llu\n",
            static_cast<unsigned long long>(count), static_cast<unsigned long long>(errors.load()));
    }
    return passed && errors.load() == 0 && domain.active() == 0;
}
#endif

#ifndef LOCK_FREE_MAP_TEST_SINGLE
/** 同线程重入占满64条记录，改变亲和提示后仍须正常复用
@return 没有重复领取、满额明确失败且每轮全部归还时true
*/
static bool NestedRecordsCase()
{
    LockFreeMapReclamation domain(64);
    int source = 7;
    for (size_t round = 0; round < 4; ++round)
    {
        LockFreeMapHazardRecord* records[64] = {};
        bool passed = true;
        for (size_t index = 0; index < 64; ++index)
        {
            records[index] = domain.acquire(&source);
            passed = records[index] != nullptr && passed;
            for (size_t prior = 0; prior < index; ++prior)
            {
                passed = records[index] != records[prior] && passed;
            }
        }
        LockFreeMapHazardRecord* extra = domain.acquire();
        passed = extra == nullptr && domain.active() == 64 && domain.protectsRoot(&source) && passed;
        if (extra != nullptr)
        {
            extra->release();
        }
        for (size_t index = 64; index != 0; --index)
        {
            if (records[index - 1] != nullptr)
            {
                // 亲和提示只影响坐标选择；持有者改变提示后仍按原协议结束借用。
                records[index - 1]->m_threadHint.store(0, std::memory_order_relaxed);
                records[index - 1]->release();
            }
        }
        if (!passed || domain.active() != 0 || domain.protectsRoot(&source))
        {
            return false;
        }
    }
    return true;
}
#endif

/** 查询提前完成后立即复用唯一记录，旧守卫析构不能破坏新借用
@return 旧借用及时结束且新借用持续到新拥有者归还时true
*/
static bool ReadCompletionCase()
{
    LockFreeMapReclamation domain(1);
    int source = 7, replacement = 9;
    std::atomic<int*> current(&source);
    LockFreeMapHazardRecord* reused = nullptr;
    {
        LockFreeMapGuard guard(domain, current);
        if (guard.m_record == nullptr)
        {
            return false;
        }
        guard.m_record->m_pointer[1].store(&source);
        guard.finishRead();
        if (domain.active() != 0 || domain.protectsRoot(&source))
        {
            return false;
        }
        reused = domain.acquire(&replacement);
        if (reused == nullptr)
        {
            return false;
        }
    }
    // 旧守卫已经析构；新的源仍由复用者独占，不能被再次清空。
    const bool preserved = domain.active() == 1 && domain.protectsRoot(&replacement);
    reused->release();
    return preserved && domain.active() == 0 && !domain.protectsRoot(&replacement);
}

/** 根保护兼作领取状态时，清空借用不能提前开放记录复用
@return 初始保护、空源标记及析构归还均满足合同时true
*/
static bool SourceProtectionCase()
{
    LockFreeMapReclamation domain(1);
    int source = 1, replacement = 2;
    std::atomic<int*> current(&source);
    {
        LockFreeMapGuard guard(domain, current);
        if (guard.m_record == nullptr || domain.active() != 1 || !domain.protectsRoot(&source))
        {
            return false;
        }
        guard.m_record->m_pointer[0].store(nullptr);
        if (!guard.m_record->claimed() || domain.protectsRoot(&source))
        {
            return false;
        }
        current.store(&replacement);
        LockFreeMapGuard nested(domain, current);
        if (nested.m_record != nullptr)
        {
            return false;
        }
        guard.m_record->m_pointer[0].store(&replacement);
        if (!domain.protectsRoot(&replacement))
        {
            return false;
        }
        // 值保护与块保护使用普通空地址语义；清空根后它们仍须独立保活，退出时全部清空。
        guard.m_record->m_pointer[1].store(&source);
        guard.m_record->m_pointer[2].store(&replacement);
        guard.m_record->m_pointer[0].store(nullptr);
        if (!domain.protectsValue(&source) || !domain.protectsBlock(&replacement))
        {
            return false;
        }
    }
    if (domain.active() != 0 || domain.protectsValue(&source) || domain.protectsBlock(&replacement) || domain.protectsRoot(&replacement))
    {
        return false;
    }
    for (unsigned iteration = 0; iteration < 3; ++iteration)
    {
        int* expected = iteration == 0 ? &source : iteration == 1 ? &replacement : nullptr;
        current.store(expected);
        LockFreeMapGuard guard(domain, current);
        if (guard.m_record == nullptr || !guard.m_record->claimed() || domain.active() != 1 ||
            guard.m_record->m_pointer[0].load() != expected ||
            (expected != nullptr && !domain.protectsRoot(expected)))
        {
            return false;
        }
        // 来源变化不自动解除旧借用；守卫退出后才能开放复用。
        current.store(nullptr);
        if (expected != nullptr && !domain.protectsRoot(expected))
        {
            return false;
        }
    }
    return domain.active() == 0 && !domain.protectsRoot(&source) && !domain.protectsRoot(&replacement);
}

/** 让值池的唯一块目录被占用，观察配置好的容量重试是否真正经过公开写入口
@return 可阻塞目标按配置等待后重新评估策略，单线程 Wasm 立即报告资源状态时为 true
*/
static bool CheckConfiguredCapacityWait()
{
    unsigned calls = 0;
    uint64_t first = 0, second = 0;
    LockFreeMapConfig config;
    if (config.capacityFastRetries != 4096 || config.capacitySleepMilliseconds != 1 ||
        config.setCapacityWait(0, 4) != LOCK_FREE_MAP_OK || config.setCapacityWait(9, 0) != LOCK_FREE_MAP_INVALID ||
        config.setCapacityWait(9, -1) != LOCK_FREE_MAP_INVALID || config.capacityFastRetries != 0 ||
        config.capacitySleepMilliseconds != 4 || config.values.setCapacity(1, -1) != LOCK_FREE_MAP_OK ||
        config.values.setBlockRecordLimit(1) != LOCK_FREE_MAP_OK)
    {
        return false;
    }
    /** 首次允许申请第二块，后续拒绝增长以结束本次公开调用
    @param [in] context 已分配容量的观察值
    @return 首次且当前仅一槽时返回一个槽，其余返回零
    */
    const auto grow = [&](const LockFreeMapGrowContext& context) -> size_t
    {
        if (++calls == 1)
        {
            first = LockFreeMapBenchmarkClock::nanoseconds();
            return context.currentCapacity == 1 ? 1 : 0;
        }
        second = LockFreeMapBenchmarkClock::nanoseconds();
        return 0;
    };
    LockFreeMap<int, int> map(false);
    config.capacitySleepMilliseconds = 0;
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_INVALID || map.lockFreeMapSet(1, 1) != LOCK_FREE_MAP_INVALID ||
        config.setCapacityWait(0, 4) != LOCK_FREE_MAP_OK ||
        config.values.setGrow(grow) != LOCK_FREE_MAP_OK || map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK ||
        config.setCapacityWait(8, 1) != LOCK_FREE_MAP_OK || !map.set(1, 7))
    {
        return false;
    }
    const LockFreeMapStatus status = map.trySet(1, 9, true, true);
    if (map.value(1) != 7 || map.lockFreeMapStats().activeOperations != 0)
    {
        return false;
    }
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    return status == LOCK_FREE_MAP_RECORD_LIMIT && calls == 1 && second == 0;
#else
    return status == LOCK_FREE_MAP_GROW_DENIED && calls == 2 && first != 0 && second >= first &&
        second - first >= 4000000;
#endif
}

bool LockFreeMapRetryTest()
{
#ifndef LOCK_FREE_MAP_TEST_SINGLE
    if (!OwnerPublicationCase())
    {
        return false;
    }
#endif
    if (!CheckConfiguredCapacityWait() || !SourceProtectionCase() || !ReadCompletionCase() ||
        !QueryContentionCase(false) || !QueryContentionCase(true))
    {
        return false;
    }
    for (unsigned mode = 0; mode < 5; ++mode)
    {
        const bool capacityPassed = CapacityCase(mode);
        const bool contentionPassed = mode == 4 || ContentionCase((mode & 1) != 0, mode >= 2);
        if (LockFreeMapTestReport::m_file != nullptr)
        {
            std::fprintf(LockFreeMapTestReport::m_file, "retry_mode,%u,capacity=%d,contention=%d\n", mode, capacityPassed, contentionPassed);
        }
        if (!capacityPassed || !contentionPassed)
        {
            return false;
        }
        LockFreeMap<int, int> denied(false);
        LockFreeMapConfig config;
        config.nodes.initialCapacity = config.values.initialCapacity = 0;
        unsigned calls = 0;
        /** 统计增长策略调用次数并明确拒绝增长
        @note 第1个未命名参数是同步借用输入，本回调不使用其内容
        @return 始终返回零，表示拒绝增长
        */
        config.nodes.grow = [&calls](const LockFreeMapGrowContext&) -> size_t
        {
            ++calls;
            return 0;
        };
        if (denied.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
        {
            return false;
        }
        const LockFreeMapStatus status = mode == 4 ? denied.lockFreeMapSet(1, 7) : denied.trySet(1, 7, (mode & 1) != 0, mode >= 2);
        if (status != LOCK_FREE_MAP_GROW_DENIED || calls != 1 || !denied.empty())
        {
            return false;
        }
    }
    LockFreeMap<int, int> nested(false);
    LockFreeMapConfig config;
    config.operationRecordLimit = 1;
    if (nested.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !nested.set(1, 7) || !ReentryCase(nested))
    {
        return false;
    }
#ifndef LOCK_FREE_MAP_TEST_SINGLE
    bool otherThread = false;
    /** 换线程执行嵌套借用的资源限制检查，结果在join后读取
    */
    LockFreeMapTestThread worker([&]() { otherThread = ReentryCase(nested); });
    if (!worker.join() || !otherThread || !OwnerReuseCase(2) || !OwnerReuseCase(3) ||
        !OwnerReuseCase(8) || !OwnerReuseCase(64) || !NestedRecordsCase())
    {
        return false;
    }
#endif
    LockFreeMapToken token;
    int value = 0, key = 0;
    if (nested.tryGetOrInsert(2, 2, value, token, true, true) != LOCK_FREE_MAP_COMMITTED ||
        nested.tryUpdate(2, token, 3, true, true) != LOCK_FREE_MAP_COMMITTED ||
        nested.tryUpdate(2, token, 4, true, true) != LOCK_FREE_MAP_CONFLICT ||
        nested.tryEraseIf(2, token, true, true) != LOCK_FREE_MAP_CONFLICT ||
        nested.lockFreeMapLast(key, value, token) != LOCK_FREE_MAP_OK || key != 2 ||
        nested.tryExtractLast(token, key, value, true, true) != LOCK_FREE_MAP_COMMITTED || value != 3 ||
        nested.tryErase(2, true, true) != LOCK_FREE_MAP_NOT_FOUND ||
        nested.tryInsert(2, 4, true, true) != LOCK_FREE_MAP_COMMITTED ||
        nested.tryExtract(2, value, true, true) != LOCK_FREE_MAP_COMMITTED || value != 4)
    {
        return false;
    }
    uint32_t retries = 0;
    for (uint32_t i = 0; i < 4096; ++i)
    {
        if (!LockFreeMapClock::waitCapacity(retries) || retries != i + 1)
        {
            return false;
        }
    }
#ifdef LOCK_FREE_MAP_TEST_SINGLE
    return !LockFreeMapClock::waitCapacity(retries);
#else
    for (unsigned i = 0; i < 8; ++i)
    {
        const uint64_t start = LockFreeMapBenchmarkClock::nanoseconds();
        if (!LockFreeMapClock::waitCapacity(retries) || retries != 4096)
        {
            return false;
        }
        const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
        if (LockFreeMapTestReport::m_file != nullptr)
        {
            std::fprintf(LockFreeMapTestReport::m_file, "retry_sleep,%u,nanoseconds=%llu\n", i,
                static_cast<unsigned long long>(end >= start ? end - start : 0));
        }
        if (start == 0 || end < start || end - start < 1000000)
        {
            return false;
        }
    }
    return true;
#endif
}

bool LockFreeMapGrowthTest()
{
    std::atomic<size_t> nodes(0), values(0), errors(0), notifications(0);
    LockFreeMapConfig config;
    config.nodes.initialCapacity = config.values.initialCapacity = 1;
    config.nodes.maximumCapacity = config.values.maximumCapacity = 128;
    /** 累计节点池实际新增容量及通知次数，检查非零增长信息
    @param [in] added 本次实际新增槽数
    @param [in] count 成功增长次数的观察值，实际通知时应非零
    */
    config.nodes.m_onGrowth = [&](size_t added, uint64_t count)
    {
        nodes.fetch_add(added);
        notifications.fetch_add(1);
        errors.fetch_add(added == 0 || count == 0 ? 1 : 0);
    };
    /** 记录值池增长后抛受控异常，验证通知异常不撤销增长
    @param [in] added 本次实际新增槽数
    @param [in] count 成功增长次数的观察值，实际通知时应非零
    */
    config.values.m_onGrowth = [&](size_t added, uint64_t count)
    {
        values.fetch_add(added);
        errors.fetch_add(added == 0 || count == 0 ? 1 : 0);
        // 观察异常必须被隔离，不能撤销已发布容量或写入。
        throw 1;
    };
    LockFreeMap<int, int> map(false);
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !map.set(0, 7) || nodes.load() != 0 || values.load() != 0)
    {
        return false;
    }
#ifndef LOCK_FREE_MAP_TEST_SINGLE
    /** 并发插入新键，触发实际增长并累计错误
    */
    LockFreeMapTestThread worker([&]()
    {
        for (int key = 1; key <= 16; ++key)
        {
            errors.fetch_add(map.set(key, key) ? 0 : 1);
        }
    });
#else
    for (int key = 1; key <= 16; ++key)
    {
        errors.fetch_add(map.set(key, key) ? 0 : 1);
    }
#endif
    for (int key = 17; key <= 32; ++key)
    {
        errors.fetch_add(map.set(key, key) ? 0 : 1);
    }
#ifndef LOCK_FREE_MAP_TEST_SINGLE
    if (!worker.join())
    {
        return false;
    }
#endif
    const LockFreeMapStats stats = map.lockFreeMapStats();
    if (errors.load() != 0 || map.size() != 33 || nodes.load() != stats.nodes.capacity - 1 ||
        values.load() != stats.values.capacity - 1 || notifications.load() != stats.nodes.successfulGrowCount)
    {
        return false;
    }
    const size_t beforeNodes = nodes.load(), beforeValues = values.load();
    map.clear();
    size_t released = 0;
    if (map.lockFreeMapMaintain(1024, released) != LOCK_FREE_MAP_OK || !map.set(1, 1) ||
        nodes.load() != beforeNodes || values.load() != beforeValues)
    {
        return false;
    }
    // 初始分配不通知；随后被策略拒绝的增长也不通知。
    LockFreeMap<int, int> denied(false);
    config.nodes.initialCapacity = 1;
    config.values.initialCapacity = 4;
    /** 拒绝节点池增长，验证失败增长不产生成功通知
    @note 第1个未命名参数是同步借用输入，本回调不使用其内容
    @return 始终返回零
    */
    config.nodes.grow = [](const LockFreeMapGrowContext&) -> size_t { return 0; };
    if (denied.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK || !denied.set(1, 1) ||
        denied.lockFreeMapSet(2, 2) != LOCK_FREE_MAP_GROW_DENIED)
    {
        return false;
    }
    return nodes.load() == beforeNodes && values.load() == beforeValues && denied.size() == 1;
}