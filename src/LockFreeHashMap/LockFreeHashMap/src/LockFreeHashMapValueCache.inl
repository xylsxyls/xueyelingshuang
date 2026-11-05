#pragma once

inline LockFreeHashMapValueCacheEntry::LockFreeHashMapValueCacheEntry() :
m_stamp(0),
m_generation(0),
m_key(0),
m_value(0),
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

template <typename K, typename V, typename E>
LockFreeHashMapValueCache<K, V, E, true>::LockFreeHashMapValueCache() :
m_entries(nullptr),
m_count(0)
{

}

template <typename K, typename V, typename E>
LockFreeHashMapValueCache<K, V, E, true>::~LockFreeHashMapValueCache()
{
    delete[] m_entries;
}

template <typename K, typename V, typename E>
LockFreeHashMapStatus LockFreeHashMapValueCache<K, V, E, true>::initialize(size_t count)
{
    if (m_entries != nullptr || (count != 0 && ((count & (count - 1)) != 0 ||
        count > (std::numeric_limits<size_t>::max)() / sizeof(LockFreeHashMapValueCacheEntry))))
    {
        return LockFreeHashMapInvalidArgument;
    }
    if (count == 0)
    {
        return LockFreeHashMapOk;
    }
    LockFreeHashMapValueCacheEntry check;
    if (!check.m_stamp.is_lock_free() || !check.m_publishing.is_lock_free())
    {
        return LockFreeHashMapUnsupported;
    }
    try
    {
        m_entries = new (std::nothrow) LockFreeHashMapValueCacheEntry[count];
        if (m_entries == nullptr)
        {
            return LockFreeHashMapNoMemory;
        }
        m_count = count;
        return LockFreeHashMapOk;
    }
    catch (...)
    {
        return LockFreeHashMapNoMemory;
    }
}

template <typename K, typename V, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMapValueCache<K, V, E, true>::lookup(const K& key, uint64_t generation, V* value) const
{
    if (m_count == 0 || generation == 0)
    {
        return false;
    }
    LockFreeHashMapValueCacheEntry& entry = m_entries[static_cast<size_t>(static_cast<uint64_t>(key) ^ (static_cast<uint64_t>(key) >> 32)) & (m_count - 1)];
    uint64_t before = entry.m_stamp.load();
    if ((before & 1) == 0)
    {
        return false;
    }
    uint64_t expectedKey = static_cast<uint64_t>(key);
    uint64_t observedKey = entry.m_key.load(std::memory_order_acquire);
    uint64_t observedGeneration = entry.m_generation.load(std::memory_order_acquire);
    uint64_t observedValue = entry.m_value.load(std::memory_order_acquire);
    if (observedKey != expectedKey || observedGeneration != generation || entry.m_stamp.load() != before)
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
        m_entries[static_cast<size_t>(static_cast<uint64_t>(key) ^ (static_cast<uint64_t>(key) >> 32)) & (m_count - 1)].m_stamp.store(stamp);
    }
}

template <typename K, typename V, typename E>
void LockFreeHashMapValueCache<K, V, E, true>::publish(const K& key, const V& value, uint64_t generation, uint64_t version)
{
    if (m_count == 0 || version == 0 || version > ((std::numeric_limits<uint64_t>::max)() >> 1))
    {
        return;
    }
    LockFreeHashMapValueCacheEntry& entry = m_entries[static_cast<size_t>(static_cast<uint64_t>(key) ^ (static_cast<uint64_t>(key) >> 32)) & (m_count - 1)];
    uint64_t expected = version << 1;
    bool idle = false;
    if (entry.m_stamp.load() != expected || !entry.m_publishing.compare_exchange_strong(idle, true))
    {
        return;
    }
    // 失效身份可能在取得发布权前被别人改变，必须再次确认，避免覆盖仍然有效的其他副本
    if (entry.m_stamp.load() == expected)
    {
        uint64_t keyBits = static_cast<uint64_t>(key), valueBits = 0;
        std::memcpy(&valueBits, &value, sizeof(V));
        entry.m_key.store(keyBits, std::memory_order_release);
        entry.m_generation.store(generation, std::memory_order_release);
        entry.m_value.store(valueBits, std::memory_order_release);
        entry.m_stamp.compare_exchange_strong(expected, expected | 1);
    }
    entry.m_publishing.store(false, std::memory_order_release);
}

template <typename K, typename V, typename E>
void LockFreeHashMapValueCache<K, V, E, true>::swap(LockFreeHashMapValueCache& other)
{
    std::swap(m_entries, other.m_entries);
    std::swap(m_count, other.m_count);
}

template <typename K, typename V, typename E>
size_t LockFreeHashMapValueCache<K, V, E, true>::count() const
{
    return m_count;
}