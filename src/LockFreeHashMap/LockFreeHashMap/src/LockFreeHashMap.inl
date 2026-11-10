#pragma once
#include <algorithm>
#include <limits>

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::hashKey(const K& key, std::true_type integral) const
{
    (void)integral;
    uint64_t bits = static_cast<uint64_t>(key);
    uint64_t mixed = (bits ^ (bits >> 32)) * UINT64_C(0x9e3779b97f4a7c15);
    return static_cast<size_t>(mixed ^ (mixed >> 32));
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::hashKey(const K& key, std::false_type integral) const
{
    (void)integral;
    return m_hash(key);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMap<K, V, H, E>::LockFreeHashMap(size_t bucketCount) :
LockFreeHashMap(bucketCount, LockFreeHashMapPoolConfig())
{

}

template <typename K, typename V, typename H, typename E>
LockFreeHashMap<K, V, H, E>::LockFreeHashMap(size_t bucketCount, const LockFreeHashMapPoolConfig& config) :
m_buckets(nullptr),
m_bucketCount(normalizeBucketCount(bucketCount)),
m_storage(nullptr),
m_generation(1),
m_sequence(0),
m_countValid(true),
m_status(LockFreeHashMapInvalidArgument),
m_maintenanceBucket(0),
m_cleanupBucket(0)
{
    if (m_bucketCount == 0)
    {
        return;
    }
    LockFreeHashMapLink pointer(0);
    if (!pointer.isLockFree() || !m_generation.is_lock_free() || !m_countValid.is_lock_free())
    {
        m_status = LockFreeHashMapUnsupported;
        return;
    }
    try
    {
        m_buckets = new (std::nothrow) LockFreeHashMapLink[m_bucketCount];
        if (m_buckets == nullptr)
        {
            m_status = LockFreeHashMapNoMemory;
            return;
        }
        // 链接构造已经初始化为零；整个数组仍私有，不重复执行原子存储。
        m_identity = std::make_shared<LockFreeHashMapIdentity>();
        m_storage = new (std::nothrow) Storage();
        m_status = m_storage == nullptr ? LockFreeHashMapNoMemory : m_storage->initialize(config);
        if (m_status == LockFreeHashMapOk)
        {
            m_status = m_cache.initialize(config.inlineScalarValues ? config.lookupCacheCapacity : 0);
        }
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
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::configurePool(const LockFreeHashMapPoolConfig& config)
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    if (!empty())
    {
        return LockFreeHashMapResult(LockFreeHashMapNotEmpty);
    }
    try
    {
        std::unique_ptr<Storage> replacement(new (std::nothrow) Storage());
        if (!replacement)
        {
            return LockFreeHashMapResult(LockFreeHashMapNoMemory);
        }
        LockFreeHashMapStatus result = replacement->initialize(config);
        if (result != LockFreeHashMapOk)
        {
            return LockFreeHashMapResult(result);
        }
        LockFreeHashMapValueCache<K, V, E> cache;
        result = cache.initialize(config.inlineScalarValues ? config.lookupCacheCapacity : 0);
        if (result != LockFreeHashMapOk)
        {
            return LockFreeHashMapResult(result);
        }
        clearAndFreeMemory();
        delete m_storage;
        m_storage = replacement.release();
        m_cache.swap(cache);
        m_countValid.store(true);
        return LockFreeHashMapResult();
    }
    catch (const std::bad_alloc&)
    {
        return LockFreeHashMapResult(LockFreeHashMapNoMemory);
    }
    catch (...)
    {
        return LockFreeHashMapResult(LockFreeHashMapUserException);
    }
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::setPoolPolicies(const LockFreeHashMapGrowPolicy& grow,
    const LockFreeHashMapShrinkPolicy& shrink)
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    m_storage->setPolicies(grow, shrink);
    return LockFreeHashMapResult();
}

template <typename K, typename V, typename H, typename E>
typename LockFreeHashMap<K, V, H, E>::Node* LockFreeHashMap<K, V, H, E>::read(const K& key, size_t hash, uint64_t generation) const
{
    if (generation == 0)
    {
        return nullptr;
    }
    uintptr_t address = m_buckets[hash & (m_bucketCount - 1)].load();
    while (address != 0)
    {
        Node* node = reinterpret_cast<Slot*>(address)->payload();
        uintptr_t next = node->m_next.load();
        // 标准整数比较不执行用户代码；先排除异键，减少碰撞链上的代次和哈希检查。
        if (std::is_integral<K>::value && std::is_same<E, std::equal_to<K>>::value && !m_equal(node->m_key, key))
        {
            address = next & ~uintptr_t(3);
            continue;
        }
        if ((next & 3) == 0 && node->m_generation == generation && node->m_hash == hash && m_equal(node->m_key, key))
        {
            return node;
        }
        address = next & ~uintptr_t(3);
    }
    return nullptr;
}

template <typename K, typename V, typename H, typename E>
int32_t LockFreeHashMap<K, V, H, E>::unlink(Slot* slot, LockFreeHashMapLink* link, uintptr_t& successor, bool bucketLink) const
{
    if (link == nullptr)
    {
        return 0;
    }
    if (bucketLink && (slot->payload()->m_next.load() & 3) == 1)
    {
        // 尚在结构中的头标记只记录待摘除时间，不设置池的退役位，因此回收者不能提前析构
        uint64_t pending = slot->m_retired.load();
        if (pending == 0)
        {
            uint64_t current = m_storage->epoch()->current();
            if (slot->m_retired.compare_exchange_strong(pending, current))
            {
                return 2;
            }
        }
        if (pending >= m_storage->epoch()->current())
        {
            return 2;
        }
        uint64_t minimum = m_storage->epoch()->minimum();
        if (minimum != 0 && minimum <= pending)
        {
            return 2;
        }
        while (successor != 0)
        {
            uintptr_t next = reinterpret_cast<Slot*>(successor)->payload()->m_next.load();
            if ((next & 3) == 0)
            {
                break;
            }
            successor = next & ~uintptr_t(3);
        }
    }
    uintptr_t expected = reinterpret_cast<uintptr_t>(slot);
    if (!link->compareExchangeStrong(expected, successor))
    {
        return 0;
    }
    // 物理摘除后重新取退休代际；在等待空桶复用期间进入的新读者也必须受到保护
    uintptr_t address = reinterpret_cast<uintptr_t>(slot);
    while (address != successor)
    {
        Slot* removed = reinterpret_cast<Slot*>(address);
        address = removed->payload()->m_next.load() & ~uintptr_t(3);
        m_storage->retire(removed);
    }
    return 1;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapStatus LockFreeHashMap<K, V, H, E>::locate(const K* key, size_t hash, uint64_t generation,
    Slot*& found, LockFreeHashMapLink*& link, uintptr_t& head, size_t retryLimit) const
{
    size_t bucket = hash & (m_bucketCount - 1);
    size_t retries = 0;
    LockFreeHashMapContention contention;
    for (;;)
    {
        found = nullptr;
        link = m_buckets + bucket;
        head = link->load();
        uintptr_t address = head;
        bool retry = false;
        while (address != 0)
        {
            Slot* slot = reinterpret_cast<Slot*>(address);
            Node* node = slot->payload();
            uintptr_t next = node->m_next.load();
            if ((next & 3) == 0 && (generation == 0 || node->m_generation < generation))
            {
                // 旧代关闭与普通删除使用相同不可逆标记，不复活被遮蔽的旧值
                uintptr_t desired = next | 1;
                if (!node->m_next.compareExchangeStrong(next, desired))
                {
                    retry = true;
                    break;
                }
                next = desired;
            }
            if ((next & 3) != 0)
            {
                uintptr_t successor = next & ~uintptr_t(3);
                if (link == nullptr)
                {
                    address = successor;
                    continue;
                }
                int32_t removed = unlink(slot, link, successor, link == m_buckets + bucket);
                if (removed == 2)
                {
                    // 只读越过尚不可摘除的头标记；恢复旧的非空前缀同样会产生缺失观察 ABA
                    link = nullptr;
                    address = successor;
                    continue;
                }
                if (removed == 0)
                {
                    retry = true;
                    break;
                }
                if (link == m_buckets + bucket)
                {
                    head = successor;
                }
                address = successor;
                continue;
            }
            if (node->m_generation > generation)
            {
                return LockFreeHashMapCleared;
            }
            if (key != nullptr && node->m_hash == hash && m_equal(node->m_key, *key))
            {
                found = slot;
                return LockFreeHashMapOk;
            }
            link = &node->m_next;
            address = next;
        }
        if (!retry)
        {
            return LockFreeHashMapNotFound;
        }
        if (retryLimit != 0 && ++retries >= retryLimit)
        {
            // 仅结束本轮附带维护，不能作为用户数据操作的失败结果
            return LockFreeHashMapReservationBusy;
        }
        contention.pause();
    }
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::swapSnapshot(Snapshot& left, Snapshot& right) throw()
{
    left.m_value.swap(right.m_value);
    left.m_version.m_instance.swap(right.m_version.m_instance);
    std::swap(left.m_version.m_sequence, right.m_version.m_sequence);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapStatus LockFreeHashMap<K, V, H, E>::snapshot(const Node& node, Snapshot& output) const
{
    try
    {
        Snapshot prepared;
        prepared.m_value = std::make_shared<V>(node.m_value);
        prepared.m_version.m_instance = m_identity;
        prepared.m_version.m_sequence = node.m_version;
        // 临时快照交付后即销毁，交换所有权避免 shared_ptr 的额外增减计数。
        swapSnapshot(output, prepared);
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
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::find(const K& key, Snapshot& output) const
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    LockFreeHashMapResult result;
    {
        LockFreeHashMapEpochGuard guard(m_storage->epoch());
        if (guard.record() == nullptr)
        {
            return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
        }
        try
        {
            Node* node = read(key, hashKey(key, IntegralHash()), m_generation.load());
            result = LockFreeHashMapResult(node == nullptr ? LockFreeHashMapNotFound : snapshot(*node, output));
        }
        catch (...)
        {
            return LockFreeHashMapResult(LockFreeHashMapUserException);
        }
    }
    return result;
}

template <typename K, typename V, typename H, typename E>
uint64_t LockFreeHashMap<K, V, H, E>::nextVersion(LockFreeHashMapEpochRecord& record)
{
    if (record.m_versionNext < record.m_versionEnd)
    {
        return ++record.m_versionNext;
    }
    uint64_t current = m_sequence.load(std::memory_order_relaxed);
    const uint64_t maximum = (std::numeric_limits<uint64_t>::max)();
    LockFreeHashMapContention contention;
    while (current != maximum)
    {
        uint64_t count = (std::min)(uint64_t(256), maximum - current);
        if (m_sequence.compare_exchange_weak(current, current + count))
        {
            record.m_versionEnd = current + count;
            record.m_versionNext = current + 1;
            return record.m_versionNext;
        }
        contention.pause();
    }
    return 0;
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::account(LockFreeHashMapEpochRecord& record, uint64_t generation, int64_t difference)
{
    if (record.m_countGeneration.load(std::memory_order_relaxed) != generation)
    {
        record.m_countDelta.store(0, std::memory_order_relaxed);
        record.m_countGeneration.store(generation, std::memory_order_release);
    }
    int64_t current = record.m_countDelta.load(std::memory_order_relaxed);
    if ((difference > 0 && current == (std::numeric_limits<int64_t>::max)()) ||
        (difference < 0 && current == (std::numeric_limits<int64_t>::min)()))
    {
        m_countValid.store(false);
        return;
    }
    record.m_countDelta.store(current + difference, std::memory_order_relaxed);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::modifyProtected(const K& key, const V* value, size_t hash, int32_t mode,
    const LockFreeHashMapVersion* version, Snapshot* output, LockFreeHashMapEpochRecord& record)
{
    uint64_t generation = m_generation.load();
    if (generation == 0)
    {
        return LockFreeHashMapResult(LockFreeHashMapVersionExhausted);
    }
    if (version != nullptr && (version->m_instance.get() != m_identity.get() || version->m_sequence == 0))
    {
        return LockFreeHashMapResult(LockFreeHashMapVersionMismatch);
    }
    std::unique_ptr<Slot, LockFreeHashMapPoolReturn<Node>> candidate(nullptr, LockFreeHashMapPoolReturn<Node>(m_storage));
    Snapshot prepared;
    LockFreeHashMapContention contention;
    for (;;)
    {
        Slot* found = nullptr;
        LockFreeHashMapLink* link = nullptr;
        uintptr_t head = 0;
        LockFreeHashMapStatus located = locate(&key, hash, generation, found, link, head);
        if (located == LockFreeHashMapCleared || m_generation.load() != generation)
        {
            return LockFreeHashMapResult(LockFreeHashMapCleared);
        }
        if (found != nullptr && mode == 1)
        {
            LockFreeHashMapStatus copied = output == nullptr ? LockFreeHashMapOk : snapshot(*found->payload(), prepared);
            if (copied == LockFreeHashMapOk && output != nullptr)
            {
                swapSnapshot(*output, prepared);
            }
            return LockFreeHashMapResult(copied == LockFreeHashMapOk ? LockFreeHashMapExists : copied);
        }
        if (found == nullptr && mode >= 2)
        {
            return LockFreeHashMapResult(LockFreeHashMapNotFound);
        }
        if (found != nullptr && version != nullptr && found->payload()->m_version != version->m_sequence)
        {
            return LockFreeHashMapResult(LockFreeHashMapVersionMismatch);
        }
        if (mode >= 3)
        {
            Node* node = found->payload();
            uintptr_t next = node->m_next.load();
            if ((next & 3) != 0)
            {
                contention.pause();
                continue;
            }
            if (output != nullptr)
            {
                LockFreeHashMapStatus copied = snapshot(*node, prepared);
                if (copied != LockFreeHashMapOk)
                {
                    return LockFreeHashMapResult(copied);
                }
            }
            m_cache.invalidate(key, 0);
            if (!node->m_next.compareExchangeStrong(next, next | 1))
            {
                contention.pause();
                continue;
            }
            account(record, generation, -1);
            unlink(found, link, next, link == m_buckets + (hash & (m_bucketCount - 1)));
            if (output != nullptr)
            {
                swapSnapshot(*output, prepared);
            }
            return LockFreeHashMapResult(LockFreeHashMapOk, true);
        }
        if (!candidate)
        {
            uint64_t sequence = nextVersion(record);
            if (sequence == 0)
            {
                return LockFreeHashMapResult(LockFreeHashMapVersionExhausted);
            }
            LockFreeHashMapStatus allocation;
            candidate.reset(m_storage->acquire(record, allocation));
            if (!candidate)
            {
                return LockFreeHashMapResult(allocation);
            }
            new (candidate->payload()) Node(key, *value, hash, generation, sequence);
            candidate->m_kind.store(1, std::memory_order_relaxed);
            if (output != nullptr)
            {
                LockFreeHashMapStatus copied = snapshot(*candidate->payload(), prepared);
                if (copied != LockFreeHashMapOk)
                {
                    return LockFreeHashMapResult(copied);
                }
            }
        }
        uintptr_t address = reinterpret_cast<uintptr_t>(candidate.get());
        Node* published = candidate->payload();
        if (found == nullptr)
        {
            uintptr_t successor = head;
            while (successor != 0)
            {
                uintptr_t next = reinterpret_cast<Slot*>(successor)->payload()->m_next.load();
                if ((next & 3) == 0)
                {
                    break;
                }
                successor = next & ~uintptr_t(3);
            }
            candidate->payload()->m_next.store(successor, std::memory_order_relaxed);
            m_cache.invalidate(key, published->m_version);
            if (!m_buckets[hash & (m_bucketCount - 1)].compareExchangeStrong(head, address))
            {
                contention.pause();
                continue;
            }
            candidate.release();
            while (head != successor)
            {
                Slot* discarded = reinterpret_cast<Slot*>(head);
                head = discarded->payload()->m_next.load() & ~uintptr_t(3);
                m_storage->retire(discarded);
            }
            account(record, generation, 1);
        }
        else
        {
            Node* node = found->payload();
            uintptr_t next = node->m_next.load();
            if ((next & 3) != 0)
            {
                contention.pause();
                continue;
            }
            candidate->payload()->m_next.store(next, std::memory_order_relaxed);
            // 同一个 CAS 冻结旧后继并发布替换，不覆盖并发物理摘除所改变的后继
            m_cache.invalidate(key, published->m_version);
            if (!node->m_next.compareExchangeStrong(next, address | 2))
            {
                contention.pause();
                continue;
            }
            candidate.release();
            unlink(found, link, address, false);
        }
        m_cache.publish(published->m_key, published->m_value, generation, published->m_version);
        if (output != nullptr)
        {
            swapSnapshot(*output, prepared);
        }
        return LockFreeHashMapResult(LockFreeHashMapOk, true);
    }
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::modify(const K& key, const V* value, int32_t mode,
    const LockFreeHashMapVersion* version, Snapshot* output)
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    try
    {
        size_t hash = hashKey(key, IntegralHash());
        for (size_t attempt = 0; attempt < 3; ++attempt)
        {
            LockFreeHashMapResult result;
            bool maintenance = false;
            bool pressure = false;
            {
                LockFreeHashMapEpochGuard guard(m_storage->epoch());
                if (guard.record() == nullptr)
                {
                    return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
                }
                result = modifyProtected(key, value, hash, mode, version, output, *guard.record());
                // 常规写入合并维护成本；容量拒绝仍立即清理全部桶并保留完整回收预算
                maintenance = (++guard.record()->m_operations & 63) == 0;
                pressure = result.m_status == LockFreeHashMapCapacityLimit;
                if (maintenance || pressure)
                {
                    // 容量压力下不能只检查当前热桶，空闲的冷桶末尾标记也必须有机会归还预算
                    size_t count = pressure ? m_bucketCount : 1;
                    size_t start = m_cleanupBucket.fetch_add(count, std::memory_order_relaxed);
                    for (size_t i = 0; i < count; ++i)
                    {
                        size_t bucket = (start + i) & (m_bucketCount - 1);
                        Slot* ignored = nullptr;
                        LockFreeHashMapLink* link = nullptr;
                        uintptr_t head = 0;
                        locate(nullptr, bucket, m_generation.load(), ignored, link, head, 2);
                    }
                }
            }
            if (maintenance || pressure)
            {
                m_storage->reclaim(pressure ? 64 : 16);
            }
            if (attempt < 2 && pressure)
            {
                continue;
            }
            return result;
        }
    }
    catch (const std::bad_alloc&)
    {
        return LockFreeHashMapResult(LockFreeHashMapNoMemory);
    }
    catch (...)
    {
        return LockFreeHashMapResult(LockFreeHashMapUserException);
    }
    return LockFreeHashMapResult(LockFreeHashMapCapacityLimit);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapStatus LockFreeHashMap<K, V, H, E>::status() const
{
    return m_status;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::trySet(const K& key, const V& value, Snapshot* output)
{
    return modify(key, &value, 0, nullptr, output);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::insertIfAbsent(const K& key, const V& value, Snapshot& output)
{
    return modify(key, &value, 1, nullptr, &output);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::compareExchange(const K& key, const LockFreeHashMapVersion& version, const V& value, Snapshot* output)
{
    return modify(key, &value, 2, &version, output);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryErase(const K& key)
{
    return modify(key, nullptr, 3, nullptr, nullptr);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::eraseIf(const K& key, const LockFreeHashMapVersion& version)
{
    return modify(key, nullptr, 4, &version, nullptr);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::take(const K& key, Snapshot& output)
{
    return modify(key, nullptr, 3, nullptr, &output);
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
size_t LockFreeHashMap<K, V, H, E>::count(const K& key) const
{
    return contains(key) ? 1 : 0;
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
size_t LockFreeHashMap<K, V, H, E>::bucketCount() const
{
    return m_bucketCount;
}

template <typename K, typename V, typename H, typename E>
HashMapValueProxy<K, V, H, E> LockFreeHashMap<K, V, H, E>::operator[](const K& key)
{
    return HashMapValueProxy<K, V, H, E>(this, key);
}

template <typename K, typename V, typename H, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMap<K, V, H, E>::get(const K& key, V& value) const
{
    return m_status == LockFreeHashMapOk && lookupValue(key, &value, IntegralHash());
}

template <typename K, typename V, typename H, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMap<K, V, H, E>::contains(const K& key) const
{
    return m_status == LockFreeHashMapOk && lookupValue(key, nullptr, IntegralHash());
}

template <typename K, typename V, typename H, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMap<K, V, H, E>::lookupValue(const K& key, V* value, std::true_type integral) const
{
    if (m_cache.lookup(key, m_generation, value))
    {
        return true;
    }
    size_t hash = hashKey(key, integral);
    // 桶数组固定存活，观察到零即可在线性化点返回未找到，不需要保护任何节点
    return m_buckets[hash & (m_bucketCount - 1)].load() != 0 && lookupProtected(key, hash, value);
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::lookupValue(const K& key, V* value, std::false_type integral) const
{
    try
    {
        size_t hash = hashKey(key, integral);
        // 未启用整数副本时不读取无用途的代次，真正节点读取仍在保护内核对代次。
        return m_cache.lookup(key, m_generation, value) ||
            (m_buckets[hash & (m_bucketCount - 1)].load() != 0 && lookupProtected(key, hash, value));
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::lookupProtected(const K& key, size_t hash, V* value) const
{
    LockFreeHashMapEpochGuard guard(m_storage->epoch());
    if (guard.record() == nullptr)
    {
        return false;
    }
    try
    {
        Node* node = read(key, hash, m_generation.load());
        if (node == nullptr)
        {
            return false;
        }
        if (value != nullptr)
        {
            *value = node->m_value;
        }
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H, typename E>
V LockFreeHashMap<K, V, H, E>::value(const K& key, const V& defaultValue) const
{
    V result(defaultValue);
    get(key, result);
    return result;
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::count() const
{
    if (m_status != LockFreeHashMapOk)
    {
        return 0;
    }
    uint64_t generation = m_generation.load();
    if (generation == 0)
    {
        return 0;
    }
    int64_t total = 0;
    bool valid = m_countValid.load();
    for (size_t i = 0; valid && i < m_storage->epoch()->count(); ++i)
    {
        LockFreeHashMapEpochRecord* record = m_storage->epoch()->record(i);
        uint64_t before = record->m_countGeneration.load();
        // 未参与本代的记录不贡献计数，无需额外执行差量的原子读取。
        if (before != generation)
        {
            continue;
        }
        int64_t delta = record->m_countDelta.load();
        if (record->m_countGeneration.load() != generation)
        {
            continue;
        }
        if ((delta > 0 && total > (std::numeric_limits<int64_t>::max)() - delta) ||
            (delta < 0 && total < (std::numeric_limits<int64_t>::min)() - delta))
        {
            valid = false;
            break;
        }
        total += delta;
    }
    if (valid && (total <= 0 || static_cast<uint64_t>(total) <= (std::numeric_limits<size_t>::max)()))
    {
        return total > 0 ? static_cast<size_t>(total) : 0;
    }
    // 表示边界回退到受保护的向前遍历，回调不借出引用
    return forEach([](const K& key, const V& value)
    {
        (void)key;
        (void)value;
    });
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryClear()
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    uint64_t current = m_generation.load();
    if (current != 0)
    {
        uint64_t next = current == (std::numeric_limits<uint64_t>::max)() ? 0 : current + 1;
        if (m_generation.compare_exchange_strong(current, next))
        {
            return LockFreeHashMapResult(next == 0 ? LockFreeHashMapVersionExhausted : LockFreeHashMapOk, true);
        }
        // 重叠调用共用期间已发生的 clear 作为线性化点，无需争抢另一个代次
        return LockFreeHashMapResult(current == 0 ? LockFreeHashMapVersionExhausted : LockFreeHashMapOk, true);
    }
    return LockFreeHashMapResult(LockFreeHashMapVersionExhausted);
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::clearAndFreeMemory()
{
    tryClear();
    if (m_buckets != nullptr)
    {
        for (size_t i = 0; i < m_bucketCount; ++i)
        {
            // 本接口要求外部静止，独立快照不借用桶指针。
            m_buckets[i].store(0, std::memory_order_relaxed);
        }
    }
    if (m_storage != nullptr)
    {
        m_storage->freeBlocks();
    }
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::maintain(size_t workBudget)
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    if (workBudget == 0)
    {
        return LockFreeHashMapResult();
    }
    {
        LockFreeHashMapEpochGuard guard(m_storage->epoch());
        if (guard.record() == nullptr)
        {
            return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
        }
        size_t limit = (std::min)(workBudget, m_bucketCount);
        for (size_t i = 0; i < limit; ++i)
        {
            size_t bucket = m_maintenanceBucket++ & (m_bucketCount - 1);
            Slot* ignored = nullptr;
            LockFreeHashMapLink* link = nullptr;
            uintptr_t head = 0;
            locate(nullptr, bucket, m_generation.load(), ignored, link, head, 2);
        }
    }
    m_storage->reclaim(workBudget);
    return m_storage->shrink();
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::poolStats(LockFreeHashMapPoolStats& output) const
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    LockFreeHashMapPoolStats result = m_storage->stats();
    if (result.protectionRecords == 0)
    {
        return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
    }
    result.retainedBytes += sizeof(*this) + sizeof(LockFreeHashMapIdentity) + m_bucketCount * sizeof(LockFreeHashMapLink);
    result.lookupCacheEntries = m_cache.count();
    result.lookupCacheBytes = m_cache.count() * sizeof(LockFreeHashMapValueCacheEntry);
    result.retainedBytes += result.lookupCacheBytes;
    result.inlineScalarValues = m_cache.count() != 0;
    output = result;
    return LockFreeHashMapResult();
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::retainedNodeCount() const
{
    LockFreeHashMapPoolStats stats = {};
    return poolStats(stats).m_status == LockFreeHashMapOk ? stats.occupied : 0;
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::normalizeBucketCount(size_t count)
{
    if (count == 0)
    {
        count = 1024;
    }
    size_t maximum = (std::numeric_limits<size_t>::max)() / sizeof(LockFreeHashMapLink);
    size_t normalized = 1;
    while (normalized < count && normalized <= maximum / 2)
    {
        normalized <<= 1;
    }
    return normalized >= count && normalized <= maximum ? normalized : 0;
}

template <typename K, typename V, typename H, typename E>
template <typename F>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::forEachProtected(F&& function) const
{
    size_t count = 0;
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    LockFreeHashMapEpochGuard guard(m_storage->epoch());
    if (guard.record() == nullptr)
    {
        return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
    }
    try
    {
        typename std::decay<F>::type callback(std::forward<F>(function));
        uint64_t generation = m_generation.load();
        if (generation == 0)
        {
            return LockFreeHashMapResult();
        }
        // 并发数据操作不改变桶目录及桶数；回调允许修改条目，但配置和析构仍要求外部静止。
        // 提前保存固定目录，避免每遍历一个空桶都因回调别名重新读取容器字段。
        LockFreeHashMapLink* const end = m_buckets + m_bucketCount;
        for (LockFreeHashMapLink* bucket = m_buckets; bucket != end; ++bucket)
        {
            uintptr_t address = bucket->load();
            while (address != 0)
            {
                Node* node = reinterpret_cast<Slot*>(address)->payload();
                uintptr_t next = node->m_next.load();
                if ((next & 3) == 0 && node->m_generation == generation)
                {
                    callback(node->m_key, node->m_value);
                    // 完成回调后再计数，异常时只报告已成功交付的条目。
                    ++count;
                }
                // 使用回调前的后继，不因回调替换当前条目而重复访问同键的新版本
                address = next & ~uintptr_t(3);
            }
        }
        return LockFreeHashMapResult(LockFreeHashMapOk, false, count);
    }
    catch (...)
    {
        return LockFreeHashMapResult(LockFreeHashMapUserException, false, count);
    }
}

template <typename K, typename V, typename H, typename E>
template <typename F>
size_t LockFreeHashMap<K, V, H, E>::forEach(const F& function) const
{
    return forEachProtected(function).m_count;
}

template <typename K, typename V, typename H, typename E>
template <typename F>
size_t LockFreeHashMap<K, V, H, E>::forEach(F&& function) const
{
    return forEachProtected(std::forward<F>(function)).m_count;
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
template <typename T>
void LockFreeHashMap<K, V, H, E>::appendExport(std::vector<T>& output, const T& value)
{
    // 节点载荷和结果向量不重叠，省去 v120 push_back 为自引用执行的地址范围判断。
    output.emplace_back(value);
}

template <typename K, typename V, typename H, typename E>
void LockFreeHashMap<K, V, H, E>::appendExport(std::vector<bool>& output, const bool& value)
{
    output.push_back(value);
}

template <typename K, typename V, typename H, typename E>
std::vector<K> LockFreeHashMap<K, V, H, E>::keys() const
{
    std::vector<K> output;
    try
    {
        output.reserve(count());
    }
    catch (...)
    {
        return output;
    }
    // 回调复制到独立结果向量，分配或复制异常由遍历边界捕获
    LockFreeHashMapResult result = forEachProtected([&](const K& key, const V& value)
    {
        (void)value;
        appendExport(output, key);
    });
    if (result.m_status != LockFreeHashMapOk)
    {
        output.clear();
    }
    return output;
}

template <typename K, typename V, typename H, typename E>
std::vector<V> LockFreeHashMap<K, V, H, E>::values() const
{
    std::vector<V> output;
    try
    {
        output.reserve(count());
    }
    catch (...)
    {
        return output;
    }
    // 回调复制到独立结果向量，分配或复制异常由遍历边界捕获
    LockFreeHashMapResult result = forEachProtected([&](const K& key, const V& value)
    {
        (void)key;
        appendExport(output, value);
    });
    if (result.m_status != LockFreeHashMapOk)
    {
        output.clear();
    }
    return output;
}

template <typename K, typename V, typename H, typename E>
std::vector<std::pair<K, V>> LockFreeHashMap<K, V, H, E>::toVector() const
{
    std::vector<std::pair<K, V>> output;
    try
    {
        output.reserve(count());
    }
    catch (...)
    {
        return output;
    }
    // 回调复制到独立结果向量，分配或复制异常由遍历边界捕获
    LockFreeHashMapResult result = forEachProtected([&](const K& key, const V& value)
    {
        output.emplace_back(key, value);
    });
    if (result.m_status != LockFreeHashMapOk)
    {
        output.clear();
    }
    return output;
}