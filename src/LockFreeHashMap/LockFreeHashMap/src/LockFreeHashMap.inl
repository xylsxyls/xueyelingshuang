#pragma once
#include <algorithm>

template <typename K, typename V, typename H, typename E>
LockFreeHashMap<K, V, H, E>::LockFreeHashMap(size_t buckets) :
m_buckets(nullptr), m_bucketCount(normalizeBucketCount(buckets)), m_hash(H()), m_equal(E()),
m_storage(nullptr), m_generation(1), m_sequence(0), m_status(LockFreeHashMapOk), m_maintenanceBucket(0), m_cleanupBucket(0)
{
    if (m_bucketCount == 0)
    {
        m_status = LockFreeHashMapInvalidArgument;
        return;
    }
    try
    {
        m_identity = std::make_shared<LockFreeHashMapIdentity>();
        m_buckets = new (std::nothrow) std::atomic<uint64_t>[m_bucketCount];
        if (m_buckets == nullptr)
        {
            m_status = LockFreeHashMapNoMemory;
            return;
        }
        for (size_t i = 0; i < m_bucketCount; ++i)
        {
            m_buckets[i].store(1);
        }
        m_storage = new (std::nothrow) Storage(LockFreeHashMapPoolConfig(), sizeof(*this) + m_bucketCount * sizeof(std::atomic<uint64_t>) + sizeof(LockFreeHashMapIdentity));
        m_status = m_storage == nullptr ? LockFreeHashMapNoMemory : m_storage->init();
    }
    catch (const std::bad_alloc&)
    {
        m_status = LockFreeHashMapNoMemory;
    }
    catch (...)
    {
        m_status = LockFreeHashMapUserException;
    }
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMap<K, V, H, E>::~LockFreeHashMap()
{
    delete m_storage;
    delete[] m_buckets;
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::normalizeBucketCount(size_t count)
{
    if (count == 0)
    {
        count = 1024;
    }
    size_t result = 1;
    size_t limit = ((std::numeric_limits<size_t>::max)() - sizeof(LockFreeHashMap<K, V, H, E>) - sizeof(LockFreeHashMapIdentity)) / sizeof(std::atomic<uint64_t>);
    while (result < count)
    {
        if (result > limit / 2)
        {
            return 0;
        }
        result *= 2;
    }
    return result <= limit ? result : 0;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapStatus LockFreeHashMap<K, V, H, E>::status() const
{
    return m_status;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::configurePool(const LockFreeHashMapPoolConfig& config)
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    // 外部静止允许直接检查，不申请内存也不执行用户代码
    uint64_t generation = m_generation.load();
    for (size_t i = 0; i < m_bucketCount; ++i)
    {
        uint64_t head = m_buckets[i].load();
        Slot* node = (head & 1) != 0 ? nullptr : reinterpret_cast<Slot*>(static_cast<uintptr_t>(head));
        while (node != nullptr)
        {
            if (generation != 0 && node->m_generation == generation && node->m_value.load() != nullptr)
            {
                return LockFreeHashMapResult(LockFreeHashMapNotEmpty);
            }
            node = reinterpret_cast<Slot*>(node->m_next.load() & ~uintptr_t(1));
        }
    }
    Storage* replacement = nullptr;
    LockFreeHashMapStatus result = LockFreeHashMapOk;
    try
    {
        replacement = new (std::nothrow) Storage(config, sizeof(*this) + m_bucketCount * sizeof(std::atomic<uint64_t>) + sizeof(LockFreeHashMapIdentity));
        result = replacement == nullptr ? LockFreeHashMapNoMemory : replacement->init();
    }
    catch (const std::bad_alloc&)
    {
        result = LockFreeHashMapNoMemory;
    }
    catch (...)
    {
        result = LockFreeHashMapUserException;
    }
    if (result != LockFreeHashMapOk)
    {
        delete replacement;
        return LockFreeHashMapResult(result);
    }
    for (size_t i = 0; i < m_bucketCount; ++i)
    {
        m_buckets[i].store(1);
    }
    delete m_storage;
    m_storage = replacement;
    return LockFreeHashMapResult();
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::setPoolPolicies(
    const LockFreeHashMapGrowPolicy& grow,
    const LockFreeHashMapShrinkPolicy& shrink)
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    try
    {
        LockFreeHashMapGrowPolicy growCopy(grow);
        LockFreeHashMapShrinkPolicy shrinkCopy(shrink);
        m_storage->m_config.growPolicy.swap(growCopy);
        m_storage->m_config.shrinkPolicy.swap(shrinkCopy);
    }
    catch (const std::bad_alloc&)
    {
        return LockFreeHashMapResult(LockFreeHashMapNoMemory);
    }
    catch (...)
    {
        return LockFreeHashMapResult(LockFreeHashMapUserException);
    }
    return LockFreeHashMapResult();
}

template <typename K, typename V, typename H, typename E>
uint64_t LockFreeHashMap<K, V, H, E>::nextVersion(LockFreeHashMapGuard& guard)
{
    if (guard.m_record->m_nextVersion != 0)
    {
        uint64_t result = guard.m_record->m_nextVersion;
        guard.m_record->m_nextVersion = result == guard.m_record->m_lastVersion ? 0 : result + 1;
        return result;
    }
    uint64_t sequence = m_sequence.load();
    while (sequence != (std::numeric_limits<uint64_t>::max)())
    {
        uint64_t count = (std::min)(uint64_t(256), (std::numeric_limits<uint64_t>::max)() - sequence);
        if (m_sequence.compare_exchange_weak(sequence, sequence + count))
        {
            guard.m_record->m_nextVersion = count == 1 ? 0 : sequence + 2;
            guard.m_record->m_lastVersion = sequence + count;
            return sequence + 1;
        }
    }
    return 0;
}

template <typename K, typename V, typename H, typename E>
uintptr_t LockFreeHashMap<K, V, H, E>::closeNode(Slot* node) const
{
    // exchange 唯一取得摘除的当前值；竞争中的转移先被封住，或将完整构造的后继交给本操作
    Slot* value = node->m_value.exchange(nullptr);
    if (m_storage->m_inlineScalar)
    {
        while (value != nullptr)
        {
            LockFreeHashMapScalarState old = value->m_scalar.load();
            LockFreeHashMapScalarState closed = {};
            while (old.m_version != 0 && !value->m_scalar.compareExchange(old, closed))
            {
                Storage::increment(m_storage->m_retries);
            }
            Slot* successor = old.m_version == 0 ? reinterpret_cast<Slot*>(static_cast<uintptr_t>(old.m_bits)) : nullptr;
            if (value != node)
            {
                m_storage->retire(value);
            }
            value = successor;
        }
    }
    else if (value != nullptr)
    {
        forgetValue(*node->key(), node->m_hash, value);
        m_storage->retire(value);
    }
    if (m_storage->m_lookup != nullptr)
    {
        for (size_t probe = 0; probe < 2; ++probe)
        {
            std::atomic<Slot*>& cell = m_storage->m_lookup[cacheIndex(lookupHash(*node->key(), node->m_hash), probe)].m_node;
            Slot* expected = cell.load();
            if (expected == node)
            {
                cell.compare_exchange_strong(expected, nullptr);
            }
        }
    }
    uintptr_t next = node->m_next.load();
    while ((next & 1) == 0 && !node->m_next.compare_exchange_weak(next, next | 1))
    {
        Storage::increment(m_storage->m_retries);
    }
    return next & ~uintptr_t(1);
}

template <typename K, typename V, typename H, typename E>
uint64_t LockFreeHashMap<K, V, H, E>::loadLink(Slot* predecessor, size_t bucket) const
{
    return predecessor == nullptr ? m_buckets[bucket].load() : static_cast<uint64_t>(predecessor->m_next.load());
}

template <typename K, typename V, typename H, typename E>
uint64_t LockFreeHashMap<K, V, H, E>::nextEmptyIdentity() const
{
    uint64_t sequence = m_storage->m_emptySequence.load();
    while (sequence != (std::numeric_limits<uint64_t>::max)())
    {
        if (m_storage->m_emptySequence.compare_exchange_weak(sequence, sequence + 2))
        {
            return sequence + 2;
        }
    }
    return 0;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::unlinkNode(Slot* predecessor, size_t bucket, Slot* node, uintptr_t& next) const
{
    bool removed = false;
    bool oldChain = false;
    if (predecessor == nullptr)
    {
        // closeNode 已先标记后继再观察代次；clear 摘除前缀后不能重新暴露旧尾链，暂停的不存在插入可能仍以该尾链为期望
        uint64_t active = m_generation.load();
        oldChain = active == 0 || node->m_generation < active;
        uint64_t desired = oldChain || next == 0 ? nextEmptyIdentity() : static_cast<uint64_t>(next);
        if (desired == 0)
        {
            if (oldChain)
            {
                // 空桶身份已耗尽时保留完整旧链作为终态哨兵，逻辑不可见，只能在外部静止时释放
                next = 0;
            }
            return false;
        }
        uint64_t expected = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(node));
        removed = m_buckets[bucket].compare_exchange_strong(expected, desired);
    }
    else
    {
        uintptr_t expected = reinterpret_cast<uintptr_t>(node);
        removed = predecessor->m_next.compare_exchange_strong(expected, next);
    }
    if (removed)
    {
        if (oldChain)
        {
            // 只从头插入使代次沿链不递增；唯一摘除者先标记父节点再追踪后继，冻结进入链接，阻止并发摘除者改变所有权
            Slot* owned = node;
            while (owned != nullptr)
            {
                uintptr_t successor = closeNode(owned);
                m_storage->retire(owned);
                owned = reinterpret_cast<Slot*>(successor);
            }
            next = 0;
        }
        else
        {
            m_storage->retire(node);
        }
    }
    return removed;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapStatus LockFreeHashMap<K, V, H, E>::search(const K* key, size_t hash, uint64_t generation,
    LockFreeHashMapGuard& guard, Slot*& found, Slot*& value, uint64_t& head, bool pinHead) const
{
    found = nullptr;
    value = nullptr;
    const size_t index = hash & (m_bucketCount - 1);
    guard.resetLinks();
    guard.protect(0, nullptr);
    guard.protect(3, nullptr);
    for (size_t probe = 0; key != nullptr && m_storage->m_lookup != nullptr && probe < 2 &&
        !m_storage->m_lookup[cacheIndex(lookupHash(*key, hash), 0)].m_colliding.load(std::memory_order_relaxed); ++probe)
    {
        std::atomic<Slot*>& cell = m_storage->m_lookup[cacheIndex(lookupHash(*key, hash), probe)].m_node;
        Slot* cached = cell.load();
        if (cached != nullptr)
        {
            guard.protect(guard.m_currentSlot, cached);
            if (cell.load() == cached && cached->m_generation == generation && cached->m_hash == hash)
            {
                try
                {
                    if (m_equal(*cached->key(), *key))
                    {
                        Slot* observed = nullptr;
                        if (captureValue(cached, observed, guard) && m_generation.load() == generation)
                        {
                            found = cached;
                            value = observed;
                            return LockFreeHashMapOk;
                        }
                    }
                    else
                    {
                        m_storage->m_lookup[cacheIndex(lookupHash(*key, hash), 0)].m_colliding.store(true, std::memory_order_relaxed);
                    }
                }
                catch (...)
                {
                    return LockFreeHashMapUserException;
                }
            }
        }
    }
    for (;;)
    {
        guard.resetLinks();
        guard.protect(3, nullptr);
        head = m_buckets[index].load();
        if (pinHead)
        {
            // 只有不存在时的插入需要在整个查找期间保持原桶头 CAS 期望的保护
            guard.protect(0, (head & 1) != 0 ? nullptr : reinterpret_cast<void*>(static_cast<uintptr_t>(head)));
            if (m_buckets[index].load() != head)
            {
                continue;
            }
        }
        Slot* predecessor = nullptr;
        uintptr_t current = (head & 1) != 0 ? 0 : static_cast<uintptr_t>(head);
        bool restart = false;
        while (current != 0)
        {
            Slot* node = reinterpret_cast<Slot*>(current);
            guard.protect(guard.m_currentSlot, node);
            if (loadLink(predecessor, index) != current)
            {
                restart = true;
                break;
            }
            uint64_t active = m_generation.load();
            Slot* observed = node->m_value.load();
            uintptr_t next = node->m_next.load();
            if (observed == nullptr || active == 0 || node->m_generation < active)
            {
                next = closeNode(node);
                unlinkNode(predecessor, index, node, next);
                if (predecessor == nullptr && next == 0 && m_storage->m_emptySequence.load() == (std::numeric_limits<uint64_t>::max)())
                {
                    // 身份耗尽时保留最后一个关闭节点作为不可复用的空表哨兵
                    return LockFreeHashMapNotFound;
                }
                restart = true;
                break;
            }
            if ((next & 1) != 0)
            {
                restart = true;
                break;
            }
            if (key != nullptr && node->m_generation == generation && node->m_hash == hash)
            {
                try
                {
                    if (m_equal(*node->key(), *key))
                    {
                        // 只为匹配键保护值；标量字可转移到新前缀记录，任何观察者都能帮助完成
                        if (!captureValue(node, observed, guard))
                        {
                            restart = true;
                            break;
                        }
                        remember(node);
                        found = node;
                        value = observed;
                        return LockFreeHashMapOk;
                    }
                }
                catch (...)
                {
                    return LockFreeHashMapUserException;
                }
            }
            guard.advance();
            predecessor = node;
            current = next;
        }
        if (!restart)
        {
            return LockFreeHashMapNotFound;
        }
        Storage::increment(m_storage->m_retries);
    }
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapStatus LockFreeHashMap<K, V, H, E>::snapshot(Slot* value, Snapshot& output, const LockFreeHashMapScalarState* scalar) const
{
    try
    {
        std::shared_ptr<const V> copy = scalar != nullptr ?
            std::make_shared<V>(LockFreeHashMapScalar<V>::decode(scalar->m_bits)) : std::make_shared<V>(*value->value());
        output.m_value.swap(copy);
        output.m_version.m_instance = m_identity;
        output.m_version.m_sequence = scalar != nullptr ? scalar->m_version : value->m_version;
    }
    catch (const std::bad_alloc&)
    {
        return LockFreeHashMapNoMemory;
    }
    catch (...)
    {
        return LockFreeHashMapUserException;
    }
    return LockFreeHashMapOk;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::find(const K& key, Snapshot& output) const
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
    if (guard.m_record == nullptr)
    {
        return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
    }
    size_t hash = 0;
    try
    {
        hash = m_hash(key);
    }
    catch (...)
    {
        return LockFreeHashMapResult(LockFreeHashMapUserException);
    }
    Slot* node = nullptr;
    Slot* value = nullptr;
    uint64_t head = 0;
    uint64_t generation = m_generation.load();
    LockFreeHashMapStatus result = search(&key, hash, generation, guard, node, value, head);
    if (result == LockFreeHashMapOk)
    {
        result = snapshot(value, output, m_storage->m_inlineScalar ? &guard.m_scalar : nullptr);
    }
    return LockFreeHashMapResult(result);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::modify(const K& key, const V* input, int32_t mode,
    const LockFreeHashMapVersion* version, Snapshot* output)
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    if (version != nullptr && (version->m_instance != m_identity || version->m_sequence == 0))
    {
        return LockFreeHashMapResult(LockFreeHashMapVersionMismatch);
    }
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
    if (guard.m_record == nullptr)
    {
        return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
    }
    uint64_t generation = m_generation.load();
    if (generation == 0 || (mode < 3 && m_storage->m_emptySequence.load() == (std::numeric_limits<uint64_t>::max)()))
    {
        return LockFreeHashMapResult(LockFreeHashMapVersionExhausted);
    }
    if (generation > 1)
    {
        cleanBuckets(guard, 1);
    }
    size_t hash = 0;
    try
    {
        hash = m_hash(key);
    }
    catch (...)
    {
        return LockFreeHashMapResult(LockFreeHashMapUserException);
    }
    Slot* candidate = nullptr;
    Slot* candidateKey = nullptr;
    Slot* reclaimValue = nullptr;
    Slot* reclaimKey = nullptr;
    Snapshot prepared;
    LockFreeHashMapScalarState candidateScalar = {};
    LockFreeHashMapResult result;
    for (;;)
    {
        Slot* node = nullptr;
        Slot* old = nullptr;
        uint64_t head = 0;
        result.m_status = search(&key, hash, generation, guard, node, old, head, mode < 2);
        if (result.m_status != LockFreeHashMapOk && result.m_status != LockFreeHashMapNotFound)
        {
            break;
        }
        if (m_generation.load() != generation)
        {
            result.m_status = LockFreeHashMapCleared;
            break;
        }
        if (mode < 3 && m_storage->m_emptySequence.load() == (std::numeric_limits<uint64_t>::max)())
        {
            result.m_status = LockFreeHashMapVersionExhausted;
            break;
        }
        if (node != nullptr && mode == 1)
        {
            result.m_status = output != nullptr ? snapshot(old, prepared, m_storage->m_inlineScalar ? &guard.m_scalar : nullptr) : LockFreeHashMapOk;
            if (result.m_status == LockFreeHashMapOk)
            {
                result.m_status = LockFreeHashMapExists;
            }
            break;
        }
        if (node == nullptr && mode >= 2)
        {
            result.m_status = LockFreeHashMapNotFound;
            break;
        }
        if (version != nullptr && (m_storage->m_inlineScalar ? guard.m_scalar.m_version : old->m_version) != version->m_sequence)
        {
            result.m_status = LockFreeHashMapVersionMismatch;
            break;
        }
        if (mode < 3 && candidate == nullptr && candidateScalar.m_version == 0)
        {
            uint64_t sequence = nextVersion(guard);
            if (sequence == 0)
            {
                result.m_status = LockFreeHashMapVersionExhausted;
                break;
            }
            if (m_storage->m_inlineScalar)
            {
                candidateScalar.m_bits = LockFreeHashMapScalar<V>::encode(*input);
                candidateScalar.m_version = sequence;
                if (output != nullptr)
                {
                    result.m_status = snapshot(nullptr, prepared, &candidateScalar);
                    if (result.m_status != LockFreeHashMapOk)
                    {
                        break;
                    }
                }
            }
            else
            {
                candidate = m_storage->acquire(2, guard, result.m_status);
                if (candidate == nullptr)
                {
                    break;
                }
                try
                {
                    new (candidate->value()) V(*input);
                }
                catch (const std::bad_alloc&)
                {
                    m_storage->release(candidate, false);
                    candidate = nullptr;
                    result.m_status = LockFreeHashMapNoMemory;
                    break;
                }
                catch (...)
                {
                    m_storage->release(candidate, false);
                    candidate = nullptr;
                    result.m_status = LockFreeHashMapUserException;
                    break;
                }
                candidate->m_version = sequence;
                if (usesValueCache())
                {
                    // 该后端只接受平凡整数或枚举键，不增加拥有资源的载荷，键生命周期结束无需析构
                    new (candidate->key()) K(key);
                    candidate->m_generation = generation;
                    candidate->m_hash = hash;
                }
                if (output != nullptr)
                {
                    result.m_status = snapshot(candidate, prepared);
                    if (result.m_status != LockFreeHashMapOk)
                    {
                        break;
                    }
                }
            }
        }
        if (node != nullptr && mode < 3 && m_storage->m_inlineScalar &&
            !old->m_scalar.accepts(candidateScalar) && candidate == nullptr)
        {
            candidate = m_storage->acquire(2, guard, result.m_status);
            if (candidate == nullptr)
            {
                break;
            }
            candidate->m_scalar.initialize(candidateScalar);
        }
        if (node == nullptr && candidateKey == nullptr)
        {
            candidateKey = m_storage->acquire(1, guard, result.m_status);
            if (candidateKey == nullptr)
            {
                break;
            }
            try
            {
                new (candidateKey->key()) K(key);
            }
            catch (const std::bad_alloc&)
            {
                m_storage->release(candidateKey, false);
                candidateKey = nullptr;
                result.m_status = LockFreeHashMapNoMemory;
                break;
            }
            catch (...)
            {
                m_storage->release(candidateKey, false);
                candidateKey = nullptr;
                result.m_status = LockFreeHashMapUserException;
                break;
            }
            candidateKey->m_hash = hash;
            candidateKey->m_generation = generation;
        }
        if (mode >= 3 && output != nullptr)
        {
            result.m_status = snapshot(old, prepared, m_storage->m_inlineScalar ? &guard.m_scalar : nullptr);
            if (result.m_status != LockFreeHashMapOk)
            {
                break;
            }
        }
        bool committed = false;
        uint64_t newVersion = mode >= 3 ? 0 : (m_storage->m_inlineScalar ? candidateScalar.m_version : candidate->m_version);
        uint64_t cacheTag = invalidateRead(key, hash, generation, newVersion);
        if (node == nullptr)
        {
            if (m_storage->m_inlineScalar)
            {
                candidateKey->m_scalar.initialize(candidateScalar);
            }
            candidateKey->m_value.store(m_storage->m_inlineScalar ? candidateKey : candidate, std::memory_order_relaxed);
            candidateKey->m_next.store((head & 1) != 0 ? 0 : static_cast<uintptr_t>(head), std::memory_order_relaxed);
            // 发布前先保护候选，并发 clear 或 erase 可能立即摘除它
            guard.protect(guard.m_currentSlot, candidateKey);
            committed = m_buckets[hash & (m_bucketCount - 1)].compare_exchange_strong(head, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(candidateKey)));
            if (committed)
            {
                remember(candidateKey);
                candidateKey = nullptr;
                if (!m_storage->m_inlineScalar)
                {
                    candidate = nullptr;
                }
            }
        }
        else
        {
            Slot* expected = old;
            if (m_storage->m_inlineScalar)
            {
                LockFreeHashMapScalarState desired = {};
                if (mode < 3)
                {
                    if (old->m_scalar.accepts(candidateScalar))
                    {
                        desired = candidateScalar;
                    }
                    else
                    {
                        desired.m_bits = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(candidate));
                    }
                }
                committed = old->m_scalar.compareExchange(guard.m_scalar, desired);
                if (committed && mode < 3 && desired.m_version == 0)
                {
                    // 此源 CAS 发布新值；移动键的值指针可以由其他线程帮助完成，不等待原发布者
                    helpValue(node, old, candidate);
                    candidate = nullptr;
                    if (old != node)
                    {
                        reclaimValue = old;
                    }
                }
            }
            else
            {
                committed = node->m_value.compare_exchange_strong(expected, mode >= 3 ? nullptr : candidate);
            }
            if (committed)
            {
                if (!m_storage->m_inlineScalar)
                {
                    candidate = nullptr;
                    forgetValue(key, hash, old);
                    m_storage->retire(old);
                    reclaimValue = old;
                }
                if (mode >= 3)
                {
                    uintptr_t next = closeNode(node);
                    if (m_storage->m_inlineScalar && old != node)
                    {
                        reclaimValue = old;
                    }
                    Slot* predecessor = static_cast<Slot*>(guard.m_record->m_pointer[guard.m_predecessorSlot].load());
                    if (unlinkNode(predecessor, hash & (m_bucketCount - 1), node, next))
                    {
                        reclaimKey = node;
                    }
                }
            }
        }
        if (committed)
        {
            if (mode < 3)
            {
                if (m_storage->m_inlineScalar)
                {
                    publishRead(key, hash, *input, cacheTag);
                }
            }
            result = LockFreeHashMapResult(LockFreeHashMapOk, true);
            break;
        }
        Storage::increment(m_storage->m_retries);
    }
    if (candidateKey != nullptr)
    {
        m_storage->release(candidateKey, true);
    }
    if (candidate != nullptr)
    {
        m_storage->release(candidate, true);
    }
    if (output != nullptr && (result.m_committed || result.m_status == LockFreeHashMapExists))
    {
        output->m_value.swap(prepared.m_value);
        output->m_version.m_instance.swap(prepared.m_version.m_instance);
        std::swap(output->m_version.m_sequence, prepared.m_version.m_sequence);
    }
    // 保持这些精确摘除槽位的保护直到领取回收权，之后不再访问载荷或链接
    bool deferred = reclaimValue != nullptr && !m_storage->reclaimProtected(reclaimValue, guard);
    if (reclaimKey != nullptr && !m_storage->reclaimProtected(reclaimKey, guard))
    {
        deferred = true;
    }
    for (size_t i = 0; i < 4; ++i)
    {
        guard.protect(i, nullptr);
    }
    if (deferred || (++guard.m_record->m_writeCount & (m_storage->m_inlineScalar ? 255 : 15)) == 0)
    {
        m_storage->reclaim(guard, 16);
    }
    return result;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::trySet(const K& key, const V& value, Snapshot* output)
{
    LockFreeHashMapResult result;
    if (output == nullptr && (updateScalar(key, value, result) || updateValue(key, value, result)))
    {
        return result;
    }
    return modify(key, &value, 0, nullptr, output);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::insertIfAbsent(const K& key, const V& value, Snapshot& output)
{
    return modify(key, &value, 1, nullptr, &output);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::compareExchange(const K& key,
    const LockFreeHashMapVersion& version, const V& value, Snapshot* output)
{
    return modify(key, &value, 2, &version, output);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::eraseIf(const K& key, const LockFreeHashMapVersion& version)
{
    return modify(key, nullptr, 4, &version, nullptr);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryErase(const K& key)
{
    LockFreeHashMapResult result;
    if (eraseScalar(key, result))
    {
        return result;
    }
    return modify(key, nullptr, 3, nullptr, nullptr);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::take(const K& key, Snapshot& output)
{
    return modify(key, nullptr, 3, nullptr, &output);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryClear()
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    uint64_t generation = m_generation.load();
    while (generation != 0)
    {
        uint64_t next = generation == (std::numeric_limits<uint64_t>::max)() ? 0 : generation + 1;
        if (m_generation.compare_exchange_weak(generation, next))
        {
            // 清理可以被帮助完成且不申请键值；保护记录不足不会撤销已经生效的 clear
            LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
            if (guard.m_record != nullptr)
            {
                cleanBuckets(guard, m_bucketCount);
                m_storage->reclaim(guard, (std::numeric_limits<size_t>::max)());
            }
            return LockFreeHashMapResult(next == 0 ? LockFreeHashMapVersionExhausted : LockFreeHashMapOk, true);
        }
    }
    return LockFreeHashMapResult(LockFreeHashMapVersionExhausted);
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::cleanBuckets(LockFreeHashMapGuard& guard, size_t bucketLimit)
{
    size_t start = m_cleanupBucket.fetch_add(bucketLimit);
    for (size_t i = 0; i < bucketLimit; ++i)
    {
        Slot* node = nullptr;
        Slot* value = nullptr;
        uint64_t head = 0;
        search(nullptr, (start + i) & (m_bucketCount - 1), m_generation.load(), guard, node, value, head);
    }
    for (size_t i = 0; i < 4; ++i)
    {
        guard.protect(i, nullptr);
    }
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::maintain(size_t budget)
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
    if (guard.m_record == nullptr)
    {
        return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
    }
    for (size_t i = 0; i < (std::min)(m_bucketCount, budget); ++i)
    {
        size_t bucket = m_maintenanceBucket++ % m_bucketCount;
        Slot* node = nullptr;
        Slot* value = nullptr;
        uint64_t head = 0;
        search(nullptr, bucket, m_generation.load(), guard, node, value, head);
    }
    for (size_t i = 0; i < 4; ++i)
    {
        guard.protect(i, nullptr);
    }
    m_storage->reclaim(guard, budget);
    return m_storage->maintain(guard, budget);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::poolStats(LockFreeHashMapPoolStats& output) const
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
    if (guard.m_record == nullptr)
    {
        return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
    }
    output = m_storage->stats(guard);
    return LockFreeHashMapResult();
}

template <typename K, typename V, typename H, typename E>
HashMapValueProxy<K, V, H, E> LockFreeHashMap<K, V, H, E>::operator[](const K& key)
{
    return HashMapValueProxy<K, V, H, E>(this, key);
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::set(const K& key, const V& value)
{
    return trySet(key, value).m_committed;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::insert(const K& key, const V& value)
{
    return modify(key, &value, 1, nullptr, nullptr).m_committed;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::erase(const K& key)
{
    return tryErase(key).m_committed;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::remove(const K& key)
{
    return erase(key);
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::get(const K& key, V& output) const
{
    if (m_status != LockFreeHashMapOk)
    {
        return false;
    }
    // 内建整数的标准哈希无用户回调；副本缓存独立按键索引，命中无需重新计算该哈希
    bool builtinHash = std::is_integral<K>::value && std::is_same<H, std::hash<K>>::value;
    if (builtinHash)
    {
        if (cachedRead(key, 0, output))
        {
            return true;
        }
        LockFreeHashMapStatus result = cachedValueRead(key, 0, output);
        if (result != LockFreeHashMapNotFound)
        {
            return result == LockFreeHashMapOk;
        }
        return getProtected(key, m_hash(key), output);
    }
    return getHashed(key, output);
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::getHashed(const K& key, V& output) const
{
    size_t hash = 0;
    try
    {
        hash = m_hash(key);
        if (cachedRead(key, hash, output))
        {
            return true;
        }
    }
    catch (...)
    {
        return false;
    }
    LockFreeHashMapStatus result = cachedValueRead(key, hash, output);
    if (result != LockFreeHashMapNotFound)
    {
        return result == LockFreeHashMapOk;
    }
    return getProtected(key, hash, output);
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::getProtected(const K& key, size_t hash, V& output) const
{
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
    if (guard.m_record == nullptr)
    {
        return false;
    }
    try
    {
        Slot* node = nullptr;
        Slot* value = nullptr;
        uint64_t head = 0;
        if (search(&key, hash, m_generation.load(), guard, node, value, head) != LockFreeHashMapOk)
        {
            return false;
        }
        if (usesValueCache())
        {
            uint64_t tag = value->m_generation != 0 && value->m_generation <= UINT64_C(0xffffffff) &&
                value->m_version != 0 && value->m_version <= UINT64_C(0xffffffff) ?
                (value->m_generation << 32) | value->m_version : 0;
            publishValue(key, hash, value, tag);
        }
        if (m_storage->m_inlineScalar)
        {
            LockFreeHashMapScalar<V>::copy(output, guard.m_scalar.m_bits);
        }
        else
        {
            // 分支赋值避免条件表达式先复制临时 V；非平凡值只执行一次赋值，保护保持到赋值完成或异常离开
            output = *value->value();
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H, typename E>
V LockFreeHashMap<K, V, H, E>::value(const K& key, const V& fallback) const
{
    V result = fallback;
    get(key, result);
    return result;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::contains(const K& key) const
{
    if (m_status != LockFreeHashMapOk)
    {
        return false;
    }
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
    if (guard.m_record == nullptr)
    {
        return false;
    }
    try
    {
        Slot* node = nullptr;
        Slot* value = nullptr;
        uint64_t head = 0;
        return search(&key, m_hash(key), m_generation.load(), guard, node, value, head) == LockFreeHashMapOk;
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::count(const K& key) const
{
    return contains(key) ? 1 : 0;
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::count() const
{
    // key、value 仅在回调期间借用；不做业务操作，由遍历器累计完成次数
    return forEach([](const K& key, const V& value)
    {
        (void)key;
        (void)value;
    });
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::size() const
{
    return count();
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::empty() const
{
    return count() == 0;
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::clear()
{
    tryClear();
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::clearAndFreeMemory()
{
    if (m_status != LockFreeHashMapOk)
    {
        return;
    }
    tryClear();
    for (size_t i = 0; i < m_bucketCount; ++i)
    {
        m_buckets[i].store(1);
    }
    m_storage->freeQuiescent();
}

template <typename K, typename V, typename H, typename E>
template <typename F>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::forEachProtected(F&& input) const
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
    if (guard.m_record == nullptr)
    {
        return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
    }
    LockFreeHashMapResult result;
    uint64_t generation = m_generation.load();
    try
    {
        // 在异常边界内准备独立回调，允许非 const 调用算子；只有右值转移所有权
        typename std::decay<F>::type function(std::forward<F>(input));
        for (size_t bucket = 0; bucket < m_bucketCount; ++bucket)
        {
            guard.resetLinks();
            Slot* predecessor = nullptr;
            uint64_t head = m_buckets[bucket].load();
            uintptr_t current = (head & 1) != 0 ? 0 : static_cast<uintptr_t>(head);
            while (current != 0)
            {
                Slot* node = reinterpret_cast<Slot*>(current);
                guard.protect(guard.m_currentSlot, node);
                if (loadLink(predecessor, bucket) != current)
                {
                    break;
                }
                Slot* value = node->m_value.load();
                uintptr_t next = node->m_next.load();
                uint64_t active = m_generation.load();
                if (value == nullptr || active == 0 || node->m_generation < active)
                {
                    next = closeNode(node);
                    if (!unlinkNode(predecessor, bucket, node, next))
                    {
                        break;
                    }
                    current = next;
                    continue;
                }
                if ((next & 1) != 0)
                {
                    break;
                }
                if (!captureValue(node, value, guard))
                {
                    // 此节点尚未调用回调，转移或关闭后只重试当前游标；关闭的中间节点不能隐藏后续尾链
                    continue;
                }
                if (node->m_generation == generation)
                {
                    if (m_storage->m_inlineScalar)
                    {
                        V copy = LockFreeHashMapScalar<V>::decode(guard.m_scalar.m_bits);
                        function(*node->key(), copy);
                    }
                    else
                    {
                        function(*node->key(), *value->value());
                    }
                    ++result.m_count;
                }
                guard.advance();
                predecessor = node;
                current = next;
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        result.m_status = LockFreeHashMapNoMemory;
    }
    catch (...)
    {
        result.m_status = LockFreeHashMapUserException;
    }
    return result;
}

template <typename K, typename V, typename H, typename E>
template <typename F>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::forEachResult(const F& function) const
{
    return forEachProtected(function);
}

template <typename K, typename V, typename H, typename E>
template <typename F>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::forEachResult(F&& function) const
{
    return forEachProtected(std::forward<F>(function));
}

template <typename K, typename V, typename H, typename E>
template <typename F>
size_t LockFreeHashMap<K, V, H, E>::forEach(const F& function) const
{
    return forEachResult(function).m_count;
}

template <typename K, typename V, typename H, typename E>
template <typename F>
size_t LockFreeHashMap<K, V, H, E>::forEach(F&& function) const
{
    return forEachResult(std::forward<F>(function)).m_count;
}

template <typename K, typename V, typename H, typename E>
std::vector<K> LockFreeHashMap<K, V, H, E>::keys() const
{
    std::vector<K> result;
    // 借用 key、value，将键副本追加到 result；复制异常由遍历边界转换为失败
    if (forEachResult([&result](const K& key, const V& value)
    {
        (void)value;
        result.push_back(key);
    }).m_status != LockFreeHashMapOk)
    {
        result.clear();
    }
    return result;
}

template <typename K, typename V, typename H, typename E>
std::vector<V> LockFreeHashMap<K, V, H, E>::values() const
{
    std::vector<V> result;
    // 借用 key、value，将值副本追加到 result；复制异常由遍历边界转换为失败
    if (forEachResult([&result](const K& key, const V& value)
    {
        (void)key;
        result.push_back(value);
    }).m_status != LockFreeHashMapOk)
    {
        result.clear();
    }
    return result;
}

template <typename K, typename V, typename H, typename E>
std::vector<std::pair<K, V>> LockFreeHashMap<K, V, H, E>::toVector() const
{
    std::vector<std::pair<K, V>> result;
    // 借用键值并将独立副本追加到 result，异常由遍历边界转换为失败
    if (forEachResult([&result](const K& key, const V& value)
    {
        result.push_back(std::make_pair(key, value));
    }).m_status != LockFreeHashMapOk)
    {
        result.clear();
    }
    return result;
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::bucketCount() const
{
    return m_bucketCount;
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::retainedNodeCount() const
{
    LockFreeHashMapPoolStats observation = {};
    poolStats(observation);
    return observation.occupied;
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::remember(Slot* node) const
{
    if (m_storage->m_lookup != nullptr)
    {
        size_t index = cacheIndex(lookupHash(*node->key(), node->m_hash), 0);
        if (m_storage->m_lookup[index].m_colliding.load(std::memory_order_relaxed))
        {
            return;
        }
        Slot* present = m_storage->m_lookup[index].m_node.load();
        if (present != nullptr && present != node)
        {
            index = cacheIndex(lookupHash(*node->key(), node->m_hash), 1);
        }
        std::atomic<Slot*>& cell = m_storage->m_lookup[index].m_node;
        // 调用方在发布和关闭状态复查期间始终保持节点保护
        cell.store(node);
        if (node->m_value.load() == nullptr || m_generation.load() != node->m_generation ||
            m_storage->m_lookup[cacheIndex(lookupHash(*node->key(), node->m_hash), 0)].m_colliding.load(std::memory_order_relaxed))
        {
            Slot* expected = node;
            cell.compare_exchange_strong(expected, nullptr);
        }
    }
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::usesReadCache() const
{
    return m_storage->m_inlineScalar && m_storage->m_lookup != nullptr &&
        (std::is_integral<K>::value || std::is_enum<K>::value) && sizeof(K) <= 4 &&
        std::is_scalar<V>::value && sizeof(V) <= 4 && std::is_same<E, std::equal_to<K>>::value;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::cachedRead(const K& key, size_t hash, V& value) const
{
    if (!usesReadCache())
    {
        return false;
    }
    Cache& cell = m_storage->m_lookup[lookupHash(key, hash) & (m_storage->m_config.lookupCacheCapacity - 1)];
    uint64_t cachedValue = cell.m_readValue.load();
    uint64_t stamp = cell.m_stamp.load();
    uint64_t sequence = stamp & UINT64_C(0xffffffff);
    if (sequence == 0 || ((stamp >> 32) & UINT64_C(0x7fffffff)) != m_generation.load() ||
        (cachedValue >> 32) != sequence)
    {
        return false;
    }
    uint64_t keyBits = LockFreeHashMapScalar<K>::encode(key);
    if ((stamp & UINT64_C(0x8000000000000000)) != 0)
    {
        // 直接索引标记绑定唯一值身份；此范围内缓存下标就是完整键，无需另读键字
        if (keyBits >= m_storage->m_config.lookupCacheCapacity)
        {
            return false;
        }
    }
    else
    {
        uint64_t cachedKey = cell.m_readKey.load();
        if ((cachedKey >> 32) != sequence || (cachedKey & UINT64_C(0xffffffff)) != keyBits)
        {
            return false;
        }
    }
    return LockFreeHashMapScalar<V>::copy(value, cachedValue & UINT64_C(0xffffffff));
}

template <typename K, typename V, typename H, typename E>
uint64_t LockFreeHashMap<K, V, H, E>::invalidateRead(const K& key, size_t hash, uint64_t generation, uint64_t version) const
{
    if (!usesReadCache() && !usesValueCache())
    {
        return 0;
    }
    // 完整 64 位条目身份仍是最终依据，超过标签范围只关闭缓存
    bool copied = usesReadCache();
    uint64_t generationLimit = copied ? UINT64_C(0x7fffffff) : UINT64_C(0xffffffff);
    uint64_t tag = generation != 0 && generation <= generationLimit &&
        version != 0 && version <= UINT64_C(0xffffffff) ? (generation << 32) | version : 0;
    if (tag != 0 && copied && LockFreeHashMapScalar<K>::encode(key) < m_storage->m_config.lookupCacheCapacity)
    {
        tag |= UINT64_C(0x8000000000000000);
    }
    m_storage->m_lookup[lookupHash(key, hash) & (m_storage->m_config.lookupCacheCapacity - 1)].m_stamp.store(tag,
        copied ? std::memory_order_release : std::memory_order_seq_cst);
    return tag;
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::publishRead(const K& key, size_t hash, const V& value, uint64_t tag) const
{
    if (tag == 0)
    {
        return;
    }
    Cache& cell = m_storage->m_lookup[lookupHash(key, hash) & (m_storage->m_config.lookupCacheCapacity - 1)];
    if (cell.m_stamp.load() != tag)
    {
        return;
    }
    uint64_t sequence = (tag & UINT64_C(0xffffffff)) << 32;
    // 每次提交的值身份唯一；迟到发布可挤掉副本，但不能使不匹配的键值版本和失效标签看起来一致
    if ((tag & UINT64_C(0x8000000000000000)) == 0)
    {
        cell.m_readKey.store(sequence | LockFreeHashMapScalar<K>::encode(key), std::memory_order_release);
    }
    cell.m_readValue.store(sequence | LockFreeHashMapScalar<V>::encode(value), std::memory_order_release);
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::lookupHash(const K& key, size_t hash) const
{
    // 只有内建等价关系允许独立键指纹，自定义等价仍用用户哈希，保留大小写折叠或模等价语义
    bool independent = (std::is_integral<K>::value || std::is_enum<K>::value) && sizeof(K) <= 4 &&
        !std::is_const<K>::value && !std::is_volatile<K>::value && std::is_same<E, std::equal_to<K>>::value;
    if (independent)
    {
        return static_cast<size_t>(LockFreeHashMapScalar<K>::encode(key));
    }
    return hash;
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::cacheIndex(size_t hash, size_t probe) const
{
    size_t mask = m_storage->m_config.lookupCacheCapacity - 1;
    size_t first = hash & mask;
    size_t second = (hash ^ (hash >> (sizeof(size_t) * 4))) & mask;
    return probe == 0 ? first : (second == first ? (first + 1) & mask : second);
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::helpValue(Slot* node, Slot* old, Slot* next) const
{
    Slot* expected = old;
    if (node->m_value.compare_exchange_strong(expected, next) && old != node)
    {
        m_storage->retire(old);
    }
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::captureValue(Slot* node, Slot*& value, LockFreeHashMapGuard& guard) const
{
    value = node->m_value.load();
    if (value == nullptr)
    {
        return false;
    }
    if (!m_storage->m_inlineScalar || value != node)
    {
        guard.protect(3, value);
        if (node->m_value.load() != value)
        {
            return false;
        }
    }
    if (m_storage->m_inlineScalar)
    {
        guard.m_scalar = value->m_scalar.load();
        if (guard.m_scalar.m_version == 0)
        {
            if (guard.m_scalar.m_bits != 0)
            {
                helpValue(node, value, reinterpret_cast<Slot*>(static_cast<uintptr_t>(guard.m_scalar.m_bits)));
            }
            else
            {
                closeNode(node);
            }
            return false;
        }
    }
    return true;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::updateScalar(const K& key, const V& input, LockFreeHashMapResult& result)
{
    if (m_status != LockFreeHashMapOk || !m_storage->m_inlineScalar || m_storage->m_lookup == nullptr)
    {
        return false;
    }
    bool builtinHash = std::is_integral<K>::value && sizeof(K) <= 4 &&
        std::is_same<H, std::hash<K>>::value && std::is_same<E, std::equal_to<K>>::value;
    size_t hash = 0;
    try
    {
        if (!builtinHash)
        {
            hash = m_hash(key);
        }
    }
    catch (...)
    {
        result = LockFreeHashMapResult(LockFreeHashMapUserException);
        return true;
    }
    if (m_storage->m_lookup[cacheIndex(lookupHash(key, hash), 0)].m_colliding.load(std::memory_order_relaxed))
    {
        return false;
    }
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
    if (guard.m_record == nullptr)
    {
        result = LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
        return true;
    }
    uint64_t generation = m_generation.load();
    if (generation == 0 || guard.m_record->m_nextVersion == 0 ||
        m_storage->m_emptySequence.load() == (std::numeric_limits<uint64_t>::max)())
    {
        return false;
    }
    if (generation > 1)
    {
        cleanBuckets(guard, 1);
        guard.resetLinks();
        guard.protect(3, nullptr);
    }
    try
    {
        for (size_t probe = 0; probe < 2; ++probe)
        {
            std::atomic<Slot*>& cell = m_storage->m_lookup[cacheIndex(lookupHash(key, hash), probe)].m_node;
            Slot* node = cell.load();
            if (node == nullptr)
            {
                continue;
            }
            guard.protect(guard.m_currentSlot, node);
            if (cell.load() != node || node->m_generation != generation || (!builtinHash && node->m_hash != hash))
            {
                continue;
            }
            if (!m_equal(*node->key(), key))
            {
                m_storage->m_lookup[cacheIndex(lookupHash(key, hash), 0)].m_colliding.store(true, std::memory_order_relaxed);
                return false;
            }
            Slot* old = nullptr;
            if (!captureValue(node, old, guard))
            {
                return false;
            }
            LockFreeHashMapScalarState desired = { LockFreeHashMapScalar<V>::encode(input), guard.m_record->m_nextVersion };
            if (!old->m_scalar.accepts(desired) || m_generation.load() != generation)
            {
                return false;
            }
            desired.m_version = nextVersion(guard);
            uint64_t tag = invalidateRead(key, hash, generation, desired.m_version);
            if (!old->m_scalar.compareExchange(guard.m_scalar, desired))
            {
                return false;
            }
            publishRead(key, hash, input, tag);
            result = LockFreeHashMapResult(LockFreeHashMapOk, true);
            if ((++guard.m_record->m_writeCount & 255) == 0)
            {
                guard.protect(guard.m_currentSlot, nullptr);
                guard.protect(3, nullptr);
                m_storage->reclaim(guard, 16);
            }
            return true;
        }
        if (builtinHash)
        {
            hash = m_hash(key);
        }
        Slot* node = nullptr;
        Slot* value = nullptr;
        uint64_t head = 0;
        LockFreeHashMapStatus searched = search(&key, hash, generation, guard, node, value, head, true);
        if (searched != LockFreeHashMapNotFound)
        {
            if (searched != LockFreeHashMapOk)
            {
                result = LockFreeHashMapResult(searched);
                return true;
            }
            return false;
        }
        if (m_generation.load() != generation)
        {
            result = LockFreeHashMapResult(LockFreeHashMapCleared);
            return true;
        }
        LockFreeHashMapScalarState desired = { LockFreeHashMapScalar<V>::encode(input), nextVersion(guard) };
        Slot* candidate = m_storage->acquire(1, guard, result.m_status);
        if (candidate == nullptr)
        {
            return true;
        }
        try
        {
            new (candidate->key()) K(key);
        }
        catch (const std::bad_alloc&)
        {
            m_storage->release(candidate, false);
            result = LockFreeHashMapResult(LockFreeHashMapNoMemory);
            return true;
        }
        catch (...)
        {
            m_storage->release(candidate, false);
            result = LockFreeHashMapResult(LockFreeHashMapUserException);
            return true;
        }
        candidate->m_hash = hash;
        candidate->m_generation = generation;
        candidate->m_scalar.initialize(desired);
        candidate->m_value.store(candidate, std::memory_order_relaxed);
        candidate->m_next.store((head & 1) != 0 ? 0 : static_cast<uintptr_t>(head), std::memory_order_relaxed);
        guard.protect(guard.m_currentSlot, candidate);
        uint64_t tag = invalidateRead(key, hash, generation, desired.m_version);
        if (!m_buckets[hash & (m_bucketCount - 1)].compare_exchange_strong(head, static_cast<uint64_t>(reinterpret_cast<uintptr_t>(candidate))))
        {
            m_storage->release(candidate, true);
            return false;
        }
        remember(candidate);
        publishRead(key, hash, input, tag);
        result = LockFreeHashMapResult(LockFreeHashMapOk, true);
        return true;
    }
    catch (...)
    {
        result = LockFreeHashMapResult(LockFreeHashMapUserException);
        return true;
    }
    return false;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::eraseScalar(const K& key, LockFreeHashMapResult& result)
{
    if (m_status != LockFreeHashMapOk || !m_storage->m_inlineScalar)
    {
        return false;
    }
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount, m_storage->m_recordSpan);
    if (guard.m_record == nullptr)
    {
        result = LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
        return true;
    }
    uint64_t generation = m_generation.load();
    if (generation == 0)
    {
        return false;
    }
    if (generation > 1)
    {
        cleanBuckets(guard, 1);
    }
    size_t hash = 0;
    try
    {
        hash = m_hash(key);
    }
    catch (...)
    {
        result = LockFreeHashMapResult(LockFreeHashMapUserException);
        return true;
    }
    Slot* node = nullptr;
    Slot* old = nullptr;
    uint64_t head = 0;
    result.m_status = search(&key, hash, generation, guard, node, old, head);
    if (m_generation.load() != generation)
    {
        result.m_status = LockFreeHashMapCleared;
        return true;
    }
    if (result.m_status != LockFreeHashMapOk)
    {
        return true;
    }
    LockFreeHashMapScalarState closed = {};
    invalidateRead(key, hash, generation, 0);
    if (!old->m_scalar.compareExchange(guard.m_scalar, closed))
    {
        return false;
    }
    uintptr_t next = closeNode(node);
    Slot* predecessor = static_cast<Slot*>(guard.m_record->m_pointer[guard.m_predecessorSlot].load());
    bool removed = unlinkNode(predecessor, hash & (m_bucketCount - 1), node, next);
    bool deferred = old != node && !m_storage->reclaimProtected(old, guard);
    if (removed && !m_storage->reclaimProtected(node, guard))
    {
        deferred = true;
    }
    for (size_t i = 0; i < 4; ++i)
    {
        guard.protect(i, nullptr);
    }
    if (deferred || (++guard.m_record->m_writeCount & 255) == 0)
    {
        m_storage->reclaim(guard, 16);
    }
    result = LockFreeHashMapResult(LockFreeHashMapOk, true);
    return true;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::usesValueCache() const
{
    return !m_storage->m_inlineScalar && m_storage->m_lookup != nullptr &&
        (std::is_integral<K>::value || std::is_enum<K>::value) && sizeof(K) <= 4 &&
        !std::is_const<K>::value && !std::is_volatile<K>::value && std::is_same<E, std::equal_to<K>>::value;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapStatus LockFreeHashMap<K, V, H, E>::cachedValueRead(const K& key, size_t hash, V& output) const
{
    if (!usesValueCache())
    {
        return LockFreeHashMapNotFound;
    }
    Cache& cell = m_storage->m_lookup[lookupHash(key, hash) & (m_storage->m_config.lookupCacheCapacity - 1)];
    Slot* value = cell.m_cachedValue.load();
    if (value == nullptr)
    {
        return LockFreeHashMapNotFound;
    }
    LockFreeHashMapDirectGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount,
        m_storage->m_recordSpan, value);
    if (guard.m_record == nullptr)
    {
        return LockFreeHashMapProtectionUnavailable;
    }
    if (cell.m_cachedValue.load() != value)
    {
        return LockFreeHashMapNotFound;
    }
    uint64_t tag = cell.m_stamp.load();
    if (tag == 0 || value->m_version != (tag & UINT64_C(0xffffffff)) ||
        value->m_generation != (tag >> 32) || value->m_generation != m_generation.load() ||
        LockFreeHashMapScalar<K>::encode(*value->key()) != LockFreeHashMapScalar<K>::encode(key))
    {
        return LockFreeHashMapNotFound;
    }
    try
    {
        output = *value->value();
        return LockFreeHashMapOk;
    }
    catch (const std::bad_alloc&)
    {
        return LockFreeHashMapNoMemory;
    }
    catch (...)
    {
        return LockFreeHashMapUserException;
    }
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::publishValue(const K& key, size_t hash, Slot* value, uint64_t tag) const
{
    if (tag == 0 || !usesValueCache())
    {
        return;
    }
    Cache& cell = m_storage->m_lookup[lookupHash(key, hash) & (m_storage->m_config.lookupCacheCapacity - 1)];
    if (cell.m_stamp.load() != tag)
    {
        return;
    }
    // 调用方在发布及标签和代次复查期间保持已源校验的值保护，迟到发布必须在放下保护前撤回
    cell.m_cachedValue.store(value);
    if (cell.m_stamp.load() != tag || m_generation.load() != value->m_generation)
    {
        Slot* expected = value;
        cell.m_cachedValue.compare_exchange_strong(expected, nullptr);
    }
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::forgetValue(const K& key, size_t hash, Slot* value) const
{
    if (usesValueCache())
    {
        Cache& cell = m_storage->m_lookup[lookupHash(key, hash) & (m_storage->m_config.lookupCacheCapacity - 1)];
        if (cell.m_cachedValue.load() == value)
        {
            Slot* expected = value;
            cell.m_cachedValue.compare_exchange_strong(expected, nullptr);
        }
    }
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::updateValue(const K& key, const V& input, LockFreeHashMapResult& result)
{
    if (m_status != LockFreeHashMapOk || m_storage->m_inlineScalar)
    {
        return false;
    }
    size_t hash = 0;
    try
    {
        hash = m_hash(key);
    }
    catch (...)
    {
        result = LockFreeHashMapResult(LockFreeHashMapUserException);
        return true;
    }
    Cache* hint = nullptr;
    Slot* hintedNode = nullptr;
    if (m_storage->m_lookup != nullptr)
    {
        hint = m_storage->m_lookup + cacheIndex(lookupHash(key, hash), 0);
        if (!hint->m_colliding.load(std::memory_order_relaxed))
        {
            hintedNode = hint->m_node.load();
        }
    }
    LockFreeHashMapGuard guard(m_storage->m_records, m_storage->m_config.protectionRecordCount,
        m_storage->m_recordSpan, hintedNode, true);
    if (guard.m_record == nullptr)
    {
        result = LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
        return true;
    }
    uint64_t generation = m_generation.load();
    if (generation == 0 || m_storage->m_emptySequence.load() == (std::numeric_limits<uint64_t>::max)())
    {
        result = LockFreeHashMapResult(LockFreeHashMapVersionExhausted);
        return true;
    }
    if (generation > 1)
    {
        cleanBuckets(guard, 1);
    }
    Slot* node = nullptr;
    Slot* old = nullptr;
    uint64_t head = 0;
    bool foundHint = false;
    try
    {
        // 接纳 CAS 已保护节点，不必再次执行非空保护发布
        if (hintedNode != nullptr && hint->m_node.load() == hintedNode &&
            hintedNode->m_generation == generation && hintedNode->m_hash == hash &&
            m_equal(*hintedNode->key(), key) && captureValue(hintedNode, old, guard))
        {
            node = hintedNode;
            foundHint = true;
        }
    }
    catch (...)
    {
        result.m_status = LockFreeHashMapUserException;
        return true;
    }
    if (!foundHint)
    {
        // 进入普通游标协议前，先放弃额外的直接节点保护
        guard.m_record->m_used.store(1);
        result.m_status = search(&key, hash, generation, guard, node, old, head);
    }
    else
    {
        result.m_status = LockFreeHashMapOk;
    }
    if (result.m_status == LockFreeHashMapNotFound)
    {
        return false;
    }
    if (result.m_status != LockFreeHashMapOk)
    {
        return true;
    }
    if (m_generation.load() != generation)
    {
        result.m_status = LockFreeHashMapCleared;
        return true;
    }
    uint64_t sequence = nextVersion(guard);
    if (sequence == 0)
    {
        result.m_status = LockFreeHashMapVersionExhausted;
        return true;
    }
    Slot* candidate = m_storage->acquire(2, guard, result.m_status);
    if (candidate == nullptr)
    {
        return true;
    }
    try
    {
        new (candidate->value()) V(input);
    }
    catch (const std::bad_alloc&)
    {
        m_storage->release(candidate, false);
        result.m_status = LockFreeHashMapNoMemory;
        return true;
    }
    catch (...)
    {
        m_storage->release(candidate, false);
        result.m_status = LockFreeHashMapUserException;
        return true;
    }
    candidate->m_version = sequence;
    if (usesValueCache())
    {
        new (candidate->key()) K(key);
        candidate->m_generation = generation;
        candidate->m_hash = hash;
    }
    bool warm = usesValueCache() && m_storage->m_lookup[lookupHash(key, hash) &
        (m_storage->m_config.lookupCacheCapacity - 1)].m_cachedValue.load() == old;
    if (warm)
    {
        guard.protect(0, candidate);
    }
    uint64_t tag = invalidateRead(key, hash, generation, sequence);
    Slot* expected = old;
    if (!node->m_value.compare_exchange_strong(expected, candidate))
    {
        m_storage->release(candidate, true);
        return false;
    }
    if (warm)
    {
        refreshValue(key, hash, old, candidate, tag);
    }
    else
    {
        forgetValue(key, hash, old);
    }
    for (size_t i = 0; i < 4; ++i)
    {
        guard.protect(i, nullptr);
    }
    m_storage->deferValue(old, guard);
    result = LockFreeHashMapResult(LockFreeHashMapOk, true);
    return true;
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::refreshValue(const K& key, size_t hash, Slot* old, Slot* value, uint64_t tag) const
{
    Cache& cell = m_storage->m_lookup[lookupHash(key, hash) & (m_storage->m_config.lookupCacheCapacity - 1)];
    if (tag == 0 || cell.m_stamp.load() != tag)
    {
        forgetValue(key, hash, old);
        return;
    }
    Slot* expected = old;
    if (cell.m_cachedValue.compare_exchange_strong(expected, value))
    {
        if (cell.m_stamp.load() != tag || m_generation.load() != value->m_generation)
        {
            expected = value;
            cell.m_cachedValue.compare_exchange_strong(expected, nullptr);
        }
    }
}