#ifndef __HASH_MAP_VALUE_PROXY_INL__
#define __HASH_MAP_VALUE_PROXY_INL__

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
    LockFreeHashMapResult result = m_map == nullptr ? LockFreeHashMapResult(LockFreeHashMapInvalidArgument) :
        m_map->trySet(m_key, value);
    if (!result.m_committed)
    {
        // 下标语法没有返回错误码的位置，失败必须显式交给调用方
        throw LockFreeHashMapAssignmentError(result.m_status);
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
    if (other.m_map == nullptr)
    {
        throw LockFreeHashMapAssignmentError(LockFreeHashMapInvalidArgument);
    }
    LockFreeHashMapSnapshot<ValueType> snapshot;
    LockFreeHashMapResult result = other.m_map->find(other.m_key, snapshot);
    if (result.m_status == LockFreeHashMapNotFound)
    {
        // 保留旧接口对不存在的来源键使用默认值的约定
        return operator=(ValueType());
    }
    if (result.m_status != LockFreeHashMapOk)
    {
        throw LockFreeHashMapAssignmentError(result.m_status);
    }
    return operator=(*snapshot.m_value);
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

#endif // __HASH_MAP_VALUE_PROXY_INL__