#ifndef __LOCK_FREE_MAP_INL__
#define __LOCK_FREE_MAP_INL__

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::LockFreeMap() :
m_less(LessType()),
m_generation(1),
m_retainedNodeCount(0),
m_retainedValueCount(0),
m_randomSeed(0x9E3779B9u)
{
    static_assert(MaxLevel > 0, "LockFreeMap MaxLevel must be greater than 0");
    for (size_t i = 0; i < MaxLevel; ++i)
    {
        m_heads[i].store(nullptr, std::memory_order_relaxed);
    }
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::~LockFreeMap()
{
    releaseAllNodes();
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
MapValueProxy<KeyType, ValueType, LessType, MaxLevel> LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::operator[](const KeyType& key)
{
    return MapValueProxy<KeyType, ValueType, LessType, MaxLevel>(this, key);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::set(const KeyType& key, const ValueType& value)
{
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    MapValueHolder<ValueType>* valueHolder = createValueHolder(value, generation);
    if (valueHolder == nullptr)
    {
        return false;
    }

    MapNode<KeyType, ValueType, MaxLevel>* node = nullptr;
    if (!findOrCreateNode(key, &node) || node == nullptr)
    {
        releaseUnusedValueHolder(valueHolder);
        return false;
    }

    generation = m_generation.load(std::memory_order_acquire);
    valueHolder->m_generation = generation;
    node->appendRetainedValue(valueHolder);
    node->m_value.exchange(valueHolder, std::memory_order_acq_rel);
    return true;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::insert(const KeyType& key, const ValueType& value)
{
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    MapValueHolder<ValueType>* valueHolder = createValueHolder(value, generation);
    if (valueHolder == nullptr)
    {
        return false;
    }

    for (;;)
    {
        MapNode<KeyType, ValueType, MaxLevel>* node = nullptr;
        if (!findOrCreateNode(key, &node) || node == nullptr)
        {
            releaseUnusedValueHolder(valueHolder);
            return false;
        }

        generation = m_generation.load(std::memory_order_acquire);
        valueHolder->m_generation = generation;
        MapValueHolder<ValueType>* currentValue = node->m_value.load(std::memory_order_acquire);
        if (node->isVisibleHolder(currentValue, generation))
        {
            releaseUnusedValueHolder(valueHolder);
            return false;
        }
        if (node->m_value.compare_exchange_strong(currentValue, valueHolder, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            node->appendRetainedValue(valueHolder);
            return true;
        }
    }
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::erase(const KeyType& key)
{
    for (;;)
    {
        MapNode<KeyType, ValueType, MaxLevel>* node = nullptr;
        if (!findNode(key, &node) || node == nullptr)
        {
            return false;
        }

        uint64_t generation = m_generation.load(std::memory_order_acquire);
        MapValueHolder<ValueType>* currentValue = node->m_value.load(std::memory_order_acquire);
        if (!node->isVisibleHolder(currentValue, generation))
        {
            return false;
        }
        if (node->m_value.compare_exchange_strong(currentValue, nullptr, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            return true;
        }
    }
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::remove(const KeyType& key)
{
    return erase(key);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::get(const KeyType& key, ValueType& value) const
{
    MapNode<KeyType, ValueType, MaxLevel>* node = nullptr;
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    if (!findNode(key, &node) || node == nullptr)
    {
        return false;
    }
    return node->visibleValue(generation, value);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
ValueType LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::value(const KeyType& key, const ValueType& defaultValue) const
{
    ValueType result = defaultValue;
    get(key, result);
    return result;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::contains(const KeyType& key) const
{
    MapNode<KeyType, ValueType, MaxLevel>* node = nullptr;
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    if (!findNode(key, &node) || node == nullptr)
    {
        return false;
    }
    return node->hasVisibleValue(generation);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
size_t LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::count(const KeyType& key) const
{
    return contains(key) ? 1 : 0;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
size_t LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::count() const
{
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    size_t result = 0;
    MapNode<KeyType, ValueType, MaxLevel>* current = m_heads[0].load(std::memory_order_acquire);
    while (current != nullptr)
    {
        if (current->hasVisibleValue(generation))
        {
            ++result;
        }
        current = current->m_next[0].load(std::memory_order_acquire);
    }
    return result;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
size_t LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::size() const
{
    return count();
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::empty() const
{
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    MapNode<KeyType, ValueType, MaxLevel>* current = m_heads[0].load(std::memory_order_acquire);
    while (current != nullptr)
    {
        if (current->hasVisibleValue(generation))
        {
            return false;
        }
        current = current->m_next[0].load(std::memory_order_acquire);
    }
    return true;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
void LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::clear()
{
    m_generation.fetch_add(1, std::memory_order_acq_rel);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
void LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::clearAndFreeMemory()
{
    releaseAllNodes();
    m_generation.fetch_add(1, std::memory_order_acq_rel);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
template <typename FunctionType>
size_t LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::forEach(FunctionType function) const
{
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    size_t result = 0;
    MapNode<KeyType, ValueType, MaxLevel>* current = m_heads[0].load(std::memory_order_acquire);
    while (current != nullptr)
    {
        MapValueHolder<ValueType>* valueHolder = current->visibleHolder(generation);
        if (valueHolder != nullptr)
        {
            try
            {
                function(current->m_key, valueHolder->m_value);
            }
            catch (...)
            {
                return result;
            }
            ++result;
        }
        current = current->m_next[0].load(std::memory_order_acquire);
    }
    return result;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
std::vector<KeyType> LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::keys() const
{
    std::vector<KeyType> result;
    try
    {
        result.reserve(count());
        forEach([&result](const KeyType& key, const ValueType&) -> void
        {
            result.push_back(key);
        });
    }
    catch (...)
    {
        result.clear();
    }
    return result;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
std::vector<ValueType> LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::values() const
{
    std::vector<ValueType> result;
    try
    {
        result.reserve(count());
        forEach([&result](const KeyType&, const ValueType& value) -> void
        {
            result.push_back(value);
        });
    }
    catch (...)
    {
        result.clear();
    }
    return result;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
std::vector<std::pair<KeyType, ValueType>> LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::toVector() const
{
    std::vector<std::pair<KeyType, ValueType>> result;
    try
    {
        result.reserve(count());
        forEach([&result](const KeyType& key, const ValueType& value) -> void
        {
            result.push_back(std::make_pair(key, value));
        });
    }
    catch (...)
    {
        result.clear();
    }
    return result;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::first(KeyType& key, ValueType& value) const
{
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    MapNode<KeyType, ValueType, MaxLevel>* current = m_heads[0].load(std::memory_order_acquire);
    while (current != nullptr)
    {
        MapValueHolder<ValueType>* valueHolder = current->visibleHolder(generation);
        if (valueHolder != nullptr)
        {
            try
            {
                key = current->m_key;
                value = valueHolder->m_value;
            }
            catch (...)
            {
                return false;
            }
            return true;
        }
        current = current->m_next[0].load(std::memory_order_acquire);
    }
    return false;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::last(KeyType& key, ValueType& value) const
{
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    bool found = false;
    MapNode<KeyType, ValueType, MaxLevel>* current = m_heads[0].load(std::memory_order_acquire);
    while (current != nullptr)
    {
        MapValueHolder<ValueType>* valueHolder = current->visibleHolder(generation);
        if (valueHolder != nullptr)
        {
            try
            {
                key = current->m_key;
                value = valueHolder->m_value;
            }
            catch (...)
            {
                return false;
            }
            found = true;
        }
        current = current->m_next[0].load(std::memory_order_acquire);
    }
    return found;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::lowerBound(const KeyType& key, KeyType& foundKey, ValueType& value) const
{
    MapNode<KeyType, ValueType, MaxLevel>* successors[MaxLevel];
    if (!findPosition(key, nullptr, successors, nullptr))
    {
        return false;
    }

    uint64_t generation = m_generation.load(std::memory_order_acquire);
    MapNode<KeyType, ValueType, MaxLevel>* current = successors[0];
    while (current != nullptr)
    {
        MapValueHolder<ValueType>* valueHolder = current->visibleHolder(generation);
        if (valueHolder != nullptr)
        {
            try
            {
                foundKey = current->m_key;
                value = valueHolder->m_value;
            }
            catch (...)
            {
                return false;
            }
            return true;
        }
        current = current->m_next[0].load(std::memory_order_acquire);
    }
    return false;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::upperBound(const KeyType& key, KeyType& foundKey, ValueType& value) const
{
    MapNode<KeyType, ValueType, MaxLevel>* successors[MaxLevel];
    if (!findPosition(key, nullptr, successors, nullptr))
    {
        return false;
    }

    uint64_t generation = m_generation.load(std::memory_order_acquire);
    MapNode<KeyType, ValueType, MaxLevel>* current = successors[0];
    while (current != nullptr)
    {
        bool isUpper = false;
        if (!keyLess(key, current->m_key, isUpper))
        {
            return false;
        }
        if (isUpper)
        {
            MapValueHolder<ValueType>* valueHolder = current->visibleHolder(generation);
            if (valueHolder != nullptr)
            {
                try
                {
                    foundKey = current->m_key;
                    value = valueHolder->m_value;
                }
                catch (...)
                {
                    return false;
                }
                return true;
            }
        }
        current = current->m_next[0].load(std::memory_order_acquire);
    }
    return false;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
size_t LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::maxLevel() const
{
    return MaxLevel;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
size_t LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::retainedNodeCount() const
{
    return m_retainedNodeCount.load(std::memory_order_acquire);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
size_t LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::retainedValueCount() const
{
    return m_retainedValueCount.load(std::memory_order_acquire);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::keyLess(const KeyType& left, const KeyType& right, bool& result) const
{
    try
    {
        result = m_less(left, right);
    }
    catch (...)
    {
        result = false;
        return false;
    }
    return true;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::keyEqual(const KeyType& left, const KeyType& right, bool& result) const
{
    bool leftLessRight = false;
    bool rightLessLeft = false;
    if (!keyLess(left, right, leftLessRight))
    {
        result = false;
        return false;
    }
    if (!keyLess(right, left, rightLessLeft))
    {
        result = false;
        return false;
    }
    result = !leftLessRight && !rightLessLeft;
    return true;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
size_t LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::randomLevel()
{
    uint32_t value = m_randomSeed.fetch_add(0x9E3779B9u, std::memory_order_acq_rel);
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;

    size_t level = 1;
    while (level < MaxLevel && (value & 0x3u) == 0)
    {
        ++level;
        value >>= 2;
        if (value == 0)
        {
            value = m_randomSeed.fetch_add(0x85EBCA6Bu, std::memory_order_acq_rel);
        }
    }
    return level;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::findPosition(const KeyType& key,
                                                                       MapNode<KeyType, ValueType, MaxLevel>** predecessors,
                                                                       MapNode<KeyType, ValueType, MaxLevel>** successors,
                                                                       MapNode<KeyType, ValueType, MaxLevel>** foundNode) const
{
    if (foundNode != nullptr)
    {
        *foundNode = nullptr;
    }

    MapNode<KeyType, ValueType, MaxLevel>* predecessor = nullptr;
    for (size_t levelIndex = MaxLevel; levelIndex > 0; --levelIndex)
    {
        size_t level = levelIndex - 1;
        MapNode<KeyType, ValueType, MaxLevel>* current = nullptr;
        if (predecessor == nullptr)
        {
            current = m_heads[level].load(std::memory_order_acquire);
        }
        else
        {
            current = predecessor->m_next[level].load(std::memory_order_acquire);
        }

        while (current != nullptr)
        {
            bool currentLessKey = false;
            if (!keyLess(current->m_key, key, currentLessKey))
            {
                return false;
            }
            if (!currentLessKey)
            {
                break;
            }
            predecessor = current;
            current = current->m_next[level].load(std::memory_order_acquire);
        }

        if (predecessors != nullptr)
        {
            predecessors[level] = predecessor;
        }
        if (successors != nullptr)
        {
            successors[level] = current;
        }
    }

    MapNode<KeyType, ValueType, MaxLevel>* candidate = nullptr;
    if (successors != nullptr)
    {
        candidate = successors[0];
    }
    else if (predecessor == nullptr)
    {
        candidate = m_heads[0].load(std::memory_order_acquire);
    }
    else
    {
        candidate = predecessor->m_next[0].load(std::memory_order_acquire);
    }
    if (candidate != nullptr)
    {
        bool equal = false;
        if (!keyEqual(candidate->m_key, key, equal))
        {
            return false;
        }
        if (equal && foundNode != nullptr)
        {
            *foundNode = candidate;
        }
    }
    return true;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::findNode(const KeyType& key, MapNode<KeyType, ValueType, MaxLevel>** node) const
{
    if (node == nullptr)
    {
        return false;
    }
    *node = nullptr;
    return findPosition(key, nullptr, nullptr, node);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::findOrCreateNode(const KeyType& key, MapNode<KeyType, ValueType, MaxLevel>** node)
{
    if (node == nullptr)
    {
        return false;
    }
    *node = nullptr;

    for (;;)
    {
        MapNode<KeyType, ValueType, MaxLevel>* predecessors[MaxLevel];
        MapNode<KeyType, ValueType, MaxLevel>* successors[MaxLevel];
        MapNode<KeyType, ValueType, MaxLevel>* foundNode = nullptr;
        if (!findPosition(key, predecessors, successors, &foundNode))
        {
            return false;
        }
        if (foundNode != nullptr)
        {
            *node = foundNode;
            return true;
        }

        MapNode<KeyType, ValueType, MaxLevel>* newNode = createNode(key, randomLevel());
        if (newNode == nullptr)
        {
            return false;
        }
        for (size_t i = 0; i < newNode->m_level; ++i)
        {
            newNode->m_next[i].store(successors[i], std::memory_order_relaxed);
        }

        MapNode<KeyType, ValueType, MaxLevel>* expected = successors[0];
        if (compareAndSwapNext(predecessors[0], 0, expected, newNode))
        {
            m_retainedNodeCount.fetch_add(1, std::memory_order_acq_rel);
            linkUpperLevels(newNode);
            *node = newNode;
            return true;
        }
        delete newNode;
    }
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
MapNode<KeyType, ValueType, MaxLevel>* LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::createNode(const KeyType& key, size_t level)
{
    try
    {
        return new (std::nothrow) MapNode<KeyType, ValueType, MaxLevel>(key, level);
    }
    catch (...)
    {
        return nullptr;
    }
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
MapValueHolder<ValueType>* LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::createValueHolder(const ValueType& value, uint64_t generation)
{
    MapValueHolder<ValueType>* valueHolder = nullptr;
    try
    {
        valueHolder = new (std::nothrow) MapValueHolder<ValueType>(value, generation);
    }
    catch (...)
    {
        return nullptr;
    }
    if (valueHolder == nullptr)
    {
        return nullptr;
    }
    m_retainedValueCount.fetch_add(1, std::memory_order_acq_rel);
    return valueHolder;
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
void LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::releaseUnusedValueHolder(MapValueHolder<ValueType>* valueHolder)
{
    if (valueHolder == nullptr)
    {
        return;
    }
    delete valueHolder;
    m_retainedValueCount.fetch_sub(1, std::memory_order_acq_rel);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
bool LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::compareAndSwapNext(MapNode<KeyType, ValueType, MaxLevel>* predecessor,
                                                                             size_t level,
                                                                             MapNode<KeyType, ValueType, MaxLevel>*& expected,
                                                                             MapNode<KeyType, ValueType, MaxLevel>* desired)
{
    if (level >= MaxLevel)
    {
        return false;
    }
    if (predecessor == nullptr)
    {
        return m_heads[level].compare_exchange_strong(expected, desired, std::memory_order_release, std::memory_order_acquire);
    }
    return predecessor->m_next[level].compare_exchange_strong(expected, desired, std::memory_order_release, std::memory_order_acquire);
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
void LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::linkUpperLevels(MapNode<KeyType, ValueType, MaxLevel>* node)
{
    if (node == nullptr)
    {
        return;
    }

    for (size_t level = 1; level < node->m_level; ++level)
    {
        for (;;)
        {
            MapNode<KeyType, ValueType, MaxLevel>* predecessors[MaxLevel];
            MapNode<KeyType, ValueType, MaxLevel>* successors[MaxLevel];
            if (!findPosition(node->m_key, predecessors, successors, nullptr))
            {
                return;
            }
            if (successors[level] == node)
            {
                break;
            }

            MapNode<KeyType, ValueType, MaxLevel>* expected = successors[level];
            node->m_next[level].store(expected, std::memory_order_relaxed);
            if (compareAndSwapNext(predecessors[level], level, expected, node))
            {
                break;
            }
        }
    }
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
void LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::releaseNodeList(MapNode<KeyType, ValueType, MaxLevel>* node)
{
    while (node != nullptr)
    {
        MapNode<KeyType, ValueType, MaxLevel>* next = node->m_next[0].load(std::memory_order_relaxed);
        delete node;
        node = next;
    }
}

template <typename KeyType, typename ValueType, typename LessType, size_t MaxLevel>
void LockFreeMap<KeyType, ValueType, LessType, MaxLevel>::releaseAllNodes()
{
    MapNode<KeyType, ValueType, MaxLevel>* head = m_heads[0].exchange(nullptr, std::memory_order_acq_rel);
    for (size_t i = 1; i < MaxLevel; ++i)
    {
        m_heads[i].store(nullptr, std::memory_order_release);
    }
    releaseNodeList(head);
    m_retainedNodeCount.store(0, std::memory_order_release);
    m_retainedValueCount.store(0, std::memory_order_release);
}

#endif // __LOCK_FREE_MAP_INL__