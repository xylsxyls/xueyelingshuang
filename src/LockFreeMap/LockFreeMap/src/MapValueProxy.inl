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
    if (m_map != nullptr)
    {
        m_map->set(m_key, value);
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
    ValueType otherValue = other.value(ValueType());
    if (m_map != nullptr)
    {
        m_map->set(m_key, otherValue);
    }
    return *this;
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