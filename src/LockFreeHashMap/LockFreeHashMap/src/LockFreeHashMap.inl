#pragma once
#include <algorithm>
#include <cstring>
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
    typedef std::integral_constant<bool, std::is_same<K, std::string>::value &&
        std::is_same<H, std::hash<K>>::value && std::is_same<E, std::equal_to<K>>::value> BuiltinString;
    return hashString(key, BuiltinString());
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::hashString(const K& key, std::false_type builtin) const
{
    (void)builtin;
    return m_hash(key);
}

template <typename K, typename V, typename H, typename E>
size_t LockFreeHashMap<K, V, H, E>::hashString(const K& key, std::true_type builtin) const
{
    (void)builtin;
    // 采用 Austin Appleby 公有领域 MurmurHash64A 的混合；按本库 size_t 长度和安全读取要求改写。
    // 来源：https://github.com/aappleby/smhasher/blob/master/src/MurmurHash2.cpp
    const uint64_t kMultiplier = UINT64_C(0xc6a4a7935bd1e995);
    size_t remaining = key.size();
    uint64_t result = static_cast<uint64_t>(remaining) * kMultiplier;
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(key.data());
    while (remaining >= sizeof(uint64_t))
    {
        // memcpy 支持非对齐字符串地址；不将字符数组强转解引用为整数对象。
        uint64_t word = 0;
        std::memcpy(&word, bytes, sizeof(word));
        word *= kMultiplier;
        word ^= word >> 47;
        word *= kMultiplier;
        result ^= word;
        result *= kMultiplier;
        bytes += sizeof(word);
        remaining -= sizeof(word);
    }
    if (remaining != 0)
    {
        // 尾部只访问实际剩余字节，不读取终止符或假设尾部存在可读填充。
        uint64_t tail = 0;
        for (size_t index = 0; index < remaining; ++index)
        {
            tail |= static_cast<uint64_t>(bytes[index]) << (index * 8);
        }
        result ^= tail;
        result *= kMultiplier;
    }
    result ^= result >> 47;
    result *= kMultiplier;
    result ^= result >> 47;
    return static_cast<size_t>(result ^ (result >> 32));
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
    if (std::alignment_of<K>::value > std::alignment_of<std::max_align_t>::value ||
        std::alignment_of<V>::value > std::alignment_of<std::max_align_t>::value ||
        !pointer.isLockFree() || !m_generation.is_lock_free() || !m_countValid.is_lock_free())
    {
        // C++11标准分配器不能保证扩展对齐；池内对齐不能替快照和导出数组提供保证。
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
LOCKFREEHASHMAP_FORCE_INLINE typename LockFreeHashMap<K, V, H, E>::Node* LockFreeHashMap<K, V, H, E>::read(const K& key, size_t hash, uint64_t generation) const
{
    if (generation == 0)
    {
        return nullptr;
    }
    uintptr_t address = m_buckets[hash & (m_bucketCount - 1)].load();
    const bool integralKey = std::is_integral<K>::value && std::is_same<E, std::equal_to<K>>::value;
    // 原子后继读取会阻止旧编译器跨迭代缓存外部引用；标准整数键只需读取一次。
    // 其他键仍借用原参数，不增加用户复制，也不改变自定义比较器观察到的参数对象。
    typename std::conditional<integralKey, const K, const K&>::type lookupKey = key;
    while (address != 0)
    {
        Node* node = reinterpret_cast<Slot*>(address)->payload();
        uintptr_t next = node->m_next.load();
        // 标准整数比较不执行用户代码；先排除异键，减少碰撞链上的代次和哈希检查。
        if (integralKey && !m_equal(node->m_key, lookupKey))
        {
            address = next & ~uintptr_t(3);
            continue;
        }
        // 整数完整键已经比较成功；合法哈希必须对等价键一致，不再重复读取哈希和比较键。
        if ((next & 3) == 0 && node->m_generation == generation &&
            (integralKey || (node->m_hash == hash && m_equal(node->m_key, lookupKey))))
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
const std::shared_ptr<const LockFreeHashMapIdentity>& LockFreeHashMap<K, V, H, E>::snapshotIdentity(LockFreeHashMapEpochRecord& record) const throw()
{
    if (!record.m_snapshotIdentity)
    {
        try
        {
            std::shared_ptr<LockFreeHashMapIdentityReference> owner =
                std::make_shared<LockFreeHashMapIdentityReference>(m_identity);
            std::shared_ptr<const LockFreeHashMapIdentity> alias(owner, m_identity.get());
            record.m_snapshotIdentity.swap(alias);
            record.m_snapshotBytes.store(sizeof(LockFreeHashMapIdentityReference), std::memory_order_relaxed);
        }
        catch (...)
        {
            // 这是有界的性能缓存；没有额外内存时仍可用原身份交付完整快照。
            return m_identity;
        }
    }
    return record.m_snapshotIdentity;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapStatus LockFreeHashMap<K, V, H, E>::snapshot(const Node& node, Snapshot& output, LockFreeHashMapEpochRecord& record) const
{
    try
    {
        Snapshot prepared;
        prepared.m_value = std::make_shared<V>(node.m_value);
        prepared.m_version.m_instance = snapshotIdentity(record);
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
            result = LockFreeHashMapResult(node == nullptr ? LockFreeHashMapNotFound : snapshot(*node, output, *guard.record()));
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
template <bool RetryContention, int32_t Mode, bool HasOutput>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::modifyProtected(const K& key, const V* value, size_t hash,
    const LockFreeHashMapVersion* version, Snapshot* output, LockFreeHashMapEpochGuard& guard)
{
    LockFreeHashMapEpochRecord& record = *guard.record();
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
        LockFreeHashMapStatus located = locate(std::addressof(key), hash, generation, found, link, head, RetryContention ? 0 : 1);
        if (located == LockFreeHashMapReservationBusy)
        {
            return LockFreeHashMapResult(located);
        }
        if (located == LockFreeHashMapCleared || m_generation.load() != generation)
        {
            return LockFreeHashMapResult(LockFreeHashMapCleared);
        }
        if (found != nullptr && Mode == 1)
        {
            if (HasOutput)
            {
                guard.track();
            }
            LockFreeHashMapStatus copied = !HasOutput ? LockFreeHashMapOk : snapshot(*found->payload(), prepared, record);
            if (copied == LockFreeHashMapOk && HasOutput)
            {
                swapSnapshot(*output, prepared);
            }
            return LockFreeHashMapResult(copied == LockFreeHashMapOk ? LockFreeHashMapExists : copied);
        }
        if (found == nullptr && Mode >= 2)
        {
            return LockFreeHashMapResult(LockFreeHashMapNotFound);
        }
        if (found != nullptr && version != nullptr && found->payload()->m_version != version->m_sequence)
        {
            return LockFreeHashMapResult(LockFreeHashMapVersionMismatch);
        }
        if (Mode >= 3)
        {
            Node* node = found->payload();
            uintptr_t next = node->m_next.load();
            if ((next & 3) != 0)
            {
                if (!RetryContention)
                {
                    return LockFreeHashMapResult(LockFreeHashMapReservationBusy);
                }
                contention.pause();
                continue;
            }
            if (HasOutput)
            {
                guard.track();
                LockFreeHashMapStatus copied = snapshot(*node, prepared, record);
                if (copied != LockFreeHashMapOk)
                {
                    return LockFreeHashMapResult(copied);
                }
            }
            m_cache.invalidate(key, 0);
            if (!node->m_next.compareExchangeStrong(next, next | 1))
            {
                if (!RetryContention)
                {
                    return LockFreeHashMapResult(LockFreeHashMapReservationBusy);
                }
                contention.pause();
                continue;
            }
            account(record, generation, -1);
            unlink(found, link, next, link == m_buckets + (hash & (m_bucketCount - 1)));
            if (HasOutput)
            {
                swapSnapshot(*output, prepared);
            }
            return LockFreeHashMapResult(LockFreeHashMapOk, true);
        }
        if (!candidate)
        {
            // 分配器、策略、键值复制及候选析构都可能重入，必须在首次准备候选之前登记。
            guard.track();
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
            if (HasOutput)
            {
                LockFreeHashMapStatus copied = snapshot(*candidate->payload(), prepared, record);
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
                if (!RetryContention)
                {
                    return LockFreeHashMapResult(LockFreeHashMapReservationBusy);
                }
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
                if (!RetryContention)
                {
                    return LockFreeHashMapResult(LockFreeHashMapReservationBusy);
                }
                contention.pause();
                continue;
            }
            candidate->payload()->m_next.store(next, std::memory_order_relaxed);
            // 同一个 CAS 冻结旧后继并发布替换，不覆盖并发物理摘除所改变的后继
            m_cache.invalidate(key, published->m_version);
            if (!node->m_next.compareExchangeStrong(next, address | 2))
            {
                if (!RetryContention)
                {
                    return LockFreeHashMapResult(LockFreeHashMapReservationBusy);
                }
                contention.pause();
                continue;
            }
            candidate.release();
            unlink(found, link, address, false);
        }
        m_cache.publish(published->m_key, published->m_value, generation, published->m_version);
        if (HasOutput)
        {
            swapSnapshot(*output, prepared);
        }
        return LockFreeHashMapResult(LockFreeHashMapOk, true);
    }
}

template <typename K, typename V, typename H, typename E>
template <bool RetryContention, int32_t Mode>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::modify(const K& key, const V* value,
    const LockFreeHashMapVersion* version, Snapshot* output)
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    try
    {
        size_t hash = hashKey(key, IntegralHash());
        size_t capacityAttempts = 0;
        LockFreeHashMapContention contention;
        for (;;)
        {
            LockFreeHashMapResult result;
            bool maintenance = false;
            bool pressure = false;
            {
                const bool standardKey = std::is_same<E, std::equal_to<K>>::value &&
                    (std::is_integral<K>::value || std::is_same<K, std::string>::value);
                // 标准键查找不执行用户代码，已存在插入和不交付值的删除可省去重入链写入。
                LockFreeHashMapEpochGuard guard(m_storage->epoch(), !standardKey);
                if (guard.record() == nullptr)
                {
                    return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
                }
                // 操作种类和是否交付快照在进入循环前固定，编译器可移除无关分支及空快照清理。
                result = output == nullptr ? modifyProtected<RetryContention, Mode, false>(key, value, hash, version, nullptr, guard) :
                    modifyProtected<RetryContention, Mode, true>(key, value, hash, version, output, guard);
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
            if (!result.m_committed && (result.m_status == LockFreeHashMapReservationBusy ||
                result.m_status == LockFreeHashMapCleared))
            {
                if (!RetryContention)
                {
                    return LockFreeHashMapResult(LockFreeHashMapReservationBusy);
                }
                // 退出保护并归还候选后重启；竞争不消耗资源恢复预算，也不更新条件版本。
                capacityAttempts = 0;
                contention.pause();
                hash = hashKey(key, IntegralHash());
                continue;
            }
            if (pressure && ++capacityAttempts < 3)
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
template <int32_t Mode>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::modifyRetry(const K& key, const V* value,
    const LockFreeHashMapVersion* version, Snapshot* output, bool retryContention, bool retryCapacity)
{
    uint32_t capacityRetries = 0;
    for (;;)
    {
        const LockFreeHashMapResult result = retryContention ? modify<true, Mode>(key, value, version, output) :
            modify<false, Mode>(key, value, version, output);
        if (result.m_committed)
        {
            return result;
        }
        if (!retryCapacity || (result.m_status != LockFreeHashMapCapacityLimit &&
            result.m_status != LockFreeHashMapProtectionUnavailable && result.m_status != LockFreeHashMapDirectoryFull &&
            result.m_status != LockFreeHashMapGrowthDeclined))
        {
            return result;
        }
        // 外层回调仍持有保护时，不能等待只有回调返回后才能归还的容量。
        if (m_storage != nullptr && LockFreeHashMapEpochGuard::heldByCurrentThread(m_storage->epoch()))
        {
            return result;
        }
        if (!LockFreeHashMapContention::waitCapacity(capacityRetries))
        {
            // 等待设施失败不改变本次未提交的真实资源原因。
            return result;
        }
    }
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryInsert(const K& key, const V& value, bool retryContention, bool retryCapacity)
{
    return modifyRetry<1>(key, std::addressof(value), nullptr, nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryInsertIfAbsent(const K& key, const V& value, Snapshot& output, bool retryContention, bool retryCapacity)
{
    return modifyRetry<1>(key, std::addressof(value), nullptr, &output, retryContention, retryCapacity);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryCompareExchange(const K& key, const LockFreeHashMapVersion& version, const V& value, Snapshot* output, bool retryContention, bool retryCapacity)
{
    return modifyRetry<2>(key, std::addressof(value), &version, output, retryContention, retryCapacity);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryEraseIf(const K& key, const LockFreeHashMapVersion& version, bool retryContention, bool retryCapacity)
{
    return modifyRetry<4>(key, nullptr, &version, nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryTake(const K& key, Snapshot& output, bool retryContention, bool retryCapacity)
{
    return modifyRetry<3>(key, nullptr, nullptr, &output, retryContention, retryCapacity);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapStatus LockFreeHashMap<K, V, H, E>::status() const
{
    return m_status;
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::trySet(const K& key, const V& value, Snapshot* output, bool retryContention, bool retryCapacity)
{
    return modifyRetry<0>(key, std::addressof(value), nullptr, output, retryContention, retryCapacity);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::insertIfAbsent(const K& key, const V& value, Snapshot& output)
{
    return modify<true, 1>(key, std::addressof(value), nullptr, &output);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::compareExchange(const K& key, const LockFreeHashMapVersion& version, const V& value, Snapshot* output)
{
    return modify<true, 2>(key, std::addressof(value), &version, output);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryErase(const K& key, bool retryContention, bool retryCapacity)
{
    return modifyRetry<3>(key, nullptr, nullptr, nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::eraseIf(const K& key, const LockFreeHashMapVersion& version)
{
    return modify<true, 4>(key, nullptr, &version, nullptr);
}

template <typename K, typename V, typename H, typename E>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::take(const K& key, Snapshot& output)
{
    return modify<true, 3>(key, nullptr, nullptr, &output);
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::set(const K& key, const V& value)
{
    return modify<true, 0>(key, std::addressof(value), nullptr, nullptr).m_committed;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::insert(const K& key, const V& value)
{
    return modify<true, 1>(key, std::addressof(value), nullptr, nullptr).m_committed;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::erase(const K& key)
{
    return modify<true, 3>(key, nullptr, nullptr, nullptr).m_committed;
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
LOCKFREEHASHMAP_FORCE_INLINE void LockFreeHashMap<K, V, H, E>::clear()
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
    return m_status == LockFreeHashMapOk && lookupValue(key, std::addressof(value), BuiltinHash());
}

template <typename K, typename V, typename H, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMap<K, V, H, E>::contains(const K& key) const
{
    // v120的is_copy_assignable可能误判已删除的赋值；仅对明确可赋值的常用类型复用旧路径。
    typedef std::integral_constant<bool, (std::is_scalar<V>::value && !std::is_const<V>::value) ||
        std::is_same<V, std::string>::value || LockFreeHashMapSharedRead<V>::value> CommonValue;
    return m_status == LockFreeHashMapOk && containsValue(key, CommonValue());
}

template <typename K, typename V, typename H, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMap<K, V, H, E>::containsValue(const K& key, std::true_type assignable) const
{
    (void)assignable;
    return lookupValue(key, nullptr, BuiltinHash());
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::containsValue(const K& key, std::false_type assignable) const
{
    (void)assignable;
    try
    {
        // 只查询节点而不实例化值赋值；不存在的桶不借用保护记录。
        size_t hash = hashKey(key, IntegralHash());
        if (m_buckets[hash & (m_bucketCount - 1)].load() == 0)
        {
            return false;
        }
        const bool standardKey = std::is_same<E, std::equal_to<K>>::value &&
            (std::is_integral<K>::value || std::is_same<K, std::string>::value);
        if (standardKey)
        {
            LockFreeHashMapReadGuard guard(m_storage->epoch());
            return guard.record() != nullptr && read(key, hash, m_generation.load()) != nullptr;
        }
        LockFreeHashMapEpochGuard guard(m_storage->epoch());
        return guard.record() != nullptr && read(key, hash, m_generation.load()) != nullptr;
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMap<K, V, H, E>::lookupValue(const K& key, V* value, std::true_type builtin) const
{
    (void)builtin;
    if (m_cache.lookup(key, m_generation, value))
    {
        return true;
    }
    // 哈希类型已确定不会调用业务逻辑；整数是否使用内建混合仍独立由 IntegralHash 决定。
    size_t hash = hashKey(key, IntegralHash());
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
            (m_buckets[hash & (m_bucketCount - 1)].load() != 0 && lookupUncached(key, hash, value));
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H, typename E>
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
bool LockFreeHashMap<K, V, H, E>::lookupUncached(const K& key, size_t hash, V* value) const
{
    // 缓存命中无需节点保护；把保护对象留在独立调用点，避免命中路径也承担其栈与清理成本。
    return lookupProtected(key, hash, value);
}

template <typename K, typename V, typename H, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMap<K, V, H, E>::lookupProtected(const K& key, size_t hash, V* value) const
{
    const bool standardKey = std::is_same<E, std::equal_to<K>>::value &&
        (std::is_integral<K>::value || std::is_same<K, std::string>::value);
    if (standardKey && (value == nullptr || std::is_trivially_copy_assignable<V>::value))
    {
        // 无业务回调的路径单独内联，不为不可能抛出的赋值生成通用异常清理边界。
        return lookupLeaf(key, hash, value);
    }
    typedef std::integral_constant<bool, standardKey && LockFreeHashMapSharedRead<V>::value> SharedRead;
    return lookupProtected(key, hash, value, SharedRead());
}

template <typename K, typename V, typename H, typename E>
LOCKFREEHASHMAP_FORCE_INLINE bool LockFreeHashMap<K, V, H, E>::lookupLeaf(const K& key, size_t hash, V* value) const
{
    LockFreeHashMapReadGuard guard(m_storage->epoch());
    if (guard.record() == nullptr)
    {
        return false;
    }
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

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::lookupProtected(const K& key, size_t hash, V* value, std::false_type shared) const
{
    (void)shared;
    const bool standardKey = std::is_same<E, std::equal_to<K>>::value &&
        (std::is_integral<K>::value || std::is_same<K, std::string>::value);
    if (standardKey && std::is_same<V, std::string>::value)
    {
        return lookupDeferred(key, hash, value);
    }
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
        if (value != nullptr && !copyReadCharacters(*value, node->m_value, typename std::is_same<V, std::string>::type()))
        {
            // 用户赋值和可能申请字符区的标准赋值均在执行前登记完整重入链。
            guard.track();
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
bool LockFreeHashMap<K, V, H, E>::lookupDeferred(const K& key, size_t hash, V* value) const
{
    LockFreeHashMapReadGuard guard(m_storage->epoch());
    if (guard.record() == nullptr)
    {
        return false;
    }
    Node* node = read(key, hash, m_generation.load());
    if (node == nullptr)
    {
        return false;
    }
    if (value == nullptr || copyReadCharacters(*value, node->m_value, typename std::is_same<V, std::string>::type()))
    {
        return true;
    }
    return assignDeferred(guard, *value, node->m_value);
}

template <typename K, typename V, typename H, typename E>
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
bool LockFreeHashMap<K, V, H, E>::assignDeferred(LockFreeHashMapReadGuard& guard, V& output, const V& source)
{
    // 转交期间不把epoch置零，不申请第二条记录；执行赋值前已建立同线程重入链。
    LockFreeHashMapEpochGuard callback(guard);
    try
    {
        output = source;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::copyReadCharacters(V& output, const V& source, std::false_type stringValue)
{
    (void)output;
    (void)source;
    (void)stringValue;
    return false;
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::copyReadCharacters(V& output, const V& source, std::true_type stringValue)
{
    (void)stringValue;
#if defined(_CPPLIB_VER) || defined(_LIBCPP_VERSION) || (defined(__GLIBCXX__) && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI)
    const size_t length = source.size();
    if (output.size() != length)
    {
        return false;
    }
    if (length != 0 && &output != &source)
    {
        // 这里仅接纳独占字符存储的标准库；不同字符串的字符区不重叠，同一对象直接成功。
        // 等长覆盖不触碰长度和终止符，copy避免为不可能出现的部分重叠选择移动路径。
        std::char_traits<char>::copy(&output[0], source.data(), length);
    }
    return true;
#else
    // 旧ABI的libstdc++或未知标准库可能在可写字符访问时解除共享并申请内存，保留完整赋值保护。
    (void)output;
    (void)source;
    return false;
#endif
}

template <typename K, typename V, typename H, typename E>
bool LockFreeHashMap<K, V, H, E>::lookupProtected(const K& key, size_t hash, V* value, std::true_type shared) const
{
    (void)shared;
    // 这里只执行标准键比较和共享计数增加；回收保护与通用路径完全相同。
    LockFreeHashMapReadGuard guard(m_storage->epoch(), std::true_type());
    if (guard.record() == nullptr)
    {
        return false;
    }
    Node* node = read(key, hash, m_generation.load());
    if (node == nullptr)
    {
        return false;
    }
    V prepared(node->m_value);
    // 已经取得独立拥有引用，之后只访问 prepared，不再借用节点。
    guard.release();
    value->swap(prepared);
    // prepared 现在拥有旧输出；其业务删除器即使重入，也不再占着本次记录。
    return true;
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
    // 固定保护域指针，循环边界与record取址紧邻，便于合并同一数量检查。
    LockFreeHashMapEpoch* const domain = m_storage->epoch();
    for (size_t i = 0; valid && i < domain->count(); ++i)
    {
        LockFreeHashMapEpochRecord* record = domain->record(i);
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
    /** 计数表示越界时仅由遍历框架累计条目，不保存引用
    @param [in] key 回调期间借用的键，不使用
    @param [in] value 回调期间借用的值，不使用
    */
    return forEach([](const K& key, const V& value)
    {
        (void)key;
        (void)value;
    });
}

template <typename K, typename V, typename H, typename E>
LOCKFREEHASHMAP_FORCE_INLINE LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::tryClear(bool retryContention, bool retryCapacity)
{
    // 重叠清空共享已完成的线性化点；不占容量，也没有未完成的竞争需要等待。
    (void)retryContention;
    (void)retryCapacity;
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
    result.lookupCacheBytes = m_cache.bytes();
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
template <bool Count, typename F>
LockFreeHashMapResult LockFreeHashMap<K, V, H, E>::forEachProtected(F&& function) const
{
    if (m_status != LockFreeHashMapOk)
    {
        return LockFreeHashMapResult(m_status);
    }
    // 把接纳扫描留在只读保护构造中，避免扩大遍历热点的寄存器占用。
    // 同一记录无间隙地转为回调保护，复制回调和执行用户代码前已登记重入链。
    LockFreeHashMapReadGuard admission(m_storage->epoch());
    LockFreeHashMapEpochGuard guard(admission);
    if (guard.record() == nullptr)
    {
        return LockFreeHashMapResult(LockFreeHashMapProtectionUnavailable);
    }
    // 回调副本位于保护对象的内层作用域，构造、执行和析构全部在保护期内。
    size_t count = 0;
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
            if (address == 0)
            {
                continue;
            }
            // 空桶已经排除；非空链在尾部判断后继，避免旧编译器为每个节点生成额外的回跳。
            do
            {
                Node* node = reinterpret_cast<Slot*>(address)->payload();
                uintptr_t next = node->m_next.load();
                if ((next & 3) == 0 && node->m_generation == generation)
                {
                    callback(node->m_key, node->m_value);
                    // 完成回调后再计数，异常时只报告已成功交付的条目。
                    if (Count)
                    {
                        ++count;
                    }
                }
                // 使用回调前的后继，不因回调替换当前条目而重复访问同键的新版本
                address = next & ~uintptr_t(3);
            } while (address != 0);
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
inline void LockFreeHashMap<K, V, H, E>::appendExport(std::vector<T>& output, const T& value)
{
    // 来源受遍历保护且不属于输出向量，直接交给构造路径，避免标量临时值取地址后再次读取。
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
    /** 复制当前键到独立输出，分配或复制异常交由遍历边界捕获
    @param [in] key 回调期间受保护的键，复制到捕获的output
    @param [in] value 回调期间受保护的值，不使用
    */
    LockFreeHashMapResult result = forEachProtected<false>([&](const K& key, const V& value)
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
    /** 复制当前值到独立输出，分配或复制异常交由遍历边界捕获
    @param [in] key 回调期间受保护的键，不使用
    @param [in] value 回调期间受保护的值，复制到捕获的output
    */
    LockFreeHashMapResult result = forEachProtected<false>([&](const K& key, const V& value)
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
    /** 将同次观察的键值复制到独立输出，分配或复制异常交由遍历边界捕获
    @param [in] key 回调期间受保护的键，与值一同构造到捕获的output
    @param [in] value 回调期间受保护的值，不在回调外保留节点引用
    */
    LockFreeHashMapResult result = forEachProtected<false>([&](const K& key, const V& value)
    {
        output.emplace_back(key, value);
    });
    if (result.m_status != LockFreeHashMapOk)
    {
        output.clear();
    }
    return output;
}