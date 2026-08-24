#ifndef __MAP_VALUE_PROXY_INL__
#define __MAP_VALUE_PROXY_INL__

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapValueProxy<KeyType, ValueType, HashType, EqualType>::HashMapValueProxy(LockFreeHashMap<KeyType, ValueType, HashType, EqualType>* map, const KeyType& key) :
m_map(map),
m_key(key)
{

}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapValueProxy<KeyType, ValueType, HashType, EqualType>::HashMapValueProxy(const HashMapValueProxy& other) :
m_map(other.m_map),
m_key(other.m_key)
{

}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapValueProxy<KeyType, ValueType, HashType, EqualType>::~HashMapValueProxy()
{

}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapValueProxy<KeyType, ValueType, HashType, EqualType>& HashMapValueProxy<KeyType, ValueType, HashType, EqualType>::operator=(const ValueType& value)
{
    if (m_map != nullptr)
    {
        m_map->set(m_key, value);
    }
    return *this;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapValueProxy<KeyType, ValueType, HashType, EqualType>& HashMapValueProxy<KeyType, ValueType, HashType, EqualType>::operator=(const HashMapValueProxy& other)
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

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapValueProxy<KeyType, ValueType, HashType, EqualType>::operator ValueType() const
{
    return value(ValueType());
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool HashMapValueProxy<KeyType, ValueType, HashType, EqualType>::get(ValueType& value) const
{
    if (m_map == nullptr)
    {
        return false;
    }
    return m_map->get(m_key, value);
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
ValueType HashMapValueProxy<KeyType, ValueType, HashType, EqualType>::value(const ValueType& defaultValue) const
{
    ValueType result = defaultValue;
    if (m_map != nullptr)
    {
        m_map->get(m_key, result);
    }
    return result;
}

#endif // __MAP_VALUE_PROXY_INL__