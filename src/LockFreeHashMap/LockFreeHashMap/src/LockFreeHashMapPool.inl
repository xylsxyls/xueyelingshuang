#pragma once
#include <algorithm>
#include <chrono>
#include <limits>
#include <new>
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER < 1900
#include <Windows.h>
#endif

inline LockFreeHashMapPoolEntry::LockFreeHashMapPoolEntry() :
m_address(0),
m_count(0)
{

}

template <typename T>
LockFreeHashMapPoolSlot<T>::LockFreeHashMapPoolSlot() :
m_block(nullptr),
m_retired(0),
m_index(0),
m_kind(0)
{

}

template <typename T>
T* LockFreeHashMapPoolSlot<T>::payload()
{
    return reinterpret_cast<T*>(&m_payload);
}

template <typename T>
LockFreeHashMapPoolBlock<T>::LockFreeHashMapPoolBlock() :
m_allocation(nullptr),
m_slots(nullptr),
m_bits(nullptr),
m_count(0),
m_words(0),
m_leases(0),
m_retired(0),
m_idle(0),
m_idleLease(0),
m_allocationAttempts(0),
m_reclaimCursor(0)
{

}

template <typename T>
LockFreeHashMapPoolBlock<T>::~LockFreeHashMapPoolBlock()
{
    for (size_t i = 0; i < m_count; ++i)
    {
        if (m_slots[i].m_kind.load() != 0)
        {
            // 与标准容器相同，用户载荷的析构必须不抛异常
            m_slots[i].payload()->~T();
        }
        m_slots[i].~LockFreeHashMapPoolSlot<T>();
    }
    delete[] m_allocation;
    delete[] m_bits;
}

template <typename T>
bool LockFreeHashMapPoolBlock<T>::initialize(size_t count)
{
    typedef LockFreeHashMapPoolSlot<T> Slot;
    const size_t alignment = (std::max)(size_t(64), size_t(std::alignment_of<Slot>::value));
    const size_t maximum = (std::numeric_limits<size_t>::max)();
    if (m_allocation != nullptr || count == 0 || count > 65535 || count > (maximum - alignment + 1) / sizeof(Slot))
    {
        return false;
    }
    try
    {
        ++m_allocationAttempts;
        m_allocation = new (std::nothrow) unsigned char[count * sizeof(Slot) + alignment - 1];
        if (m_allocation == nullptr)
        {
            return false;
        }
        uintptr_t address = reinterpret_cast<uintptr_t>(m_allocation);
        size_t adjustment = static_cast<size_t>((alignment - (address & (alignment - 1))) & (alignment - 1));
        m_slots = reinterpret_cast<Slot*>(m_allocation + adjustment);
        m_words = (count + 63) / 64;
        ++m_allocationAttempts;
        m_bits = new (std::nothrow) std::atomic<uint64_t>[m_words * 2];
        if (m_bits == nullptr)
        {
            return false;
        }
        for (size_t i = 0; i < m_words * 2; ++i)
        {
            m_bits[i].store(0);
        }
        if ((count & 63) != 0)
        {
            m_bits[m_words - 1].store(~((uint64_t(1) << (count & 63)) - 1));
        }
        for (size_t i = 0; i < count; ++i)
        {
            new (m_slots + i) Slot();
            ++m_count;
            m_slots[i].m_block = this;
            m_slots[i].m_index = static_cast<uint32_t>(i);
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename T>
LockFreeHashMapPool<T>::LockFreeHashMapPool() :
m_directory(nullptr),
m_high(0),
m_budget(0),
m_peak(0),
m_grows(0),
m_allocations(0),
m_lastGrow(0),
m_cursor(0)
{

}

template <typename T>
LockFreeHashMapPool<T>::~LockFreeHashMapPool()
{
    freeBlocks();
    delete[] m_directory;
}

template <typename T>
LockFreeHashMapStatus LockFreeHashMapPool<T>::initialize(const LockFreeHashMapPoolConfig& config)
{
    const size_t maximum = (std::numeric_limits<size_t>::max)();
    if (m_directory != nullptr || config.maximumCapacity < -1 || config.directoryCapacity == 0 ||
        config.directoryCapacity > maximum / sizeof(LockFreeHashMapPoolEntry) || config.protectionRecordCount == 0 ||
        config.initialCapacity > maximum / sizeof(LockFreeHashMapPoolSlot<T>) ||
        (config.maximumCapacity >= 0 && static_cast<uint64_t>(config.maximumCapacity) < config.initialCapacity) ||
        (config.lookupCacheCapacity != 0 && (config.lookupCacheCapacity & (config.lookupCacheCapacity - 1)) != 0) ||
        config.initialCapacity / 65535 + (config.initialCapacity % 65535 != 0 ? 1 : 0) > config.directoryCapacity)
    {
        return LockFreeHashMapInvalidArgument;
    }
    std::atomic<uintptr_t> pointer(0);
    if (!pointer.is_lock_free() || !m_budget.is_lock_free() || !m_grows.is_lock_free() || !m_epoch.supported())
    {
        return LockFreeHashMapUnsupported;
    }
    m_config = config;
    if (!m_epoch.initialize(config.protectionRecordCount))
    {
        return LockFreeHashMapNoMemory;
    }
    try
    {
        m_directory = new (std::nothrow) LockFreeHashMapPoolEntry[config.directoryCapacity];
    }
    catch (...)
    {
        return LockFreeHashMapNoMemory;
    }
    if (m_directory == nullptr)
    {
        return LockFreeHashMapNoMemory;
    }
    size_t remaining = config.initialCapacity;
    while (remaining != 0)
    {
        size_t count = (std::min)(remaining, size_t(65535));
        LockFreeHashMapStatus status = grow(count, true);
        if (status != LockFreeHashMapOk)
        {
            freeBlocks();
            return status;
        }
        remaining -= count;
    }
    return LockFreeHashMapOk;
}

template <typename T>
LockFreeHashMapEpoch* LockFreeHashMapPool<T>::epoch()
{
    return &m_epoch;
}

template <typename T>
LockFreeHashMapPoolSlot<T>* LockFreeHashMapPool<T>::take(LockFreeHashMapPoolBlock<T>* block, LockFreeHashMapEpochRecord& record)
{
    const uint64_t closed = uint64_t(1) << 63;
    const uint64_t versionMask = UINT64_C(0x7fffffff00000000);
    uint64_t leases = block->m_leases.load(std::memory_order_relaxed);
    uint64_t next = 0;
    do
    {
        if ((leases & closed) != 0 || (leases & UINT64_C(0xffffffff)) > UINT64_C(0xfffffffb))
        {
            return nullptr;
        }
        // 活动序号耗尽后仍可分配，但禁用该块自动缩容；绝不回绕伪造连续空闲
        next = leases + 4 + ((leases & versionMask) == versionMask ? 0 : (uint64_t(1) << 32));
    } while (!block->m_leases.compare_exchange_weak(leases, next));
    size_t word = record.m_wordHint % block->m_words;
    for (size_t checked = 0; checked < block->m_words; ++checked)
    {
        uint64_t occupied = block->m_bits[word].load(std::memory_order_relaxed);
        while (~occupied != 0)
        {
            uint64_t available = ~occupied;
            uint64_t claimed = 0;
            size_t count = 0;
            while (available != 0 && count < 4)
            {
                uint64_t lowest = available & (~available + 1);
                claimed |= lowest;
                available &= ~lowest;
                ++count;
            }
            if (block->m_bits[word].compare_exchange_weak(occupied, occupied | claimed))
            {
                // 先预留四份以防计数溢出；不足四槽时在发布缓存之前归还多余预留
                if (count < 4)
                {
                    block->m_leases.fetch_sub(4 - count);
                }
                size_t first = firstBit(claimed);
                claimed &= ~(uint64_t(1) << first);
                size_t cached = 0;
                while (claimed != 0)
                {
                    size_t bit = firstBit(claimed);
                    record.m_cachedSlots[cached++] = reinterpret_cast<uintptr_t>(block->m_slots + word * 64 + bit);
                    claimed &= ~(uint64_t(1) << bit);
                }
                record.m_cachedCount.store(cached, std::memory_order_relaxed);
                record.m_wordHint = word;
                return block->m_slots + word * 64 + first;
            }
        }
        if (++word == block->m_words)
        {
            word = 0;
        }
    }
    block->m_leases.fetch_sub(4);
    return nullptr;
}

template <typename T>
LockFreeHashMapPoolSlot<T>* LockFreeHashMapPool<T>::scan(LockFreeHashMapEpochRecord& record)
{
    size_t cached = record.m_cachedCount.load(std::memory_order_relaxed);
    if (cached != 0)
    {
        LockFreeHashMapPoolSlot<T>* result = reinterpret_cast<LockFreeHashMapPoolSlot<T>*>(record.m_cachedSlots[cached - 1]);
        record.m_cachedCount.store(cached - 1, std::memory_order_relaxed);
        return result;
    }
    size_t high = m_high.load();
    if (high == 0)
    {
        return nullptr;
    }
    size_t index = record.m_blockHint % high;
    for (size_t checked = 0; checked < high; ++checked)
    {
        uintptr_t address = m_directory[index].m_address.load();
        if (address != 0 && (address & 1) == 0)
        {
            LockFreeHashMapPoolBlock<T>* block = reinterpret_cast<LockFreeHashMapPoolBlock<T>*>(address);
            LockFreeHashMapPoolSlot<T>* result = take(block, record);
            if (result != nullptr)
            {
                record.m_blockHint = index;
                return result;
            }
        }
        if (++index == high)
        {
            index = 0;
        }
    }
    return nullptr;
}

template <typename T>
LockFreeHashMapPoolSlot<T>* LockFreeHashMapPool<T>::transfer(LockFreeHashMapEpochRecord& record)
{
    uint64_t epoch = m_epoch.current();
    for (size_t i = 0; i < m_epoch.count(); ++i)
    {
        LockFreeHashMapEpochRecord* other = m_epoch.record(i);
        if (other == &record || other->m_cachedCount.load(std::memory_order_relaxed) == 0)
        {
            continue;
        }
        uint64_t empty = 0;
        if (!other->m_epoch.compare_exchange_strong(empty, epoch))
        {
            continue;
        }
        LockFreeHashMapPoolSlot<T>* result = nullptr;
        size_t count = other->m_cachedCount.load(std::memory_order_relaxed);
        if (count != 0)
        {
            result = reinterpret_cast<LockFreeHashMapPoolSlot<T>*>(other->m_cachedSlots[count - 1]);
            other->m_cachedCount.store(count - 1, std::memory_order_relaxed);
        }
        other->m_epoch.store(0, std::memory_order_release);
        if (result != nullptr)
        {
            return result;
        }
    }
    return nullptr;
}

template <typename T>
LockFreeHashMapPoolSlot<T>* LockFreeHashMapPool<T>::acquire(LockFreeHashMapEpochRecord& record, LockFreeHashMapStatus& status)
{
    status = LockFreeHashMapOk;
    LockFreeHashMapPoolSlot<T>* result = scan(record);
    if (result == nullptr)
    {
        result = transfer(record);
    }
    if (result == nullptr)
    {
        reclaim();
        result = scan(record);
        if (result == nullptr)
        {
            result = transfer(record);
        }
        if (result == nullptr)
        {
            status = evaluateGrowth(true);
            result = scan(record);
            if (result != nullptr)
            {
                status = LockFreeHashMapOk;
            }
            else if (status == LockFreeHashMapOk)
            {
                status = LockFreeHashMapReservationBusy;
            }
        }
    }
    if (result != nullptr && m_config.growFreeThreshold != 0)
    {
        size_t capacity = 0, available = 0;
        observe(capacity, available);
        if (available <= m_config.growFreeThreshold)
        {
            reclaim();
            observe(capacity, available);
            if (available <= m_config.growFreeThreshold)
            {
                // 预增长失败不撤销已经取得的槽位
                evaluateGrowth(false);
            }
        }
    }
    return result;
}

template <typename T>
void LockFreeHashMapPool<T>::release(LockFreeHashMapPoolSlot<T>* slot)
{
    if (slot == nullptr)
    {
        return;
    }
    LockFreeHashMapPoolBlock<T>* block = slot->m_block;
    destroyPayload(slot);
    block->m_bits[slot->m_index / 64].fetch_and(~(uint64_t(1) << (slot->m_index & 63)), std::memory_order_release);
    block->m_leases.fetch_sub(1);
}

template <typename T>
void LockFreeHashMapPool<T>::destroyPayload(LockFreeHashMapPoolSlot<T>* slot)
{
    if (slot->m_kind.load(std::memory_order_relaxed) != 0)
    {
        slot->payload()->~T();
        slot->m_kind.store(0, std::memory_order_relaxed);
    }
    slot->m_retired.store(0, std::memory_order_relaxed);
}

template <typename T>
void LockFreeHashMapPool<T>::recycle(LockFreeHashMapPoolSlot<T>* slot, LockFreeHashMapEpochRecord& record)
{
    size_t count = record.m_cachedCount.load(std::memory_order_relaxed);
    if (count >= 4)
    {
        release(slot);
        return;
    }
    destroyPayload(slot);
    record.m_cachedSlots[count] = reinterpret_cast<uintptr_t>(slot);
    record.m_cachedCount.store(count + 1, std::memory_order_relaxed);
}

template <typename T>
void LockFreeHashMapPool<T>::drain(LockFreeHashMapEpochRecord& record)
{
    size_t count = record.m_cachedCount.exchange(0, std::memory_order_relaxed);
    for (size_t i = 0; i < count; ++i)
    {
        release(reinterpret_cast<LockFreeHashMapPoolSlot<T>*>(record.m_cachedSlots[i]));
    }
}

template <typename T>
void LockFreeHashMapPool<T>::retire(LockFreeHashMapPoolSlot<T>* slot)
{
    if (slot != nullptr)
    {
        // 摘除先于取代际，扫描开始后的退役必定落在新的代际中
        slot->m_retired.store(m_epoch.current(), std::memory_order_relaxed);
        slot->m_block->m_bits[slot->m_block->m_words + slot->m_index / 64].fetch_or(uint64_t(1) << (slot->m_index & 63), std::memory_order_release);
    }
}

template <typename T>
size_t LockFreeHashMapPool<T>::reclaim(size_t wordLimit)
{
    uint64_t cutoff = m_epoch.advance();
    LockFreeHashMapEpochGuard guard(&m_epoch);
    if (guard.record() == nullptr)
    {
        return 0;
    }
    uint64_t minimum = m_epoch.minimum();
    size_t high = m_high.load();
    size_t freed = 0, checkedWords = 0;
    size_t start = high == 0 ? 0 : m_cursor.fetch_add(1, std::memory_order_relaxed) % high;
    for (size_t checked = 0; checked < high; ++checked)
    {
        size_t index = start + checked;
        if (index >= high)
        {
            index -= high;
        }
        uintptr_t address = m_directory[index].m_address.load();
        if (address == 0 || (address & 1) != 0)
        {
            continue;
        }
        LockFreeHashMapPoolBlock<T>* block = reinterpret_cast<LockFreeHashMapPoolBlock<T>*>(address);
        size_t step = wordLimit == 0 ? block->m_words : (std::min)(wordLimit - checkedWords, block->m_words);
        size_t word = block->m_reclaimCursor.fetch_add(step, std::memory_order_relaxed) % block->m_words;
        for (size_t visited = 0; visited < block->m_words; ++visited)
        {
            if (wordLimit != 0 && checkedWords++ == wordLimit)
            {
                return freed;
            }
            uint64_t bits = block->m_bits[block->m_words + word].load();
            if (bits != 0)
            {
                // 位图交换一次性取得本字的退役责任；占用位在析构结束前不归还，槽身份不能被复用
                bits = block->m_bits[block->m_words + word].exchange(0, std::memory_order_acq_rel);
            }
            uint64_t claimed = 0;
            uint64_t deferred = 0;
            while (bits != 0)
            {
                size_t bit = firstBit(bits);
                uint64_t mask = uint64_t(1) << bit;
                LockFreeHashMapPoolSlot<T>* slot = block->m_slots + word * 64 + bit;
                uint64_t retired = slot->m_retired.load();
                if (retired != 0 && retired != (std::numeric_limits<uint64_t>::max)() && retired <= cutoff &&
                    (minimum == 0 || retired < minimum))
                {
                    claimed |= mask;
                }
                else
                {
                    deferred |= mask;
                }
                bits &= ~mask;
            }
            if (deferred != 0)
            {
                // 尚不安全的槽只归还退役责任；保留同字中其他线程新发布的位，不等待本轮析构
                block->m_bits[block->m_words + word].fetch_or(deferred, std::memory_order_release);
            }
            if (claimed != 0)
            {
                // 退役位已经独占移出；析构后才发布可复用位，暂停的析构不会暴露半个空槽
                uint64_t released = 0;
                size_t releasedCount = 0;
                while (claimed != 0)
                {
                    size_t bit = firstBit(claimed);
                    uint64_t mask = uint64_t(1) << bit;
                    LockFreeHashMapPoolSlot<T>* slot = block->m_slots + word * 64 + bit;
                    if (guard.record()->m_cachedCount.load(std::memory_order_relaxed) < 4)
                    {
                        recycle(slot, *guard.record());
                    }
                    else
                    {
                        destroyPayload(slot);
                        released |= mask;
                        ++releasedCount;
                    }
                    claimed &= ~mask;
                    ++freed;
                }
                if (released != 0)
                {
                    block->m_bits[word].fetch_and(~released, std::memory_order_release);
                    block->m_leases.fetch_sub(releasedCount);
                }
            }
            if (++word == block->m_words)
            {
                word = 0;
            }
        }
    }
    return freed;
}

template <typename T>
LockFreeHashMapStatus LockFreeHashMapPool<T>::grow(size_t requested, bool initial)
{
    size_t maximum = (std::numeric_limits<size_t>::max)();
    if (m_config.maximumCapacity >= 0 && static_cast<uint64_t>(m_config.maximumCapacity) < maximum)
    {
        maximum = static_cast<size_t>(m_config.maximumCapacity);
    }
    requested = (std::min)(requested, size_t(65535));
    if (requested == 0)
    {
        return LockFreeHashMapGrowthDeclined;
    }
    size_t budget = m_budget.load();
    size_t count = 0;
    do
    {
        if (budget >= maximum)
        {
            size_t capacity = 0, available = 0;
            observe(capacity, available);
            return capacity < budget || available != 0 ? LockFreeHashMapReservationBusy : LockFreeHashMapCapacityLimit;
        }
        count = (std::min)(requested, maximum - budget);
    } while (!m_budget.compare_exchange_weak(budget, budget + count));
    size_t peak = m_peak.load(std::memory_order_relaxed);
    while (peak < budget + count && !m_peak.compare_exchange_weak(peak, budget + count))
    {

    }
    size_t index = 0;
    for (; index < m_config.directoryCapacity; ++index)
    {
        uintptr_t empty = 0;
        if (m_directory[index].m_address.compare_exchange_strong(empty, 1))
        {
            break;
        }
    }
    if (index == m_config.directoryCapacity)
    {
        m_budget.fetch_sub(count);
        return LockFreeHashMapDirectoryFull;
    }
    size_t high = m_high.load();
    while (high < index + 1 && !m_high.compare_exchange_weak(high, index + 1))
    {

    }
    LockFreeHashMapPoolBlock<T>* block = nullptr;
    try
    {
        increment(m_allocations);
        block = new (std::nothrow) LockFreeHashMapPoolBlock<T>();
        bool initialized = block != nullptr && block->initialize(count);
        if (block != nullptr)
        {
            for (size_t attempt = 0; attempt < block->m_allocationAttempts; ++attempt)
            {
                increment(m_allocations);
            }
        }
        if (!initialized)
        {
            delete block;
            m_directory[index].m_address.store(0);
            m_budget.fetch_sub(count);
            return LockFreeHashMapNoMemory;
        }
    }
    catch (...)
    {
        delete block;
        m_directory[index].m_address.store(0);
        m_budget.fetch_sub(count);
        return LockFreeHashMapNoMemory;
    }
    m_directory[index].m_count.store(count);
    m_directory[index].m_address.store(reinterpret_cast<uintptr_t>(block));
    if (!initial)
    {
        increment(m_grows);
        uint64_t time = now();
        uint64_t last = m_lastGrow.load();
        while (last < time && !m_lastGrow.compare_exchange_weak(last, time))
        {

        }
    }
    return LockFreeHashMapOk;
}

template <typename T>
LockFreeHashMapStatus LockFreeHashMapPool<T>::evaluateGrowth(bool exhausted)
{
    size_t capacity = 0, available = 0;
    observe(capacity, available);
    if (!exhausted && available > m_config.growFreeThreshold)
    {
        return LockFreeHashMapOk;
    }
    LockFreeHashMapGrowContext context = { m_config.initialCapacity, m_grows.load(), capacity };
    size_t count = 0;
    try
    {
        count = m_config.growPolicy ? m_config.growPolicy(context) : (std::max)(size_t(64), capacity / 2);
    }
    catch (...)
    {
        return LockFreeHashMapUserException;
    }
    return grow(count, false);
}

template <typename T>
void LockFreeHashMapPool<T>::observe(size_t& capacity, size_t& available)
{
    capacity = 0;
    available = 0;
    for (size_t i = 0; i < m_epoch.count(); ++i)
    {
        available += m_epoch.record(i)->m_cachedCount.load(std::memory_order_relaxed);
    }
    size_t high = m_high.load();
    for (size_t i = 0; i < high; ++i)
    {
        uintptr_t address = m_directory[i].m_address.load();
        if (address <= 1)
        {
            continue;
        }
        capacity += m_directory[i].m_count.load();
        if ((address & 1) == 0)
        {
            LockFreeHashMapPoolBlock<T>* block = reinterpret_cast<LockFreeHashMapPoolBlock<T>*>(address);
            if ((block->m_leases.load() >> 63) != 0)
            {
                continue;
            }
            for (size_t word = 0; word < block->m_words; ++word)
            {
                available += population(~block->m_bits[word].load(std::memory_order_relaxed));
            }
        }
    }
}

template <typename T>
void LockFreeHashMapPool<T>::close(size_t index, LockFreeHashMapPoolBlock<T>* block)
{
    uintptr_t expected = reinterpret_cast<uintptr_t>(block);
    m_directory[index].m_address.compare_exchange_strong(expected, expected | 1);
    uint64_t empty = 0;
    block->m_retired.compare_exchange_strong(empty, m_epoch.current());
}

template <typename T>
LockFreeHashMapResult LockFreeHashMapPool<T>::shrink()
{
    if (!m_config.shrinkPolicy)
    {
        bool closing = false;
        for (size_t i = 0; i < m_high.load(); ++i)
        {
            uintptr_t address = m_directory[i].m_address.load();
            closing = closing || (address > 1 && (address & 1) != 0);
        }
        if (!closing)
        {
            return LockFreeHashMapResult();
        }
    }
    uint64_t cutoff = m_epoch.advance();
    LockFreeHashMapEpochGuard guard(&m_epoch);
    if (guard.record() == nullptr)
    {
        return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
    }
    // 只转移已空闲记录；暂停操作持有的最多四槽继续占预算，不等待其退出
    drain(*guard.record());
    for (size_t i = 0; i < m_epoch.count(); ++i)
    {
        LockFreeHashMapEpochRecord* record = m_epoch.record(i);
        uint64_t empty = 0;
        if (record != guard.record() && record->m_cachedCount.load(std::memory_order_relaxed) != 0 &&
            record->m_epoch.compare_exchange_strong(empty, m_epoch.current()))
        {
            drain(*record);
            record->m_epoch.store(0, std::memory_order_release);
        }
    }
    size_t capacity = 0, available = 0;
    observe(capacity, available);
    uint64_t time = now();
    uint64_t last = m_lastGrow.load();
    LockFreeHashMapShrinkContext context = { m_config.initialCapacity, m_grows.load(), capacity, available,
        m_grows.load() != 0, time >= last && last != 0 ? time - last : 0 };
    LockFreeHashMapShrinkDecision decision;
    try
    {
        if (m_config.shrinkPolicy)
        {
            decision = m_config.shrinkPolicy(context);
        }
    }
    catch (...)
    {
        return LockFreeHashMapResult(LockFreeHashMapUserException);
    }
    uint64_t minimum = m_epoch.minimum();
    size_t freed = 0;
    const uint64_t closed = uint64_t(1) << 63;
    const uint64_t versionMask = UINT64_C(0x7fffffff00000000);
    size_t high = m_high.load();
    // 已关闭容量不再参与可关闭额度，防止连续维护轮次合计低于初始下限
    for (size_t i = 0; i < high; ++i)
    {
        uintptr_t address = m_directory[i].m_address.load();
        if (address > 1 && (address & 1) != 0)
        {
            size_t count = m_directory[i].m_count.load();
            capacity = capacity >= count ? capacity - count : 0;
        }
    }
    for (size_t i = 0; i < high; ++i)
    {
        uintptr_t address = m_directory[i].m_address.load();
        if (address <= 1)
        {
            continue;
        }
        LockFreeHashMapPoolBlock<T>* block = reinterpret_cast<LockFreeHashMapPoolBlock<T>*>(address & ~uintptr_t(1));
        uint64_t leases = block->m_leases.load();
        if ((leases & UINT64_C(0xffffffff)) == 0 && (leases & closed) == 0 && (leases & versionMask) != versionMask &&
            decision.shrinkCount >= block->m_count && decision.idleMilliseconds != 0 && time != 0 &&
            capacity >= m_config.initialCapacity && block->m_count <= capacity - m_config.initialCapacity)
        {
            uint64_t idle = block->m_idle;
            if (idle == 0 || block->m_idleLease != leases)
            {
                block->m_idle = time;
                block->m_idleLease = leases;
            }
            else if (time >= idle && time - idle >= decision.idleMilliseconds && block->m_leases.compare_exchange_strong(leases, closed))
            {
                close(i, block);
                decision.shrinkCount -= block->m_count;
                capacity -= block->m_count;
            }
        }
        if (block->m_leases.load() == closed)
        {
            close(i, block);
            uint64_t retired = block->m_retired.load();
            if (retired != 0 && retired != (std::numeric_limits<uint64_t>::max)() && retired <= cutoff &&
                (minimum == 0 || retired < minimum) &&
                block->m_retired.compare_exchange_strong(retired, (std::numeric_limits<uint64_t>::max)()))
            {
                size_t count = block->m_count;
                m_directory[i].m_address.store(0);
                delete block;
                m_budget.fetch_sub(count);
                freed += count;
            }
        }
    }
    return LockFreeHashMapResult(LockFreeHashMapOk, false, freed);
}

template <typename T>
LockFreeHashMapPoolStats LockFreeHashMapPool<T>::stats()
{
    LockFreeHashMapPoolStats result = {};
    LockFreeHashMapEpochGuard guard(&m_epoch);
    if (guard.record() == nullptr)
    {
        return result;
    }
    observe(result.capacity, result.reusableFree);
    result.budget = m_budget.load();
    result.reservedCapacity = result.budget > result.capacity ? result.budget - result.capacity : 0;
    result.peakBudget = m_peak.load();
    result.protectionRecords = m_epoch.count();
    result.successfulGrowCount = m_grows.load();
    result.systemAllocationCount = m_allocations.load();
    result.retainedBytes = sizeof(*this) + m_config.directoryCapacity * sizeof(LockFreeHashMapPoolEntry) +
        m_epoch.count() * ((sizeof(LockFreeHashMapEpochRecord) + 63) & ~size_t(63)) + 63;
    for (size_t i = 0; i < m_epoch.count(); ++i)
    {
        result.activeProtectionRecords += m_epoch.record(i)->m_epoch.load() != 0 ? 1 : 0;
        result.cachedFree += m_epoch.record(i)->m_cachedCount.load(std::memory_order_relaxed);
    }
    size_t high = m_high.load();
    for (size_t i = 0; i < high; ++i)
    {
        uintptr_t address = m_directory[i].m_address.load();
        if (address <= 1)
        {
            continue;
        }
        ++result.blockCount;
        size_t count = m_directory[i].m_count.load();
        result.retainedBytes += sizeof(LockFreeHashMapPoolBlock<T>) + count * sizeof(LockFreeHashMapPoolSlot<T>) +
            (std::max)(size_t(64), size_t(std::alignment_of<LockFreeHashMapPoolSlot<T>>::value)) - 1 +
            ((count + 63) / 64) * 2 * sizeof(std::atomic<uint64_t>);
        if ((address & 1) != 0)
        {
            result.closedCapacity += count;
            continue;
        }
        LockFreeHashMapPoolBlock<T>* block = reinterpret_cast<LockFreeHashMapPoolBlock<T>*>(address);
        for (size_t j = 0; j < block->m_count; ++j)
        {
            uint32_t kind = block->m_slots[j].m_kind.load();
            result.keyNodes += kind == 1 ? 1 : 0;
            result.valueVersions += kind == 2 ? 1 : 0;
            result.retired += block->m_slots[j].m_retired.load() != 0 ? 1 : 0;
        }
    }
    result.occupied = result.capacity >= result.reusableFree + result.closedCapacity ?
        result.capacity - result.reusableFree - result.closedCapacity : 0;
    return result;
}

template <typename T>
void LockFreeHashMapPool<T>::freeBlocks()
{
    if (m_directory == nullptr)
    {
        return;
    }
    for (size_t i = 0; i < m_epoch.count(); ++i)
    {
        m_epoch.record(i)->m_cachedCount.store(0);
    }
    for (size_t i = 0; i < m_high.load(); ++i)
    {
        uintptr_t address = m_directory[i].m_address.exchange(0);
        if (address > 1)
        {
            delete reinterpret_cast<LockFreeHashMapPoolBlock<T>*>(address & ~uintptr_t(1));
        }
    }
    m_high.store(0);
    m_budget.store(0);
}

template <typename T>
const LockFreeHashMapPoolConfig& LockFreeHashMapPool<T>::config() const
{
    return m_config;
}

template <typename T>
void LockFreeHashMapPool<T>::setPolicies(const LockFreeHashMapGrowPolicy& grow, const LockFreeHashMapShrinkPolicy& shrink)
{
    m_config.growPolicy = grow;
    m_config.shrinkPolicy = shrink;
}

template <typename T>
uint64_t LockFreeHashMapPool<T>::now()
{
    try
    {
#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER < 1900
        // v120 的 steady_clock 继承 system_clock，不能用于连续空闲时间判定
        LARGE_INTEGER ticks = {}, frequency = {};
        if (!QueryPerformanceCounter(&ticks) || !QueryPerformanceFrequency(&frequency) || ticks.QuadPart < 0 || frequency.QuadPart <= 0)
        {
            return 0;
        }
        uint64_t seconds = static_cast<uint64_t>(ticks.QuadPart / frequency.QuadPart);
        uint64_t remainder = static_cast<uint64_t>(ticks.QuadPart % frequency.QuadPart);
        if (seconds > ((std::numeric_limits<uint64_t>::max)() - 1001) / 1000)
        {
            return 0;
        }
        return seconds * 1000 + static_cast<uint64_t>(double(remainder) * 1000 / double(frequency.QuadPart)) + 1;
#else
        int64_t value = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        return value > 0 ? static_cast<uint64_t>(value) : 1;
#endif
    }
    catch (...)
    {
        return 0;
    }
}

template <typename T>
size_t LockFreeHashMapPool<T>::population(uint64_t bits)
{
    bits -= (bits >> 1) & UINT64_C(0x5555555555555555);
    bits = (bits & UINT64_C(0x3333333333333333)) + ((bits >> 2) & UINT64_C(0x3333333333333333));
    return static_cast<size_t>((((bits + (bits >> 4)) & UINT64_C(0x0f0f0f0f0f0f0f0f)) * UINT64_C(0x0101010101010101)) >> 56);
}

template <typename T>
size_t LockFreeHashMapPool<T>::firstBit(uint64_t bits)
{
#if defined(_MSC_VER) && defined(_WIN64)
    unsigned long index = 0;
    _BitScanForward64(&index, bits);
    return index;
#elif defined(_MSC_VER)
    unsigned long index = 0;
    if (_BitScanForward(&index, static_cast<unsigned long>(bits)) != 0)
    {
        return index;
    }
    _BitScanForward(&index, static_cast<unsigned long>(bits >> 32));
    return index + 32;
#elif defined(__GNUC__) || defined(__clang__)
    return static_cast<size_t>(__builtin_ctzll(bits));
#else
    size_t index = 0;
    while ((bits & 1) == 0)
    {
        ++index;
        bits >>= 1;
    }
    return index;
#endif
}

template <typename T>
void LockFreeHashMapPool<T>::increment(std::atomic<uint64_t>& counter)
{
    uint64_t value = counter.load(std::memory_order_relaxed);
    while (value != (std::numeric_limits<uint64_t>::max)() && !counter.compare_exchange_weak(value, value + 1))
    {

    }
}

template <typename T>
LockFreeHashMapPoolReturn<T>::LockFreeHashMapPoolReturn(LockFreeHashMapPool<T>* pool) :
m_pool(pool)
{

}

template <typename T>
void LockFreeHashMapPoolReturn<T>::operator()(LockFreeHashMapPoolSlot<T>* slot) const
{
    m_pool->release(slot);
}