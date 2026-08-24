#ifndef __MAP_NODE_INL__
#define __MAP_NODE_INL__

template <typename KeyType, typename ValueType>
HashMapNode<KeyType, ValueType>::HashMapNode(const KeyType& key, const ValueType& value, uint64_t generation) :
m_key(key),
m_value(new (std::nothrow) ValueType(value)),
m_next(nullptr),
m_generation(generation)
{

}

template <typename KeyType, typename ValueType>
HashMapNode<KeyType, ValueType>::HashMapNode(const KeyType& key, uint64_t generation) :
m_key(key),
m_value(nullptr),
m_next(nullptr),
m_generation(generation)
{

}

template <typename KeyType, typename ValueType>
HashMapNode<KeyType, ValueType>::~HashMapNode()
{
    if (m_value != nullptr)
    {
        delete m_value;
        m_value = nullptr;
    }
}

template <typename KeyType, typename ValueType>
bool HashMapNode<KeyType, ValueType>::hasValue() const
{
    return m_value != nullptr;
}

#endif // __MAP_NODE_INL__