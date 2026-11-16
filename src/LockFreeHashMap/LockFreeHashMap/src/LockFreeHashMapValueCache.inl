#pragma once

inline LockFreeHashMapValueCacheEntry::LockFreeHashMapValueCacheEntry() :
m_stamp(0),
m_generation(0),
m_key(0),
m_value(0),
m_publishing(false)
{

}

inline LockFreeHashMapPackedValueCacheEntry::LockFreeHashMapPackedValueCacheEntry() :
m_stamp(0),
m_generation(0),
m_key(0),
m_publishing(false)
{

}

template <typename K, typename V, typename E, bool Enabled>
LockFreeHashMapStatus LockFreeHashMapValueCache<K, V, E, Enabled>::initialize(size_t count)
{
    (void)count;
    return LockFreeHashMapOk;
}

template <typename K, typename V, typename E, bool Enabled>
bool LockFreeHashMapValueCache<K, V, E, Enabled>::lookup(const K& key, uint64_t generation, V* value) const
{
    (void)key;
    (void)generation;
    (void)value;
    return false;
}

template <typename K, typename V, typename E, bool Enabled>
bool LockFreeHashMapValueCache<K, V, E, Enabled>::lookup(const K& key, const std::atomic<uint64_t>& generation, V* value) const
{
    (void)key;
    (void)generation;
    (void)value;
    return false;
}

template <typename K, typename V, typename E, bool Enabled>
void LockFreeHashMapValueCache<K, V, E, Enabled>::invalidate(const K& key, uint64_t version)
{
    (void)key;
    (void)version;
}

template <typename K, typename V, typename E, bool Enabled>
void LockFreeHashMapValueCache<K, V, E, Enabled>::publish(const K& key, const V& value, uint64_t generation, uint64_t version)
{
    (void)key;
    (void)value;
    (void)generation;
    (void)version;
}

template <typename K, typename V, typename E, bool Enabled>
void LockFreeHashMapValueCache<K, V, E, Enabled>::swap(LockFreeHashMapValueCache& other)
{
    (void)other;
}

template <typename K, typename V, typename E, bool Enabled>
size_t LockFreeHashMapValueCache<K, V, E, Enabled>::count() const
{
    return 0;
}

template <typename K, typename V, typename E, bool Enabled>
size_t LockFreeHashMapValueCache<K, V, E, Enabled>::bytes() const
{
    return 0;
}

template <typename K, typename V, typename E>
LockFreeHashMapValueCache<K, V, E, true>::LockFreeHashMapValueCache() :
m_entries(nullptr),
m_count(0),
m_allocation(nullptr)
{

}

template <typename K, typename V, typename E>
LockFreeHashMapValueCache<K, V, E, true>::~LockFreeHashMapValueCache()
{
    for (size_t i = 0; i < m_count; ++i)
    {
        m_entries[i].~Entry();
    }
    delete[] m_allocation;
}

template <typename K, typename V, typename E>
LockFreeHashMapStatus LockFreeHashMapValueCache<K, V, E, true>::initialize(size_t count)
{
    if (m_entries != nullptr || (count != 0 && ((count & (count - 1)) != 0 ||
        count > ((std::numeric_limits<size_t>::max)() - 63) / sizeof(Entry))))
    {
        return LockFreeHashMapInvalidArgument;
    }
    if (count == 0)
    {
        return LockFreeHashMapOk;
    }
    Entry check;
    if (!check.m_stamp.is_lock_free() || !check.m_publishing.is_lock_free())
    {
        return LockFreeHashMapUnsupported;
    }
    try
    {
        m_allocation = new (std::nothrow) unsigned char[count * sizeof(Entry) + 63];
        if (m_allocation == nullptr)
        {
            return LockFreeHashMapNoMemory;
        }
        // 窄格每两个占一个64字节区间；起点对齐避免格跨区间，减少相邻键区间的边界伪共享。
        uintptr_t address = reinterpret_cast<uintptr_t>(m_allocation);
        size_t adjustment = static_cast<size_t>((64 - (address & 63)) & 63);
        m_entries = reinterpret_cast<Entry*>(m_allocation + adjustment);
        for (; m_count < count; ++m_count)
        {
            new (m_entries + m_count) Entry();
        }
        return LockFreeHashMapOk;
    }
    catch (...)
    {
        for (size_t i = 0; i < m_count; ++i)
        {
            m_entries[i].~Entry();
        }
        delete[] m_allocation;
        m_allocation = nullptr;
        m_entries = nullptr;
        m_count = 0;
        return LockFreeHashMapNoMemory;
    }
}

template <typename K, typename V, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMapValueCache<K, V, E, true>::lookup(const K& key, uint64_t generation, V* value) const
{
    size_t count = m_count;
    return count != 0 && lookupAtGeneration(key, generation, value, count);
}

template <typename K, typename V, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMapValueCache<K, V, E, true>::lookup(const K& key, const std::atomic<uint64_t>& generation, V* value) const
{
    size_t count = m_count;
    return count != 0 && lookupAtGeneration(key, generation, value, count);
}

template <typename K, typename V, typename E>
LOCKFREEHASHMAP_FORCE_INLINE size_t LockFreeHashMapValueCache<K, V, E, true>::keyBits(const K& key)
{
    if (sizeof(K) <= sizeof(uint32_t))
    {
        return static_cast<size_t>(key);
    }
    uint64_t bits = static_cast<uint64_t>(key);
    return static_cast<size_t>(bits ^ (bits >> 32));
}

template <typename K, typename V, typename E>
template <typename GenerationType>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMapValueCache<K, V, E, true>::lookupAtGeneration(const K& key, const GenerationType& generation, V* value, size_t count) const
{
    Entry& entry = m_entries[keyBits(key) & (count - 1)];
    uint64_t before = entry.m_stamp.load();
    if ((before & 1) == 0)
    {
        return false;
    }
    uint64_t observedKey = entry.m_key.load(std::memory_order_acquire);
    if (kPacked ? static_cast<uint32_t>(observedKey) != static_cast<uint32_t>(key) :
        observedKey != static_cast<uint64_t>(key))
    {
        return false;
    }
    // 缓存碰撞先排除异键，避免额外读取独立值字段；窄键值共用一个原子字，仍验证代次与末次身份。
    uint64_t observedGeneration = entry.m_generation.load(std::memory_order_acquire);
    ValueWord observedValue = loadValue(entry, observedKey, value != nullptr);
    // 原子整数的标准转换等价于顺序一致的load；缓存未命中时省去这次全局代次读取。
    uint64_t currentGeneration = static_cast<uint64_t>(generation);
    if (currentGeneration == 0 || observedGeneration != currentGeneration || entry.m_stamp.load() != before)
    {
        return false;
    }
    if (value != nullptr)
    {
        std::memcpy(value, &observedValue, sizeof(V));
    }
    return true;
}

template <typename K, typename V, typename E>
void LockFreeHashMapValueCache<K, V, E, true>::invalidate(const K& key, uint64_t version)
{
    if (m_count != 0)
    {
        uint64_t stamp = version <= ((std::numeric_limits<uint64_t>::max)() >> 1) ? version << 1 : 0;
        m_entries[keyBits(key) & (m_count - 1)].m_stamp.store(stamp);
    }
}

template <typename K, typename V, typename E>
void LockFreeHashMapValueCache<K, V, E, true>::publish(const K& key, const V& value, uint64_t generation, uint64_t version)
{
    if (m_count == 0 || version == 0 || version > ((std::numeric_limits<uint64_t>::max)() >> 1))
    {
        return;
    }
    Entry& entry = m_entries[keyBits(key) & (m_count - 1)];
    uint64_t expected = version << 1;
    bool idle = false;
    if (entry.m_stamp.load() != expected || !entry.m_publishing.compare_exchange_strong(idle, true))
    {
        return;
    }
    // 失效身份可能在取得发布权前被别人改变，必须再次确认，避免覆盖仍然有效的其他副本
    if (entry.m_stamp.load() == expected)
    {
        storeValue(entry, key, value);
        entry.m_generation.store(generation, std::memory_order_release);
        entry.m_stamp.compare_exchange_strong(expected, expected | 1);
    }
    entry.m_publishing.store(false, std::memory_order_release);
}

template <typename K, typename V, typename E>
void LockFreeHashMapValueCache<K, V, E, true>::swap(LockFreeHashMapValueCache& other)
{
    std::swap(m_entries, other.m_entries);
    std::swap(m_count, other.m_count);
    std::swap(m_allocation, other.m_allocation);
}

template <typename K, typename V, typename E>
size_t LockFreeHashMapValueCache<K, V, E, true>::count() const
{
    return m_count;
}

template <typename K, typename V, typename E>
size_t LockFreeHashMapValueCache<K, V, E, true>::bytes() const
{
    return m_count == 0 ? 0 : m_count * sizeof(Entry) + 63;
}

template <typename K, typename V, typename E>
LOCKFREEHASHMAP_FORCE_INLINE typename LockFreeHashMapValueCache<K, V, E, true>::ValueWord
LockFreeHashMapValueCache<K, V, E, true>::loadValue(const LockFreeHashMapPackedValueCacheEntry& entry, uint64_t observedKey, bool copy)
{
    (void)entry;
    (void)copy;
    return static_cast<ValueWord>(observedKey >> 32);
}

template <typename K, typename V, typename E>
LOCKFREEHASHMAP_FORCE_INLINE typename LockFreeHashMapValueCache<K, V, E, true>::ValueWord
LockFreeHashMapValueCache<K, V, E, true>::loadValue(const LockFreeHashMapValueCacheEntry& entry, uint64_t observedKey, bool copy)
{
    (void)observedKey;
    return copy ? static_cast<ValueWord>(entry.m_value.load(std::memory_order_acquire)) : 0;
}

template <typename K, typename V, typename E>
LOCKFREEHASHMAP_FORCE_INLINE void LockFreeHashMapValueCache<K, V, E, true>::storeValue(LockFreeHashMapPackedValueCacheEntry& entry, const K& key, const V& value)
{
    ValueWord bits = 0;
    std::memcpy(&bits, &value, sizeof(V));
    entry.m_key.store(static_cast<uint32_t>(key) | (static_cast<uint64_t>(bits) << 32), std::memory_order_release);
}

template <typename K, typename V, typename E>
LOCKFREEHASHMAP_FORCE_INLINE void LockFreeHashMapValueCache<K, V, E, true>::storeValue(LockFreeHashMapValueCacheEntry& entry, const K& key, const V& value)
{
    ValueWord bits = 0;
    std::memcpy(&bits, &value, sizeof(V));
    entry.m_key.store(static_cast<uint64_t>(key), std::memory_order_release);
    entry.m_value.store(bits, std::memory_order_release);
}