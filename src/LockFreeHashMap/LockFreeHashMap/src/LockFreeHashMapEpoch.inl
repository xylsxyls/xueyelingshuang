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
m_cachedCount(0),
m_snapshotBytes(0)
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

inline bool LockFreeHashMapEpoch::validCount(size_t count)
{
    return count != 0 && count <= ((std::numeric_limits<size_t>::max)() - 63) / kRecordStride;
}

inline bool LockFreeHashMapEpoch::initialize(size_t count)
{
    const size_t alignment = 64;
    const size_t stride = kRecordStride;
    if (m_allocation != nullptr || !validCount(count) || !supported())
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
    for (size_t i = 0; i < count; ++i)
    {
        // 首次从零号位图字扫描，保留连续分配的局部性；后续记住实际成功位置。
        new (m_records + i * stride) LockFreeHashMapEpochRecord();
        ++m_count;
    }
    return true;
}

inline bool LockFreeHashMapEpoch::supported() const
{
    LOCKFREEHASHMAP_ALIGN_64 std::atomic<int64_t> delta(0);
    std::atomic<size_t> count(0);
    return m_clock.is_lock_free() && delta.is_lock_free() && count.is_lock_free();
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
    return index < m_count ? reinterpret_cast<LockFreeHashMapEpochRecord*>(m_records + index * kRecordStride) : nullptr;
}

LOCKFREEHASHMAP_FORCE_INLINE size_t& LockFreeHashMapEpoch::hintSlot()
{
#if defined(_MSC_VER)
    static __declspec(thread) size_t s_hint = static_cast<size_t>(-1);
#elif defined(__GNUC__) || defined(__clang__)
    static __thread size_t s_hint = static_cast<size_t>(-1);
#else
    static thread_local size_t s_hint = static_cast<size_t>(-1);
#endif
    return s_hint;
}

inline size_t LockFreeHashMapEpoch::initializeHint() throw()
{
    size_t& hint = hintSlot();
    try
    {
        // 不使用函数局部的动态初始化全局计数器，避免 v120 首次并发调用的初始化竞争。
        hint = std::hash<std::thread::id>()(std::this_thread::get_id());
    }
    catch (...)
    {
        hint = 0;
    }
    return hint;
}

LOCKFREEHASHMAP_FORCE_INLINE size_t LockFreeHashMapEpoch::hint()
{
    // 热路径只读当前线程的数值；初始化内部捕获异常，每次接纳仍通过记录 CAS。
    const size_t hint = hintSlot();
    return hint == (std::numeric_limits<size_t>::max)() ? initializeHint() : hint;
}

LOCKFREEHASHMAP_FORCE_INLINE void LockFreeHashMapEpoch::remember(size_t index)
{
    hintSlot() = index;
}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapEpochRecord* LockFreeHashMapEpoch::acquire(bool rememberHint) throw()
{
    if (m_count == 0)
    {
        return nullptr;
    }
    uint64_t epoch = current();
    size_t count = m_count;
    size_t hint = LockFreeHashMapEpoch::hint();
    // 默认固定数量为二次幂，接纳热路径不执行整数除法
    size_t index = (count & (count - 1)) == 0 ? hint & (count - 1) : hint % count;
    // 提示通常指向本线程上次归还的记录，直接尝试接纳，免去一次重复的原子预读。
    // index 已经取模且记录数组在外部静止时才销毁，内部无需重复执行公开 record 的边界判断。
    LockFreeHashMapEpochRecord* candidate = reinterpret_cast<LockFreeHashMapEpochRecord*>(m_records + index * kRecordStride);
    uint64_t empty = 0;
    if (candidate->m_epoch.compare_exchange_strong(empty, epoch))
    {
        if (rememberHint && index != hint)
        {
            LockFreeHashMapEpoch::remember(index);
        }
        return candidate;
    }
    return acquireRemaining(index, epoch, rememberHint);
}

// 保持头文件的单一定义规则，将首选槽冲突后的扫描留在独立函数内。
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
inline LockFreeHashMapEpochRecord* LockFreeHashMapEpoch::acquireRemaining(size_t index, uint64_t epoch, bool rememberHint) throw()
{
    const size_t count = m_count;
    for (size_t i = 1; i < count; ++i)
    {
        if (++index == count)
        {
            index = 0;
        }
        LockFreeHashMapEpochRecord* candidate = reinterpret_cast<LockFreeHashMapEpochRecord*>(m_records + index * kRecordStride);
        uint64_t empty = 0;
        // 冷扫描仍先检查占用标志，不对每个正在使用的记录执行写入式 CAS。
        if (candidate->m_epoch.load(std::memory_order_relaxed) == 0 &&
            candidate->m_epoch.compare_exchange_strong(empty, epoch))
        {
            if (rememberHint)
            {
                LockFreeHashMapEpoch::remember(index);
            }
            return candidate;
        }
    }
    return nullptr;
}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapEpochGuard::LockFreeHashMapEpochGuard(LockFreeHashMapEpoch* domain, bool trackThread, bool rememberHint) throw() :
m_record(domain != nullptr ? domain->acquire(rememberHint) : nullptr),
m_previous(nullptr),
m_threadHead(nullptr)
{
    if (trackThread)
    {
        track();
    }
}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapEpochGuard::~LockFreeHashMapEpochGuard() throw()
{
    release();
}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapEpochGuard::LockFreeHashMapEpochGuard(LockFreeHashMapReadGuard& source) throw() :
m_record(source.transfer()),
m_previous(nullptr),
m_threadHead(nullptr)
{
    track();
}

LOCKFREEHASHMAP_FORCE_INLINE void LockFreeHashMapEpochGuard::release() throw()
{
    if (m_record != nullptr)
    {
        if (m_threadHead != nullptr)
        {
            *m_threadHead = m_previous;
            m_threadHead = nullptr;
        }
        // 解引用结束后只需发布退出；进入 CAS、结构摘除和回收扫描保持顺序一致
        m_record->m_epoch.store(0, std::memory_order_release);
        m_record = nullptr;
    }
}

inline LockFreeHashMapEpochRecord* LockFreeHashMapEpochGuard::record() const
{
    return m_record;
}

LOCKFREEHASHMAP_FORCE_INLINE void LockFreeHashMapEpochGuard::track() throw()
{
    if (m_record != nullptr && m_threadHead == nullptr)
    {
        m_threadHead = &current();
        m_previous = *m_threadHead;
        *m_threadHead = this;
    }
}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapEpochGuard*& LockFreeHashMapEpochGuard::current()
{
#ifdef _MSC_VER
    static __declspec(thread) LockFreeHashMapEpochGuard* s_current = nullptr;
#else
    static thread_local LockFreeHashMapEpochGuard* s_current = nullptr;
#endif
    return s_current;
}

inline bool LockFreeHashMapEpochGuard::heldByCurrentThread(const LockFreeHashMapEpoch* domain)
{
    if (domain == nullptr)
    {
        return false;
    }
    for (LockFreeHashMapEpochGuard* guard = current(); guard != nullptr; guard = guard->m_previous)
    {
        if (domain->owns(guard->m_record))
        {
            return true;
        }
    }
    return false;
}

inline bool LockFreeHashMapEpoch::owns(const LockFreeHashMapEpochRecord* candidate) const
{
    if (candidate == nullptr || m_count == 0)
    {
        return false;
    }
    // 用整数地址判定，避免对不同数组的指针作关系比较；不计算可能溢出的末端地址。
    uintptr_t begin = reinterpret_cast<uintptr_t>(m_records);
    uintptr_t address = reinterpret_cast<uintptr_t>(candidate);
    if (address < begin)
    {
        return false;
    }
    uintptr_t offset = address - begin;
    return offset % kRecordStride == 0 && offset / kRecordStride < m_count;
}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapReadGuard::LockFreeHashMapReadGuard(LockFreeHashMapEpoch* domain) throw() :
m_record(acquireRecord(domain))
{

}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapReadGuard::LockFreeHashMapReadGuard(LockFreeHashMapEpoch* domain, std::true_type inlineAdmission) throw() :
m_record(domain != nullptr ? domain->acquire() : nullptr)
{
    (void)inlineAdmission;
}

// 将接纳临时状态限制在独立调用内；只返回记录指针，避免把保护对象地址传出读取函数。
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
inline LockFreeHashMapEpochRecord* LockFreeHashMapReadGuard::acquireRecord(LockFreeHashMapEpoch* domain) throw()
{
    return domain != nullptr ? domain->acquire() : nullptr;
}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapReadGuard::~LockFreeHashMapReadGuard() throw()
{
    release();
}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapEpochRecord* LockFreeHashMapReadGuard::record() const
{
    return m_record;
}

LOCKFREEHASHMAP_FORCE_INLINE void LockFreeHashMapReadGuard::release() throw()
{
    if (m_record != nullptr)
    {
        m_record->m_epoch.store(0, std::memory_order_release);
        m_record = nullptr;
    }
}

LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapEpochRecord* LockFreeHashMapReadGuard::transfer() throw()
{
    LockFreeHashMapEpochRecord* record = m_record;
    m_record = nullptr;
    return record;
}