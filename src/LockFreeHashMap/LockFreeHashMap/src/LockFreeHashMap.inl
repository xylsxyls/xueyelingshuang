#ifndef __LOCK_FREE_MAP_INL__
#define __LOCK_FREE_MAP_INL__

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::LockFreeHashMap(size_t bucketCount) :
m_buckets(nullptr),
m_bucketCount(normalizeBucketCount(bucketCount)),
m_bucketMask(m_bucketCount - 1),
m_hash(HashType()),
m_equal(EqualType()),
m_generation(1),
m_retainedNodeCount(0)
{
    try
    {
        m_buckets = new (std::nothrow) std::atomic<HashMapNode<KeyType, ValueType>*>[m_bucketCount];
    }
    catch (...)
    {
        m_buckets = nullptr;
    }
    if (m_buckets == nullptr)
    {
        m_bucketCount = 0;
        m_bucketMask = 0;
        return;
    }
    for (size_t i = 0; i < m_bucketCount; ++i)
    {
        m_buckets[i].store(nullptr, std::memory_order_relaxed);
    }
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::~LockFreeHashMap()
{
    releaseAllNodes();
    if (m_buckets != nullptr)
    {
        delete[] m_buckets;
        m_buckets = nullptr;
    }
    m_bucketCount = 0;
    m_bucketMask = 0;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapValueProxy<KeyType, ValueType, HashType, EqualType> LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::operator[](const KeyType& key)
{
    return HashMapValueProxy<KeyType, ValueType, HashType, EqualType>(this, key);
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::set(const KeyType& key, const ValueType& value)
{
    if (m_buckets == nullptr || m_bucketCount == 0)
    {
        return false;
    }

    size_t index = 0;
    if (!bucketIndex(key, index))
    {
        return false;
    }

    uint64_t generation = m_generation.load(std::memory_order_acquire);
    HashMapNode<KeyType, ValueType>* node = createValueNode(key, value, generation);
    if (node == nullptr)
    {
        return false;
    }

    for (;;)
    {
        generation = m_generation.load(std::memory_order_acquire);
        node->m_generation = generation;
        HashMapNode<KeyType, ValueType>* head = m_buckets[index].load(std::memory_order_acquire);
        node->m_next.store(head, std::memory_order_relaxed);
        if (m_buckets[index].compare_exchange_strong(head, node, std::memory_order_release, std::memory_order_acquire))
        {
            return true;
        }
    }
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::insert(const KeyType& key, const ValueType& value)
{
    if (m_buckets == nullptr || m_bucketCount == 0)
    {
        return false;
    }

    size_t index = 0;
    if (!bucketIndex(key, index))
    {
        return false;
    }

    uint64_t generation = m_generation.load(std::memory_order_acquire);
    HashMapNode<KeyType, ValueType>* node = createValueNode(key, value, generation);
    if (node == nullptr)
    {
        return false;
    }

    for (;;)
    {
        generation = m_generation.load(std::memory_order_acquire);
        node->m_generation = generation;
        HashMapNode<KeyType, ValueType>* head = m_buckets[index].load(std::memory_order_acquire);
        HashMapNode<KeyType, ValueType>* oldVisibleNode = nullptr;
        for (HashMapNode<KeyType, ValueType>* current = head; current != nullptr; current = current->m_next.load(std::memory_order_acquire))
        {
            if (current->m_generation != generation)
            {
                continue;
            }
            bool equal = false;
            if (!keysEqual(current->m_key, key, equal))
            {
                m_retainedNodeCount.fetch_sub(1, std::memory_order_acq_rel);
                delete node;
                return false;
            }
            if (equal)
            {
                oldVisibleNode = current;
                break;
            }
        }
        if (oldVisibleNode != nullptr && oldVisibleNode->hasValue())
        {
            m_retainedNodeCount.fetch_sub(1, std::memory_order_acq_rel);
            delete node;
            return false;
        }
        node->m_next.store(head, std::memory_order_relaxed);
        if (m_buckets[index].compare_exchange_strong(head, node, std::memory_order_release, std::memory_order_acquire))
        {
            return true;
        }
    }
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::erase(const KeyType& key)
{
    if (m_buckets == nullptr || m_bucketCount == 0)
    {
        return false;
    }

    size_t index = 0;
    if (!bucketIndex(key, index))
    {
        return false;
    }

    uint64_t generation = m_generation.load(std::memory_order_acquire);
    HashMapNode<KeyType, ValueType>* node = createEraseNode(key, generation);
    if (node == nullptr)
    {
        return false;
    }

    for (;;)
    {
        generation = m_generation.load(std::memory_order_acquire);
        node->m_generation = generation;
        HashMapNode<KeyType, ValueType>* head = m_buckets[index].load(std::memory_order_acquire);
        HashMapNode<KeyType, ValueType>* oldVisibleNode = nullptr;
        for (HashMapNode<KeyType, ValueType>* current = head; current != nullptr; current = current->m_next.load(std::memory_order_acquire))
        {
            if (current->m_generation != generation)
            {
                continue;
            }
            bool equal = false;
            if (!keysEqual(current->m_key, key, equal))
            {
                m_retainedNodeCount.fetch_sub(1, std::memory_order_acq_rel);
                delete node;
                return false;
            }
            if (equal)
            {
                oldVisibleNode = current;
                break;
            }
        }
        if (oldVisibleNode == nullptr || !oldVisibleNode->hasValue())
        {
            m_retainedNodeCount.fetch_sub(1, std::memory_order_acq_rel);
            delete node;
            return false;
        }
        node->m_next.store(head, std::memory_order_relaxed);
        if (m_buckets[index].compare_exchange_strong(head, node, std::memory_order_release, std::memory_order_acquire))
        {
            return true;
        }
    }
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::remove(const KeyType& key)
{
    return erase(key);
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::get(const KeyType& key, ValueType& value) const
{
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    HashMapNode<KeyType, ValueType>* node = visibleNode(key, generation);
    if (node == nullptr || !node->hasValue())
    {
        return false;
    }
    try
    {
        value = *node->m_value;
    }
    catch (...)
    {
        return false;
    }
    return true;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
ValueType LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::value(const KeyType& key, const ValueType& defaultValue) const
{
    ValueType result = defaultValue;
    get(key, result);
    return result;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::contains(const KeyType& key) const
{
    uint64_t generation = m_generation.load(std::memory_order_acquire);
    HashMapNode<KeyType, ValueType>* node = visibleNode(key, generation);
    return node != nullptr && node->hasValue();
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
size_t LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::count(const KeyType& key) const
{
    return contains(key) ? 1 : 0;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
size_t LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::count() const
{
    if (m_buckets == nullptr || m_bucketCount == 0)
    {
        return 0;
    }

    uint64_t generation = m_generation.load(std::memory_order_acquire);
    size_t result = 0;
    for (size_t i = 0; i < m_bucketCount; ++i)
    {
        HashMapNode<KeyType, ValueType>* head = m_buckets[i].load(std::memory_order_acquire);
        for (HashMapNode<KeyType, ValueType>* current = head; current != nullptr; current = current->m_next.load(std::memory_order_acquire))
        {
            if (current->m_generation != generation || !current->hasValue())
            {
                continue;
            }
            if (!isFirstVisibleVersion(head, current, generation))
            {
                continue;
            }
            ++result;
        }
    }
    return result;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
size_t LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::size() const
{
    return count();
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::empty() const
{
    return count() == 0;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
void LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::clear()
{
    m_generation.fetch_add(1, std::memory_order_acq_rel);
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
void LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::clearAndFreeMemory()
{
    releaseAllNodes();
    m_generation.fetch_add(1, std::memory_order_acq_rel);
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
template <typename FunctionType>
size_t LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::forEach(FunctionType function) const
{
    if (m_buckets == nullptr || m_bucketCount == 0)
    {
        return 0;
    }

    uint64_t generation = m_generation.load(std::memory_order_acquire);
    size_t result = 0;
    for (size_t i = 0; i < m_bucketCount; ++i)
    {
        HashMapNode<KeyType, ValueType>* head = m_buckets[i].load(std::memory_order_acquire);
        for (HashMapNode<KeyType, ValueType>* current = head; current != nullptr; current = current->m_next.load(std::memory_order_acquire))
        {
            if (current->m_generation != generation || !current->hasValue())
            {
                continue;
            }
            if (!isFirstVisibleVersion(head, current, generation))
            {
                continue;
            }
            try
            {
                function(current->m_key, *current->m_value);
            }
            catch (...)
            {
                return result;
            }
            ++result;
        }
    }
    return result;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
std::vector<KeyType> LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::keys() const
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

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
std::vector<ValueType> LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::values() const
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

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
std::vector<std::pair<KeyType, ValueType>> LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::toVector() const
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

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
size_t LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::bucketCount() const
{
    return m_bucketCount;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
size_t LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::retainedNodeCount() const
{
    return m_retainedNodeCount.load(std::memory_order_acquire);
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
size_t LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::normalizeBucketCount(size_t bucketCount)
{
    if (bucketCount == 0)
    {
        bucketCount = 1024;
    }
    size_t result = 1;
    while (result < bucketCount && result < (static_cast<size_t>(1) << (sizeof(size_t) * 8 - 1)))
    {
        result <<= 1;
    }
    return result;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::bucketIndex(const KeyType& key, size_t& index) const
{
    if (m_bucketCount == 0)
    {
        index = 0;
        return false;
    }
    try
    {
        index = m_hash(key) & m_bucketMask;
    }
    catch (...)
    {
        index = 0;
        return false;
    }
    return true;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::keysEqual(const KeyType& left, const KeyType& right, bool& result) const
{
    try
    {
        result = m_equal(left, right);
    }
    catch (...)
    {
        result = false;
        return false;
    }
    return true;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapNode<KeyType, ValueType>* LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::createValueNode(const KeyType& key, const ValueType& value, uint64_t generation)
{
    HashMapNode<KeyType, ValueType>* node = nullptr;
    try
    {
        node = new (std::nothrow) HashMapNode<KeyType, ValueType>(key, value, generation);
    }
    catch (...)
    {
        return nullptr;
    }
    if (node == nullptr)
    {
        return nullptr;
    }
    if (!node->hasValue())
    {
        delete node;
        return nullptr;
    }
    m_retainedNodeCount.fetch_add(1, std::memory_order_acq_rel);
    return node;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapNode<KeyType, ValueType>* LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::createEraseNode(const KeyType& key, uint64_t generation)
{
    HashMapNode<KeyType, ValueType>* node = nullptr;
    try
    {
        node = new (std::nothrow) HashMapNode<KeyType, ValueType>(key, generation);
    }
    catch (...)
    {
        return nullptr;
    }
    if (node == nullptr)
    {
        return nullptr;
    }
    m_retainedNodeCount.fetch_add(1, std::memory_order_acq_rel);
    return node;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
HashMapNode<KeyType, ValueType>* LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::visibleNode(const KeyType& key, uint64_t generation) const
{
    if (m_buckets == nullptr || m_bucketCount == 0)
    {
        return nullptr;
    }

    size_t index = 0;
    if (!bucketIndex(key, index))
    {
        return nullptr;
    }
    HashMapNode<KeyType, ValueType>* current = m_buckets[index].load(std::memory_order_acquire);
    while (current != nullptr)
    {
        if (current->m_generation == generation)
        {
            bool equal = false;
            if (!keysEqual(current->m_key, key, equal))
            {
                return nullptr;
            }
            if (equal)
            {
                return current;
            }
        }
        current = current->m_next.load(std::memory_order_acquire);
    }
    return nullptr;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
bool LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::isFirstVisibleVersion(HashMapNode<KeyType, ValueType>* head, HashMapNode<KeyType, ValueType>* node, uint64_t generation) const
{
    for (HashMapNode<KeyType, ValueType>* current = head; current != nullptr && current != node; current = current->m_next.load(std::memory_order_acquire))
    {
        if (current->m_generation != generation)
        {
            continue;
        }
        bool equal = false;
        if (!keysEqual(current->m_key, node->m_key, equal))
        {
            return false;
        }
        if (equal)
        {
            return false;
        }
    }
    return true;
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
void LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::releaseNodeList(HashMapNode<KeyType, ValueType>* node)
{
    while (node != nullptr)
    {
        HashMapNode<KeyType, ValueType>* next = node->m_next.load(std::memory_order_relaxed);
        delete node;
        m_retainedNodeCount.fetch_sub(1, std::memory_order_acq_rel);
        node = next;
    }
}

template <typename KeyType, typename ValueType, typename HashType, typename EqualType>
void LockFreeHashMap<KeyType, ValueType, HashType, EqualType>::releaseAllNodes()
{
    if (m_buckets == nullptr || m_bucketCount == 0)
    {
        return;
    }
    for (size_t i = 0; i < m_bucketCount; ++i)
    {
        HashMapNode<KeyType, ValueType>* head = m_buckets[i].exchange(nullptr, std::memory_order_acq_rel);
        releaseNodeList(head);
    }
}

#endif // __LOCK_FREE_MAP_INL__