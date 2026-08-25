#ifndef __MAP_NODE_INL__
#define __MAP_NODE_INL__

template <typename KeyType, typename ValueType, size_t MaxLevel>
MapNode<KeyType, ValueType, MaxLevel>::MapNode(const KeyType& key, size_t level) :
m_key(key),
m_value(nullptr),
m_retainedValues(nullptr),
m_level(level)
{
    if (m_level == 0)
    {
        m_level = 1;
    }
    if (m_level > MaxLevel)
    {
        m_level = MaxLevel;
    }
    for (size_t i = 0; i < MaxLevel; ++i)
    {
        m_next[i].store(nullptr, std::memory_order_relaxed);
    }
}

template <typename KeyType, typename ValueType, size_t MaxLevel>
MapNode<KeyType, ValueType, MaxLevel>::~MapNode()
{
    releaseValueHolders();
}

template <typename KeyType, typename ValueType, size_t MaxLevel>
bool MapNode<KeyType, ValueType, MaxLevel>::appendRetainedValue(MapValueHolder<ValueType>* valueHolder)
{
    if (valueHolder == nullptr)
    {
        return false;
    }

    MapValueHolder<ValueType>* head = m_retainedValues.load(std::memory_order_acquire);
    for (;;)
    {
        valueHolder->m_nextRetained.store(head, std::memory_order_relaxed);
        if (m_retainedValues.compare_exchange_strong(head, valueHolder, std::memory_order_release, std::memory_order_acquire))
        {
            return true;
        }
    }
}

template <typename KeyType, typename ValueType, size_t MaxLevel>
bool MapNode<KeyType, ValueType, MaxLevel>::visibleValue(uint64_t generation, ValueType& value) const
{
    MapValueHolder<ValueType>* valueHolder = visibleHolder(generation);
    if (valueHolder == nullptr)
    {
        return false;
    }
    try
    {
        value = valueHolder->m_value;
    }
    catch (...)
    {
        return false;
    }
    return true;
}

template <typename KeyType, typename ValueType, size_t MaxLevel>
bool MapNode<KeyType, ValueType, MaxLevel>::hasVisibleValue(uint64_t generation) const
{
    return visibleHolder(generation) != nullptr;
}

template <typename KeyType, typename ValueType, size_t MaxLevel>
MapValueHolder<ValueType>* MapNode<KeyType, ValueType, MaxLevel>::visibleHolder(uint64_t generation) const
{
    MapValueHolder<ValueType>* valueHolder = m_value.load(std::memory_order_acquire);
    if (!isVisibleHolder(valueHolder, generation))
    {
        return nullptr;
    }
    return valueHolder;
}

template <typename KeyType, typename ValueType, size_t MaxLevel>
bool MapNode<KeyType, ValueType, MaxLevel>::isVisibleHolder(MapValueHolder<ValueType>* valueHolder, uint64_t generation) const
{
    return valueHolder != nullptr && valueHolder->m_generation == generation;
}

template <typename KeyType, typename ValueType, size_t MaxLevel>
void MapNode<KeyType, ValueType, MaxLevel>::releaseValueHolders()
{
    MapValueHolder<ValueType>* valueHolder = m_retainedValues.exchange(nullptr, std::memory_order_acq_rel);
    while (valueHolder != nullptr)
    {
        MapValueHolder<ValueType>* next = valueHolder->m_nextRetained.load(std::memory_order_relaxed);
        delete valueHolder;
        valueHolder = next;
    }
    m_value.store(nullptr, std::memory_order_release);
}

#endif // __MAP_NODE_INL__