#pragma once
#include <algorithm>
#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER < 1900
// VS2013 的 steady_clock 继承系统时钟；仅旧工具链改用性能计数器，保留调用方原有宏状态
#pragma push_macro("min")
#pragma push_macro("max")
#include <Windows.h>
#pragma pop_macro("max")
#pragma pop_macro("min")
#endif

inline LockFreeHashMapHazards::LockFreeHashMapHazards() :
m_used(0), m_registered(0), m_writeCount(0), m_registeredOnce(false), m_nextVersion(0), m_lastVersion(0), m_free(nullptr)
{
    for (size_t i = 0; i < 5; ++i)
    {
        m_pointer[i].store(nullptr);
    }
    for (size_t i = 0; i < 7; ++i)
    {
        m_spare[i].store(nullptr);
    }
}

inline std::atomic<void*>& LockFreeHashMapHazards::cache(size_t index)
{
    return index == 0 ? m_free : m_spare[index - 1];
}

inline LockFreeHashMapHazards* LockFreeHashMapHazards::acquire(LockFreeHashMapHazards* records, size_t count,
    std::atomic<size_t>& span, uintptr_t hint, uintptr_t admission)
{
    // 用栈区域提示分散同时调用者，不引入 TLS 所有权或全局接纳计数；碰撞只影响探测起点，每条记录仍最多检查一次
    hint >>= 16;
    hint ^= hint >> 4;
    hint ^= hint >> 8;
    hint ^= hint >> 16;
    // 将读者分散到有界记录库，稀疏永久登记位图让回收成本取决于实际使用的记录数
    size_t mask = count >= 128 ? 127 : (count >= 64 ? 63 : (count >= 32 ? 31 :
        (count >= 16 ? 15 : (count >= 8 ? 7 : (count >= 4 ? 3 : (count >= 2 ? 1 : 0))))));
    size_t index = hint & mask;
    for (size_t scanned = 0; records != nullptr && scanned < count; ++scanned)
    {
        size_t i = index;
        if (++index == count)
        {
            index = 0;
        }
        if (records[i].m_used.load(std::memory_order_relaxed))
        {
            continue;
        }
        uintptr_t expected = 0;
        if (records[i].m_used.compare_exchange_strong(expected, admission))
        {
            LockFreeHashMapHazards* record = records + i;
            if (!record->m_registeredOnce)
            {
                records[i / 64].m_registered.fetch_or(UINT64_C(1) << (i & 63));
                size_t previous = span.load();
                while (previous < i + 1 && !span.compare_exchange_weak(previous, i + 1))
                {

                }
                // m_used 的释放把此标志交给下一位借用者；回收者不读取它，也不能先于永久登记发布
                record->m_registeredOnce = true;
            }
            return record;
        }
    }
    return nullptr;
}

inline LockFreeHashMapGuard::LockFreeHashMapGuard(LockFreeHashMapHazards* records, size_t count, std::atomic<size_t>& span, void* directPointer, bool directNode) :
m_record(nullptr), m_predecessorSlot(1), m_currentSlot(2), m_scalar()
{
    uintptr_t admission = directPointer == nullptr ? uintptr_t(1) :
        (reinterpret_cast<uintptr_t>(directPointer) | (directNode ? uintptr_t(1) : uintptr_t(0)));
    m_record = LockFreeHashMapHazards::acquire(records, count, span, reinterpret_cast<uintptr_t>(this), admission);
}

inline LockFreeHashMapGuard::~LockFreeHashMapGuard()
{
    if (m_record != nullptr)
    {
        if ((m_record->m_used.load(std::memory_order_relaxed) & 1) != 0)
        {
            for (size_t i = 0; i < 5; ++i)
            {
                m_record->m_pointer[i].store(nullptr, std::memory_order_release);
            }
        }
        m_record->m_used.store(0, std::memory_order_release);
    }
}

inline void LockFreeHashMapGuard::protect(size_t index, void* pointer)
{
    // 只有当前借用者写入保护格，重复保护同一地址沿用已有稳定发布
    if (m_record->m_pointer[index].load(std::memory_order_relaxed) != pointer)
    {
        m_record->m_pointer[index].store(pointer, pointer == nullptr ? std::memory_order_release : std::memory_order_seq_cst);
    }
}

inline void LockFreeHashMapGuard::resetLinks()
{
    protect(1, nullptr);
    protect(2, nullptr);
    m_predecessorSlot = 1;
    m_currentSlot = 2;
}

inline void LockFreeHashMapGuard::advance()
{
    std::swap(m_predecessorSlot, m_currentSlot);
}

template <typename K, typename V>
LockFreeHashMapSlot<K, V>::LockFreeHashMapSlot() :
m_state(0), m_freeNext(65535), m_index(0), m_next(0), m_value(nullptr), m_block(nullptr), m_generation(0), m_version(0), m_hash(0)
{

}

template <typename K, typename V>
K* LockFreeHashMapSlot<K, V>::key()
{
    return reinterpret_cast<K*>(&m_keyStorage);
}

template <typename K, typename V>
V* LockFreeHashMapSlot<K, V>::value()
{
    return reinterpret_cast<V*>(&m_valueStorage);
}

template <typename K, typename V>
LockFreeHashMapBlock<K, V>::LockFreeHashMapBlock() :
m_slots(nullptr), m_count(0), m_slotAllocation(nullptr), m_activity(0), m_freeHead(65535), m_ready(false),
m_idleActivity(0), m_idleSince(0)
{

}

template <typename K, typename V>
LockFreeHashMapStorage<K, V>::LockFreeHashMapStorage(const LockFreeHashMapPoolConfig& config, size_t ownerBytes) :
m_config(config), m_inlineScalar(config.inlineScalarValues && LockFreeHashMapScalar<V>::enabled()), m_lookup(nullptr), m_lookupAllocation(nullptr), m_records(nullptr), m_recordSpan(0), m_directory(nullptr), m_budget(0), m_capacity(0),
m_reserved(0), m_reusable(0), m_blocks(0), m_directorySpan(0), m_peakBudget(0), m_grows(0), m_allocations(0), m_retries(0), m_lastGrow(0), m_emptySequence(1),
m_reclaimCursor(0), m_maintenanceCursor(0), m_cacheMaintenanceCursor(0), m_cacheReclaimCursor(0), m_shrinkPending(0), m_ownerBytes(ownerBytes)
{

}

template <typename K, typename V>
LockFreeHashMapStorage<K, V>::~LockFreeHashMapStorage()
{
    freeQuiescent();
    if (m_lookup != nullptr)
    {
        for (size_t i = 0; i < m_config.lookupCacheCapacity; ++i)
        {
            m_lookup[i].~Cache();
        }
    }
    delete[] m_lookupAllocation;
    delete[] m_directory;
    delete[] m_records;
}

template <typename K, typename V>
void LockFreeHashMapStorage<K, V>::freeQuiescent()
{
    if (m_lookup != nullptr)
    {
        for (size_t i = 0; i < m_config.lookupCacheCapacity; ++i)
        {
            m_lookup[i].m_node.store(nullptr);
            m_lookup[i].m_cachedValue.store(nullptr);
            m_lookup[i].m_stamp.store(0);
            m_lookup[i].m_colliding.store(false);
        }
    }
    if (m_directory != nullptr)
    {
        for (size_t i = 0; i < m_config.directoryCapacity; ++i)
        {
            Block* item = reinterpret_cast<Block*>(reinterpret_cast<uintptr_t>(m_directory[i].load()) & ~uintptr_t(1));
            if (item == nullptr)
            {
                continue;
            }
            for (size_t j = 0; j < item->m_count; ++j)
            {
                Slot& slot = item->m_slots[j];
                int32_t state = slot.m_state.load();
                if (state == 1 || state == 3)
                {
                    slot.key()->~K();
                }
                else if ((state == 2 || state == 4 || state == 7) && !m_inlineScalar)
                {
                    slot.value()->~V();
                }
            }
            destroyBlock(item);
            m_directory[i].store(nullptr);
        }
    }
    if (m_records != nullptr)
    {
        for (size_t i = 0; i < m_config.protectionRecordCount; ++i)
        {
            for (size_t cell = 0; cell < 8; ++cell)
            {
                m_records[i].cache(cell).store(nullptr);
            }
        }
    }
    m_emptySequence.store(1);
    m_budget.store(0);
    m_capacity.store(0);
    m_reserved.store(0);
    m_reusable.store(0);
    m_blocks.store(0);
    m_directorySpan.store(0);
    m_shrinkPending = 0;
    m_cacheMaintenanceCursor = 0;
    m_cacheReclaimCursor.store(0);
}

template <typename K, typename V>
uint64_t LockFreeHashMapStorage<K, V>::now()
{
#if defined(_WIN32) && defined(_MSC_VER) && _MSC_VER < 1900
    LARGE_INTEGER ticks = {}, frequency = {};
    if (!QueryPerformanceCounter(&ticks) || !QueryPerformanceFrequency(&frequency) ||
        ticks.QuadPart < 0 || frequency.QuadPart <= 0)
    {
        // 时钟不可用时不开始空闲区间，也不把纪元不同的时钟值用于缩容
        return 0;
    }
    uint64_t seconds = static_cast<uint64_t>(ticks.QuadPart / frequency.QuadPart);
    uint64_t remainder = static_cast<uint64_t>(ticks.QuadPart % frequency.QuadPart);
    return seconds * 1000 + static_cast<uint64_t>(double(remainder) * 1000 / double(frequency.QuadPart)) + 1;
#else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count()) + 1;
#endif
}

template <typename K, typename V>
void LockFreeHashMapStorage<K, V>::increment(std::atomic<uint64_t>& counter)
{
    uint64_t value = counter.load();
    while (value != (std::numeric_limits<uint64_t>::max)() && !counter.compare_exchange_weak(value, value + 1))
    {

    }
}

template <typename K, typename V>
LockFreeHashMapStatus LockFreeHashMapStorage<K, V>::init()
{
    const size_t limit = (std::numeric_limits<size_t>::max)();
    if ((m_config.lookupCacheCapacity & (m_config.lookupCacheCapacity - 1)) != 0 ||
        m_config.lookupCacheCapacity > (limit - 15) / sizeof(Cache) ||
        m_config.maximumCapacity < -1 || m_config.directoryCapacity == 0 ||
        m_config.directoryCapacity > limit / (sizeof(std::atomic<Block*>) + sizeof(Block)) ||
        m_config.protectionRecordCount == 0 || m_config.protectionRecordCount > 65535 ||
        m_config.protectionRecordCount > limit / sizeof(LockFreeHashMapHazards) ||
        m_config.initialCapacity > limit / sizeof(Slot) ||
        (m_config.initialCapacity / 65535 + (m_config.initialCapacity % 65535 != 0)) > m_config.directoryCapacity ||
        (m_config.maximumCapacity >= 0 && static_cast<uint64_t>(m_config.maximumCapacity) < m_config.initialCapacity))
    {
        return LockFreeHashMapInvalidArgument;
    }
    size_t remaining = limit - sizeof(*this);
    if (m_ownerBytes > remaining)
    {
        return LockFreeHashMapInvalidArgument;
    }
    remaining -= m_ownerBytes;
    if (remaining < 15 || m_config.lookupCacheCapacity > (remaining - 15) / sizeof(Cache))
    {
        return LockFreeHashMapInvalidArgument;
    }
    remaining -= m_config.lookupCacheCapacity * sizeof(Cache) + 15;
    if (m_config.directoryCapacity > remaining / (sizeof(std::atomic<Block*>) + sizeof(Block) + 64))
    {
        return LockFreeHashMapInvalidArgument;
    }
    remaining -= m_config.directoryCapacity * (sizeof(std::atomic<Block*>) + sizeof(Block) + 64);
    if (m_config.protectionRecordCount > remaining / sizeof(LockFreeHashMapHazards))
    {
        return LockFreeHashMapInvalidArgument;
    }
    remaining -= m_config.protectionRecordCount * sizeof(LockFreeHashMapHazards);
    if (m_config.initialCapacity > remaining / sizeof(Slot))
    {
        return LockFreeHashMapInvalidArgument;
    }
    // C++11 数组申请只保证基础对齐，超出支持范围的扩展对齐载荷明确拒绝
    size_t alignmentLimit = std::alignment_of<long double>::value;
    if (std::alignment_of<K>::value > alignmentLimit || std::alignment_of<V>::value > alignmentLimit)
    {
        return LockFreeHashMapUnsupported;
    }
    std::atomic<uint64_t> a64(0);
    std::atomic<uintptr_t> ap(0);
    std::atomic<int32_t> ai(0);
    std::atomic<uint16_t> a16(0);
    std::atomic<bool> ab(false);
    std::atomic<Slot*> as(nullptr);
    if (!a64.is_lock_free() || !ap.is_lock_free() || !ai.is_lock_free() || !a16.is_lock_free() || !ab.is_lock_free() || !as.is_lock_free())
    {
        return LockFreeHashMapUnsupported;
    }
    try
    {
        if (m_config.lookupCacheCapacity != 0)
        {
            const size_t alignment = std::alignment_of<Cache>::value;
            m_lookupAllocation = new (std::nothrow) unsigned char[m_config.lookupCacheCapacity * sizeof(Cache) + alignment - 1];
            if (m_lookupAllocation == nullptr)
            {
                return LockFreeHashMapNoMemory;
            }
            uintptr_t address = reinterpret_cast<uintptr_t>(m_lookupAllocation);
            m_lookup = reinterpret_cast<Cache*>((address + alignment - 1) & ~(uintptr_t(alignment) - 1));
            for (size_t i = 0; i < m_config.lookupCacheCapacity; ++i)
            {
                new (m_lookup + i) Cache;
                m_lookup[i].m_node.store(nullptr);
                m_lookup[i].m_cachedValue.store(nullptr);
                m_lookup[i].m_stamp.store(0);
                m_lookup[i].m_readKey.store(0);
                m_lookup[i].m_readValue.store(0);
                m_lookup[i].m_colliding.store(false);
            }
        }
        m_records = new (std::nothrow) LockFreeHashMapHazards[m_config.protectionRecordCount];
        if (m_records == nullptr)
        {
            return LockFreeHashMapNoMemory;
        }
        m_directory = new (std::nothrow) std::atomic<Block*>[m_config.directoryCapacity];
        if (m_directory == nullptr)
        {
            return LockFreeHashMapNoMemory;
        }
    }
    catch (const std::bad_alloc&)
    {
        return LockFreeHashMapNoMemory;
    }
    for (size_t i = 0; i < m_config.directoryCapacity; ++i)
    {
        m_directory[i].store(nullptr);
    }
    remaining = m_config.initialCapacity;
    while (remaining != 0)
    {
        size_t count = (std::min)(remaining, size_t(65535));
        LockFreeHashMapStatus status = grow(count, true);
        if (status != LockFreeHashMapOk)
        {
            return status;
        }
        remaining -= count;
    }
    return LockFreeHashMapOk;
}

template <typename K, typename V>
typename LockFreeHashMapStorage<K, V>::Block* LockFreeHashMapStorage<K, V>::block(size_t index, LockFreeHashMapGuard& guard) const
{
    Block* item = m_directory[index].load();
    if ((reinterpret_cast<uintptr_t>(item) & 1) != 0)
    {
        return nullptr;
    }
    guard.protect(4, item);
    if (m_directory[index].load() != item)
    {
        return nullptr;
    }
    return item;
}

template <typename K, typename V>
size_t LockFreeHashMapStorage<K, V>::firstRecord(uint64_t bits)
{
    size_t index = 0;
    if ((bits & UINT64_C(0xffffffff)) == 0)
    {
        bits >>= 32;
        index += 32;
    }
    if ((bits & UINT64_C(0xffff)) == 0)
    {
        bits >>= 16;
        index += 16;
    }
    if ((bits & UINT64_C(0xff)) == 0)
    {
        bits >>= 8;
        index += 8;
    }
    if ((bits & UINT64_C(0xf)) == 0)
    {
        bits >>= 4;
        index += 4;
    }
    if ((bits & UINT64_C(3)) == 0)
    {
        bits >>= 2;
        index += 2;
    }
    return index + ((bits & 1) == 0 ? 1 : 0);
}

template <typename K, typename V>
bool LockFreeHashMapStorage<K, V>::protectedPointer(void* pointer, const LockFreeHashMapHazards* excluded) const
{
    // 永久登记位和上界发布都先于源校验；扫描若遗漏更晚的首次登记，则该读者随后不能成功校验已经退役的源
    size_t groups = (m_recordSpan.load() + 63) / 64;
    for (size_t group = 0; group < groups; ++group)
    {
        uint64_t bits = m_records[group].m_registered.load();
        while (bits != 0)
        {
            size_t index = group * 64 + firstRecord(bits);
            bits &= bits - 1;
            const LockFreeHashMapHazards* record = m_records + index;
            if (record == excluded)
            {
                continue;
            }
            uintptr_t owner = record->m_used.load();
            if (owner > 1 && (owner & ~uintptr_t(1)) == reinterpret_cast<uintptr_t>(pointer))
            {
                return true;
            }
            if ((owner & 1) == 0)
            {
                continue;
            }
            for (size_t j = 0; j < 5; ++j)
            {
                if (record->m_pointer[j].load() == pointer)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

template <typename K, typename V>
typename LockFreeHashMapStorage<K, V>::Slot* LockFreeHashMapStorage<K, V>::available(int32_t kind, LockFreeHashMapGuard& guard, bool& exhausted)
{
    Slot* cached = cachedSlot(kind, *guard.m_record);
    if (cached != nullptr)
    {
        return cached;
    }
    size_t span = m_directorySpan.load();
    // 不同保护记录优先尝试不同批次，每个批次仍至多检查一次
    size_t index = span > 1 ? static_cast<size_t>(guard.m_record - m_records) % span : 0;
    for (size_t i = 0; i < span; ++i)
    {
        Block* item = block(index, guard);
        if (++index == span)
        {
            index = 0;
        }
        if (item == nullptr || !item->m_ready.load())
        {
            continue;
        }
        uint64_t activity = item->m_activity.load();
        exhausted = exhausted || (activity >> 16) == UINT64_C(0x7fffffffffff);
        while ((activity >> 16) < UINT64_C(0x7fffffffffff) && (activity & 65535) < item->m_count)
        {
            if (!item->m_activity.compare_exchange_weak(activity, activity + 65537))
            {
                increment(m_retries);
                continue;
            }
            // 先预留占用，即使本线程暂停也保持块存储有效
            uint64_t head = item->m_freeHead.load();
            while ((head & 65535) != 65535)
            {
                Slot* slot = item->m_slots + static_cast<size_t>(head & 65535);
                uint16_t next = slot->m_freeNext.load(std::memory_order_relaxed);
                uint64_t desired = ((head & ~UINT64_C(65535)) + UINT64_C(65536)) | next;
                if (item->m_freeHead.compare_exchange_weak(head, desired))
                {
                    slot->m_state.store(kind, std::memory_order_relaxed);
                    m_reusable.fetch_sub(1);
                    guard.protect(4, nullptr);
                    return slot;
                }
                increment(m_retries);
            }
            item->m_activity.fetch_sub(1);
            break;
        }
    }
    guard.protect(4, nullptr);
    size_t records = m_recordSpan.load();
    for (size_t i = 0; i < records; ++i)
    {
        cached = cachedSlot(kind, m_records[i]);
        if (cached != nullptr)
        {
            return cached;
        }
    }
    return nullptr;
}

template <typename K, typename V>
void LockFreeHashMapStorage<K, V>::release(Slot* slot, bool constructed, LockFreeHashMapGuard* guard)
{
    if (constructed)
    {
        int32_t state = slot->m_state.load();
        if (state == 1 || state == 3)
        {
            slot->key()->~K();
        }
        else if (!m_inlineScalar)
        {
            slot->value()->~V();
        }
    }
    for (size_t i = 0; guard != nullptr && i < 8; ++i)
    {
        std::atomic<void*>& cell = guard->m_record->cache(i);
        if (cell.load(std::memory_order_relaxed) == nullptr)
        {
            // 仅记录借用者向自身格发布，其他帮助者只能领取已有格
            slot->m_state.store(6, std::memory_order_release);
            cell.store(slot, std::memory_order_release);
            return;
        }
    }
    Block* item = slot->m_block;
    m_reusable.fetch_add(1);
    slot->m_state.store(0, std::memory_order_release);
    uint64_t head = item->m_freeHead.load();
    for (;;)
    {
        slot->m_freeNext.store(static_cast<uint16_t>(head & 65535), std::memory_order_relaxed);
        uint64_t desired = ((head & ~UINT64_C(65535)) + UINT64_C(65536)) | slot->m_index;
        if (item->m_freeHead.compare_exchange_weak(head, desired))
        {
            break;
        }
        increment(m_retries);
    }
    item->m_activity.fetch_sub(1);
}

template <typename K, typename V>
void LockFreeHashMapStorage<K, V>::retire(Slot* slot)
{
    // 唯一成功摘除者拥有此状态转换，无需另外申请退役记录；回收者先赢得顺序一致 CAS 再扫描保护，因此摘除者只需释放发布
    slot->m_state.store(slot->m_state.load(std::memory_order_relaxed) + 2, std::memory_order_release);
}

template <typename K, typename V>
bool LockFreeHashMapStorage<K, V>::reclaimProtected(Slot* slot, LockFreeHashMapGuard& guard)
{
    int32_t state = slot->m_state.load();
    if ((state != 3 && state != 4) || !slot->m_state.compare_exchange_strong(state, 5))
    {
        return false;
    }
    if (protectedPointer(slot, guard.m_record))
    {
        slot->m_state.store(state);
        return false;
    }
    if (state == 3)
    {
        slot->key()->~K();
    }
    else if (!m_inlineScalar)
    {
        slot->value()->~V();
    }
    release(slot, false, &guard);
    return true;
}

template <typename K, typename V>
size_t LockFreeHashMapStorage<K, V>::reclaim(LockFreeHashMapGuard& guard, size_t budget)
{
    size_t cacheBudget = budget / 2 + (budget & 1);
    size_t remainingCache = cacheBudget;
    size_t released = reclaimCached(guard, remainingCache);
    budget -= cacheBudget - remainingCache;
    size_t span = m_directorySpan.load();
    if (budget == 0 || span == 0)
    {
        return released;
    }
    size_t cursor = m_reclaimCursor.fetch_add(1);
    for (size_t i = 0; i < span && budget != 0; ++i)
    {
        Block* item = block((cursor + i) % span, guard);
        if (item == nullptr || !item->m_ready.load())
        {
            continue;
        }
        size_t start = (cursor / span) % item->m_count;
        for (size_t j = 0; j < item->m_count && budget != 0; ++j, --budget)
        {
            Slot* slot = item->m_slots + ((start + j) % item->m_count);
            int32_t state = slot->m_state.load();
            if ((state == 3 || state == 4) && slot->m_state.compare_exchange_strong(state, 5))
            {
                if (protectedPointer(slot))
                {
                    slot->m_state.store(state);
                }
                else
                {
                    if (state == 3)
                    {
                        slot->key()->~K();
                    }
                    else if (!m_inlineScalar)
                    {
                        slot->value()->~V();
                    }
                    release(slot, false);
                    ++released;
                }
            }
        }
    }
    guard.protect(4, nullptr);
    return released;
}

template <typename K, typename V>
LockFreeHashMapStatus LockFreeHashMapStorage<K, V>::grow(size_t requested, bool initial)
{
    if (requested == 0)
    {
        return LockFreeHashMapGrowthDeclined;
    }
    size_t maximum = maximumSlots();
    if (m_config.maximumCapacity >= 0 && static_cast<uint64_t>(m_config.maximumCapacity) < maximum)
    {
        maximum = static_cast<size_t>(m_config.maximumCapacity);
    }
    size_t old = m_budget.load();
    size_t count = 0;
    for (;;)
    {
        if (old >= maximum)
        {
            return m_budget.load() > m_capacity.load() ? LockFreeHashMapReservationBusy : LockFreeHashMapCapacityLimit;
        }
        count = (std::min)((std::min)(requested, maximum - old), size_t(65535));
        if (m_budget.compare_exchange_weak(old, old + count))
        {
            break;
        }
        increment(m_retries);
    }
    size_t peak = m_peakBudget.load();
    while (peak < old + count && !m_peakBudget.compare_exchange_weak(peak, old + count))
    {

    }
    m_reserved.fetch_add(count);
    Block* item = nullptr;
    try
    {
        increment(m_allocations);
        item = new (std::nothrow) Block;
        if (item == nullptr)
        {
            m_reserved.fetch_sub(count);
            m_budget.fetch_sub(count);
            return LockFreeHashMapNoMemory;
        }
        increment(m_allocations);
        const size_t alignment = std::alignment_of<Slot>::value;
        item->m_slotAllocation = new (std::nothrow) unsigned char[count * sizeof(Slot) + alignment - 1];
        if (item->m_slotAllocation == nullptr)
        {
            destroyBlock(item);
            m_reserved.fetch_sub(count);
            m_budget.fetch_sub(count);
            return LockFreeHashMapNoMemory;
        }
        uintptr_t address = reinterpret_cast<uintptr_t>(item->m_slotAllocation);
        item->m_slots = reinterpret_cast<Slot*>((address + alignment - 1) & ~(uintptr_t(alignment) - 1));
        for (size_t i = 0; i < count; ++i)
        {
            new (item->m_slots + i) Slot;
            ++item->m_count;
            item->m_slots[i].m_block = item;
            item->m_slots[i].m_index = static_cast<uint16_t>(i);
            item->m_slots[i].m_freeNext.store(i + 1 == count ? uint16_t(65535) : static_cast<uint16_t>(i + 1));
        }
        item->m_freeHead.store(0);
    }
    catch (const std::bad_alloc&)
    {
        destroyBlock(item);
        m_reserved.fetch_sub(count);
        m_budget.fetch_sub(count);
        return LockFreeHashMapNoMemory;
    }
    for (size_t i = 0; i < m_config.directoryCapacity; ++i)
    {
        Block* expected = nullptr;
        if (m_directory[i].compare_exchange_strong(expected, item))
        {
            size_t span = m_directorySpan.load();
            while (span < i + 1 && !m_directorySpan.compare_exchange_weak(span, i + 1))
            {

            }
            m_capacity.fetch_add(count);
            m_blocks.fetch_add(1);
            m_reusable.fetch_add(count);
            m_reserved.fetch_sub(count);
            if (!initial)
            {
                increment(m_grows);
                m_lastGrow.store(now());
            }
            item->m_ready.store(true);
            return LockFreeHashMapOk;
        }
    }
    destroyBlock(item);
    m_reserved.fetch_sub(count);
    m_budget.fetch_sub(count);
    return LockFreeHashMapDirectoryFull;
}

template <typename K, typename V>
LockFreeHashMapStatus LockFreeHashMapStorage<K, V>::evaluateGrow(LockFreeHashMapGuard& guard, bool empty)
{
    reclaim(guard, 256);
    if (reusableFree() > (empty ? 0 : m_config.growFreeThreshold))
    {
        return LockFreeHashMapOk;
    }
    // 尚未就绪的已发布批次及申请预留不会使调用者等待唯一持有者
    if (m_budget.load() > m_capacity.load())
    {
        return LockFreeHashMapReservationBusy;
    }
    size_t requested = 0;
    try
    {
        LockFreeHashMapGrowContext context = { m_config.initialCapacity, m_grows.load(), m_capacity.load() };
        requested = m_config.growPolicy ? m_config.growPolicy(context) : (std::max)(size_t(64), context.currentCapacity / 2);
    }
    catch (...)
    {
        return LockFreeHashMapUserException;
    }
    return grow(requested, false);
}

template <typename K, typename V>
typename LockFreeHashMapStorage<K, V>::Slot* LockFreeHashMapStorage<K, V>::acquire(int32_t kind, LockFreeHashMapGuard& guard, LockFreeHashMapStatus& status)
{
    bool exhausted = false;
    Slot* slot = available(kind, guard, exhausted);
    if (slot == nullptr)
    {
        // 只有实际无可用槽位的分配路径要求完整回收扫描
        reclaim(guard, (std::numeric_limits<size_t>::max)());
        slot = available(kind, guard, exhausted);
        if (slot == nullptr)
        {
            status = evaluateGrow(guard, true);
            slot = available(kind, guard, exhausted);
            if (slot == nullptr)
            {
                if (status == LockFreeHashMapOk)
                {
                    status = exhausted ? LockFreeHashMapVersionExhausted : LockFreeHashMapReservationBusy;
                }
                return nullptr;
            }
        }
    }
    status = LockFreeHashMapOk;
    if (m_config.growFreeThreshold != 0)
    {
        evaluateGrow(guard, false);
    }
    return slot;
}

template <typename K, typename V>
LockFreeHashMapPoolStats LockFreeHashMapStorage<K, V>::stats(LockFreeHashMapGuard& guard) const
{
    LockFreeHashMapPoolStats result = {};
    result.capacity = m_capacity.load();
    result.reservedCapacity = m_reserved.load();
    result.budget = m_budget.load();
    result.peakBudget = m_peakBudget.load();
    result.blockCount = m_blocks.load();
    result.successfulGrowCount = m_grows.load();
    result.systemAllocationCount = m_allocations.load();
    result.casRetries = m_retries.load();
    result.protectionRecords = m_config.protectionRecordCount;
    result.inlineScalarValues = m_inlineScalar;
    result.lookupCacheEntries = m_config.lookupCacheCapacity;
    result.lookupCacheBytes = m_config.lookupCacheCapacity * sizeof(Cache) +
        (m_lookup != nullptr ? std::alignment_of<Cache>::value - 1 : 0);
    result.retainedBytes = sizeof(*this) + m_config.directoryCapacity * sizeof(std::atomic<Block*>) +
        m_config.protectionRecordCount * sizeof(LockFreeHashMapHazards) + result.capacity * sizeof(Slot) +
        m_config.lookupCacheCapacity * sizeof(Cache) + (m_lookup != nullptr ? std::alignment_of<Cache>::value - 1 : 0) +
        result.blockCount * (sizeof(Block) + std::alignment_of<Slot>::value - 1) + m_ownerBytes;
    for (size_t i = 0; i < m_config.protectionRecordCount; ++i)
    {
        result.activeProtectionRecords += m_records[i].m_used.load() ? 1 : 0;
    }
    size_t span = m_directorySpan.load();
    for (size_t i = 0; i < span; ++i)
    {
        Block* item = block(i, guard);
        if (item == nullptr)
        {
            continue;
        }
        for (size_t j = 0; j < item->m_count; ++j)
        {
            int32_t state = item->m_slots[j].m_state.load();
            result.reusableFree += state == 0 || state == 6 ? 1 : 0;
            result.cachedFree += state == 6 ? 1 : 0;
            result.occupied += state != 0 && state != 6 ? 1 : 0;
            result.retired += (state >= 3 && state <= 5) || state == 7 ? 1 : 0;
            result.keyNodes += state == 1 || state == 3 ? 1 : 0;
            result.valueVersions += state == 2 || state == 4 || state == 7 ? 1 : 0;
        }
    }
    guard.protect(4, nullptr);
    return result;
}

template <typename K, typename V>
LockFreeHashMapResult LockFreeHashMapStorage<K, V>::maintain(LockFreeHashMapGuard& guard, size_t blockBudget)
{
    // 按预算归还空闲缓存格，带标志的退役值由 reclaimCached 负责
    size_t cells = m_recordSpan.load() * 8;
    for (size_t i = 0; i < (std::min)(blockBudget, cells); ++i)
    {
        size_t index = (m_cacheMaintenanceCursor + i) % cells;
        std::atomic<void*>& cell = m_records[index % (cells / 8)].cache(index / (cells / 8));
        void* expected = cell.load();
        if (expected != nullptr && (reinterpret_cast<uintptr_t>(expected) & 1) == 0 &&
            cell.compare_exchange_strong(expected, nullptr))
        {
            release(static_cast<Slot*>(expected), false);
        }
    }
    m_cacheMaintenanceCursor = cells == 0 ? 0 : (m_cacheMaintenanceCursor + (std::min)(blockBudget, cells)) % cells;
    LockFreeHashMapShrinkDecision decision;
    uint64_t time = now();
    uint64_t last = m_lastGrow.load();
    try
    {
        LockFreeHashMapShrinkContext context = { m_config.initialCapacity, m_grows.load(),
            m_capacity.load(), reusableFree(), last != 0, last != 0 && time >= last ? time - last : 0 };
        if (m_config.shrinkPolicy)
        {
            decision = m_config.shrinkPolicy(context);
        }
    }
    catch (...)
    {
        return LockFreeHashMapResult(LockFreeHashMapUserException);
    }
    size_t released = 0;
    size_t span = m_directorySpan.load();
    for (size_t scan = 0; scan < (std::min)(blockBudget, span); ++scan)
    {
        size_t index = m_maintenanceCursor++ % span;
        Block* entry = m_directory[index].load();
        if (reinterpret_cast<uintptr_t>(entry) & 1)
        {
            Block* item = reinterpret_cast<Block*>(reinterpret_cast<uintptr_t>(entry) & ~uintptr_t(1));
            if (item != nullptr && !protectedPointer(item))
            {
                // 只有此维护者摘除目录块，带标志的关闭项不再允许领取
                size_t count = item->m_count;
                destroyBlock(item);
                m_directory[index].store(nullptr);
                m_capacity.fetch_sub(count);
                m_blocks.fetch_sub(1);
                m_budget.fetch_sub(count);
                m_shrinkPending -= count;
                released += count;
            }
            continue;
        }
        Block* item = block(index, guard);
        if (item == nullptr || !item->m_ready.load())
        {
            continue;
        }
        uint64_t activity = item->m_activity.load();
        if ((activity & 65535) != 0 || item->m_idleSince == 0 || item->m_idleActivity != activity)
        {
            item->m_idleActivity = activity;
            item->m_idleSince = (activity & 65535) == 0 ? time : 0;
            continue;
        }
        size_t capacity = m_capacity.load() - m_shrinkPending;
        if (decision.idleMilliseconds == 0 || time < item->m_idleSince ||
            time - item->m_idleSince < decision.idleMilliseconds ||
            item->m_count > decision.shrinkCount || capacity < item->m_count ||
            capacity - item->m_count < m_config.initialCapacity)
        {
            continue;
        }
        if (item->m_activity.compare_exchange_strong(activity, activity | (UINT64_C(1) << 63)))
        {
            // 占用序号校验整个连续空闲区间，包括两次维护观察之间的短暂复用
            m_directory[index].store(reinterpret_cast<Block*>(reinterpret_cast<uintptr_t>(item) | 1));
            m_reusable.fetch_sub(item->m_count);
            decision.shrinkCount -= item->m_count;
            m_shrinkPending += item->m_count;
        }
    }
    guard.protect(4, nullptr);
    return LockFreeHashMapResult(LockFreeHashMapOk, false, released);
}

template <typename K, typename V>
size_t LockFreeHashMapStorage<K, V>::maximumSlots() const
{
    size_t bytes = (std::numeric_limits<size_t>::max)() - sizeof(*this) - m_ownerBytes;
    bytes -= m_config.directoryCapacity * (sizeof(std::atomic<Block*>) + sizeof(Block) + 64);
    bytes -= m_config.lookupCacheCapacity * sizeof(Cache) + 15;
    bytes -= m_config.protectionRecordCount * sizeof(LockFreeHashMapHazards);
    return bytes / sizeof(Slot);
}

template <typename K, typename V>
void LockFreeHashMapStorage<K, V>::destroyBlock(Block* item)
{
    if (item != nullptr)
    {
        for (size_t i = 0; i < item->m_count; ++i)
        {
            item->m_slots[i].~Slot();
        }
        delete[] item->m_slotAllocation;
        delete item;
    }
}

template <typename K, typename V>
typename LockFreeHashMapStorage<K, V>::Slot* LockFreeHashMapStorage<K, V>::cachedSlot(int32_t kind, LockFreeHashMapHazards& record)
{
    for (size_t i = 0; i < 8; ++i)
    {
        std::atomic<void*>& cell = record.cache(i);
        void* expected = cell.load(std::memory_order_relaxed);
        if (expected != nullptr && (reinterpret_cast<uintptr_t>(expected) & 1) == 0 &&
            cell.compare_exchange_strong(expected, nullptr))
        {
            Slot* slot = static_cast<Slot*>(expected);
            slot->m_state.store(kind, std::memory_order_relaxed);
            return slot;
        }
    }
    return nullptr;
}

template <typename K, typename V>
size_t LockFreeHashMapStorage<K, V>::reusableFree() const
{
    size_t result = m_reusable.load();
    size_t span = m_recordSpan.load();
    for (size_t i = 0; i < span; ++i)
    {
        for (size_t j = 0; j < 8; ++j)
        {
            void* value = m_records[i].cache(j).load();
            if (value != nullptr && (reinterpret_cast<uintptr_t>(value) & 1) == 0 &&
                result != (std::numeric_limits<size_t>::max)())
            {
                ++result;
            }
        }
    }
    return result;
}

template <typename K, typename V>
void LockFreeHashMapStorage<K, V>::deferValue(Slot* slot, LockFreeHashMapGuard& guard)
{
    // 顺序一致的源 CAS 唯一摘除此值；状态 7 由一个私有持有者或缓存格拥有，普通扫描不能领取，帮助者通过 CAS 转移所有权
    slot->m_state.store(7, std::memory_order_relaxed);
    bool queued = false;
    for (size_t i = 0; i < 8; ++i)
    {
        std::atomic<void*>& cell = guard.m_record->cache(i);
        if (cell.load(std::memory_order_relaxed) == nullptr)
        {
            cell.store(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(slot) | 1), std::memory_order_release);
            queued = true;
            break;
        }
    }
    if (!queued)
    {
        slot->m_state.store(4, std::memory_order_release);
    }
    if ((++guard.m_record->m_writeCount & 7) == 0 || m_capacity.load() < 16)
    {
        size_t budget = 8;
        reclaimCached(guard, budget, guard.m_record);
    }
    if ((guard.m_record->m_writeCount & 255) == 0)
    {
        reclaim(guard, 16);
    }
}

template <typename K, typename V>
uint32_t LockFreeHashMapStorage<K, V>::protectedValues(Slot** slots, size_t count) const
{
    uint32_t protectedMask = 0;
    size_t groups = (m_recordSpan.load() + 63) / 64;
    for (size_t group = 0; group < groups; ++group)
    {
        uint64_t bits = m_records[group].m_registered.load();
        while (bits != 0)
        {
            size_t index = group * 64 + firstRecord(bits);
            bits &= bits - 1;
            const LockFreeHashMapHazards& record = m_records[index];
            uintptr_t owner = record.m_used.load();
            for (size_t i = 0; i < count; ++i)
            {
                if (owner > 1 && (owner & ~uintptr_t(1)) == reinterpret_cast<uintptr_t>(slots[i]))
                {
                    protectedMask |= uint32_t(1) << i;
                }
            }
            if ((owner & 1) == 0)
            {
                continue;
            }
            for (size_t j = 0; j < 5; ++j)
            {
                void* pointer = record.m_pointer[j].load();
                for (size_t i = 0; pointer != nullptr && i < count; ++i)
                {
                    if (pointer == slots[i])
                    {
                        protectedMask |= uint32_t(1) << i;
                    }
                }
            }
        }
    }
    return protectedMask;
}

template <typename K, typename V>
size_t LockFreeHashMapStorage<K, V>::reclaimCached(LockFreeHashMapGuard& guard, size_t& budget, LockFreeHashMapHazards* record)
{
    size_t released = 0;
    size_t records = record != nullptr ? 1 : m_recordSpan.load();
    size_t start = record != nullptr || records == 0 ? 0 : m_cacheReclaimCursor.fetch_add(1) % records;
    for (size_t r = 0; r < records && budget != 0; ++r)
    {
        LockFreeHashMapHazards& source = record != nullptr ? *record : m_records[(start + r) % records];
        Slot* candidates[8] = {};
        size_t count = 0;
        for (size_t i = 0; i < 8 && budget != 0; ++i)
        {
            std::atomic<void*>& cell = source.cache(i);
            void* expected = cell.load();
            if ((reinterpret_cast<uintptr_t>(expected) & 1) != 0 && cell.compare_exchange_strong(expected, nullptr))
            {
                candidates[count++] = reinterpret_cast<Slot*>(reinterpret_cast<uintptr_t>(expected) & ~uintptr_t(1));
                --budget;
            }
        }
        if (count == 0)
        {
            continue;
        }
        // 先独占领取整批候选，再做顺序一致保护快照；即使其他槽复用，也不向本批次替换活对象
        uint32_t protectedMask = protectedValues(candidates, count);
        for (size_t i = 0; i < count; ++i)
        {
            Slot* slot = candidates[i];
            if ((protectedMask & (uint32_t(1) << i)) != 0)
            {
                slot->m_state.store(4, std::memory_order_release);
            }
            else
            {
                slot->value()->~V();
                release(slot, false, &guard);
                ++released;
            }
        }
    }
    return released;
}