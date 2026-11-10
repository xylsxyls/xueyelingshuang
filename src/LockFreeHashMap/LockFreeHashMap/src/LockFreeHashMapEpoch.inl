#pragma once
#include <limits>
#include <new>
#include <functional>
#include <thread>

inline LockFreeHashMapEpochRecord::LockFreeHashMapEpochRecord() :
m_epoch(0),
m_blockHint(0),
m_wordHint(0),
m_versionNext(0),
m_versionEnd(0),
m_operations(0),
m_countGeneration(0),
m_countDelta(0),
m_cachedCount(0)
{
    for (size_t i = 0; i < 4; ++i)
    {
        m_cachedSlots[i] = 0;
    }
}

inline LockFreeHashMapEpoch::LockFreeHashMapEpoch() :
m_clock(1),
m_allocation(nullptr),
m_records(nullptr),
m_stride(0),
m_count(0)
{

}

inline LockFreeHashMapEpoch::~LockFreeHashMapEpoch()
{
    for (size_t i = 0; i < m_count; ++i)
    {
        record(i)->~LockFreeHashMapEpochRecord();
    }
    delete[] m_allocation;
}

inline bool LockFreeHashMapEpoch::initialize(size_t count)
{
    const size_t maximum = (std::numeric_limits<size_t>::max)();
    const size_t alignment = 64;
    if (m_allocation != nullptr || count == 0 || !supported() ||
        sizeof(LockFreeHashMapEpochRecord) > maximum - (alignment - 1))
    {
        return false;
    }
    size_t stride = (sizeof(LockFreeHashMapEpochRecord) + alignment - 1) & ~(alignment - 1);
    if (count > (maximum - alignment + 1) / stride)
    {
        return false;
    }
    unsigned char* allocation = nullptr;
    try
    {
        allocation = new (std::nothrow) unsigned char[count * stride + alignment - 1];
    }
    catch (...)
    {
        return false;
    }
    if (allocation == nullptr)
    {
        return false;
    }
    uintptr_t address = reinterpret_cast<uintptr_t>(allocation);
    size_t adjustment = static_cast<size_t>((alignment - (address & (alignment - 1))) & (alignment - 1));
    m_allocation = allocation;
    m_records = allocation + adjustment;
    m_stride = stride;
    for (size_t i = 0; i < count; ++i)
    {
        new (m_records + i * stride) LockFreeHashMapEpochRecord();
        ++m_count;
    }
    return true;
}

inline bool LockFreeHashMapEpoch::supported() const
{
    return m_clock.is_lock_free();
}

inline size_t LockFreeHashMapEpoch::count() const
{
    return m_count;
}

inline uint64_t LockFreeHashMapEpoch::current() const
{
    return m_clock.load();
}

inline uint64_t LockFreeHashMapEpoch::advance()
{
    uint64_t cutoff = m_clock.load();
    if (cutoff != (std::numeric_limits<uint64_t>::max)())
    {
        // 其他线程已经推进时共用该次推进；原先观察的截止点只会更保守
        uint64_t observed = cutoff;
        m_clock.compare_exchange_strong(observed, cutoff + 1);
    }
    return cutoff;
}

inline uint64_t LockFreeHashMapEpoch::minimum() const
{
    uint64_t result = 0;
    for (size_t i = 0; i < m_count; ++i)
    {
        uint64_t value = record(i)->m_epoch.load();
        if (value != 0 && (result == 0 || value < result))
        {
            result = value;
        }
    }
    return result;
}

inline LockFreeHashMapEpochRecord* LockFreeHashMapEpoch::record(size_t index) const
{
    return index < m_count ? reinterpret_cast<LockFreeHashMapEpochRecord*>(m_records + index * m_stride) : nullptr;
}

inline size_t LockFreeHashMapEpoch::hint()
{
    // 提示不拥有任何记录；真正所有权始终来自每次接纳 CAS，重入和线程编号复用都不影响安全性
#if defined(_MSC_VER)
    static __declspec(thread) size_t s_hint = static_cast<size_t>(-1);
#elif defined(__GNUC__) || defined(__clang__)
    static __thread size_t s_hint = static_cast<size_t>(-1);
#else
    return 0;
#endif
#if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
    if (s_hint == (std::numeric_limits<size_t>::max)())
    {
        try
        {
            // 不使用函数局部的动态初始化全局计数器，避免 v120 首次并发调用的初始化竞争
            s_hint = std::hash<std::thread::id>()(std::this_thread::get_id());
        }
        catch (...)
        {
            s_hint = 0;
        }
    }
    return s_hint;
#endif
}

inline LockFreeHashMapEpochGuard::LockFreeHashMapEpochGuard(LockFreeHashMapEpoch* domain) :
m_record(nullptr)
{
    if (domain == nullptr || domain->count() == 0)
    {
        return;
    }
    uint64_t epoch = domain->current();
    size_t count = domain->count();
    size_t hint = LockFreeHashMapEpoch::hint();
    // 默认固定数量为二次幂，接纳热路径不执行整数除法
    size_t index = (count & (count - 1)) == 0 ? hint & (count - 1) : hint % count;
    for (size_t i = 0; i < domain->count(); ++i)
    {
        LockFreeHashMapEpochRecord* candidate = domain->record(index);
        uint64_t empty = 0;
        if (candidate->m_epoch.load(std::memory_order_relaxed) == 0 &&
            candidate->m_epoch.compare_exchange_strong(empty, epoch))
        {
            m_record = candidate;
            return;
        }
        if (++index == domain->count())
        {
            index = 0;
        }
    }
}

inline LockFreeHashMapEpochGuard::~LockFreeHashMapEpochGuard()
{
    if (m_record != nullptr)
    {
        // 解引用结束后只需发布退出；进入 CAS、结构摘除和回收扫描保持顺序一致
        m_record->m_epoch.store(0, std::memory_order_release);
    }
}

inline LockFreeHashMapEpochRecord* LockFreeHashMapEpochGuard::record() const
{
    return m_record;
}