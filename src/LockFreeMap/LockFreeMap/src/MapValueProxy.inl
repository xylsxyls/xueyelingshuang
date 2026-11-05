#ifndef __MAP_VALUE_PROXY_INL__
#define __MAP_VALUE_PROXY_INL__

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
MapValueProxy<KeyType, ValueType, LessType, MaxLevel>::MapValueProxy(LockFreeMap<KeyType, ValueType, LessType, MaxLevel>* map, const KeyType& key) :
m_map(map),
m_key(key)
{

}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
MapValueProxy<KeyType, ValueType, LessType, MaxLevel>::MapValueProxy(const MapValueProxy& other) :
m_map(other.m_map),
m_key(other.m_key)
{

}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
MapValueProxy<KeyType, ValueType, LessType, MaxLevel>::~MapValueProxy()
{

}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
MapValueProxy<KeyType, ValueType, LessType, MaxLevel>& MapValueProxy<KeyType, ValueType, LessType, MaxLevel>::operator=(const ValueType& value)
{
    if (m_map == nullptr)
    {
        throw LockFreeMapError(LOCK_FREE_MAP_INVALID);
    }
    LockFreeMapStatus status;
    do
    {
        status = m_map->lockFreeMapSet(m_key, value);
    }
    while (status == LOCK_FREE_MAP_CONFLICT);
    if (status != LOCK_FREE_MAP_COMMITTED)
    {
        throw LockFreeMapError(status);
    }
    return *this;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
MapValueProxy<KeyType, ValueType, LessType, MaxLevel>& MapValueProxy<KeyType, ValueType, LessType, MaxLevel>::operator=(const MapValueProxy& other)
{
    if (this == &other)
    {
        return *this;
    }
    if (other.m_map == nullptr)
    {
        throw LockFreeMapError(LOCK_FREE_MAP_INVALID);
    }
    ValueType otherValue = ValueType();
    LockFreeMapToken token;
    const LockFreeMapStatus status = other.m_map->lockFreeMapGet(other.m_key, otherValue, token);
    if (status != LOCK_FREE_MAP_OK && status != LOCK_FREE_MAP_NOT_FOUND)
    {
        throw LockFreeMapError(status);
    }
    return operator=(otherValue);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
MapValueProxy<KeyType, ValueType, LessType, MaxLevel>::operator ValueType() const
{
    return value(ValueType());
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool MapValueProxy<KeyType, ValueType, LessType, MaxLevel>::get(ValueType& value) const
{
    if (m_map == nullptr)
    {
        return false;
    }
    return m_map->get(m_key, value);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
ValueType MapValueProxy<KeyType, ValueType, LessType, MaxLevel>::value(const ValueType& defaultValue) const
{
    ValueType result = defaultValue;
    if (m_map != nullptr)
    {
        m_map->get(m_key, result);
    }
    return result;
}

#endif // __MAP_VALUE_PROXY_INL__