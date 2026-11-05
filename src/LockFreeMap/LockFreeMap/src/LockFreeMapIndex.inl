#ifndef LOCK_FREE_MAP_INDEX_INL
#define LOCK_FREE_MAP_INDEX_INL

template <typename K, typename V>
LockFreeMapKey<K, V>::LockFreeMapKey(const K& key, LockFreeMapPool<V>& pool) :
m_key(key),
m_value(nullptr),
m_pool(&pool)
{

}

template <typename K, typename V>
LockFreeMapKey<K, V>::LockFreeMapKey(const K& key, LockFreeMapPool<V>& pool, LockFreeMapPoolSlot<V>* value) :
m_key(key),
m_value(value),
m_pool(&pool)
{

}

template <typename K, typename V>
LockFreeMapKey<K, V>::LockFreeMapKey(const LockFreeMapKey& other) :
m_key(other.m_key),
m_value(nullptr),
m_pool(other.m_pool)
{

}

template <typename K, typename V>
LockFreeMapKey<K, V>::~LockFreeMapKey()
{
#ifdef __EMSCRIPTEN__
    // 末个拥有者已显式归还值时，只需继续执行键成员的正常析构
    if (m_pool == nullptr)
    {
        return;
    }
#endif
    LockFreeMapPoolSlot<V>* value = m_value.load();
    if (value != nullptr)
    {
        m_pool->release(value);
    }
}

template <typename K, typename V>
void LockFreeMapPoolDestroy<LockFreeMapKey<K, V>>::destroy(LockFreeMapKey<K, V>& value, LockFreeMapHazardRecord* record)
{
    // 键的最后一个拥有者已退出，当前值不再有新的条件计划取得引用
    LockFreeMapPoolSlot<V>* current = value.m_value.load(std::memory_order_relaxed);
    if (current != nullptr)
    {
        value.m_pool->releaseDetached(current, record);
    }
#ifdef __EMSCRIPTEN__
    // 此键已无其他拥有者和新引用入口，私有析构标记替代重复的原子清空与读取
    value.m_pool = nullptr;
#else
    value.m_value.store(nullptr, std::memory_order_relaxed);
#endif
    value.~LockFreeMapKey<K, V>();
}

template <typename K, typename V>
LockFreeMapIndexBuffer<K, V>::LockFreeMapIndexBuffer() :
m_references(0),
m_entries(nullptr),
m_capacity(0)
{

}

template <typename K, typename V>
LockFreeMapIndex<K, V>::LockFreeMapIndex() :
m_state(0),
m_entries(nullptr),
m_count(0),
m_reclaimed(0),
m_decision(0),
m_target(nullptr),
m_expected(nullptr),
m_omitted(0),
m_omittedReleased(false),
m_buffer(nullptr),
m_added(nullptr),
m_borrowedBuffer(false),
m_overridePosition(0),
m_merged(1)
{
    m_overrideEntry.m_key = nullptr;
}

template <typename K, typename V>
size_t LockFreeMapIndex<K, V>::count() const
{
    const size_t raw = m_count.load();
    return raw - (raw != 0 && m_decision.load() == 2 ? 1 : 0);
}

template <typename K, typename V>
const LockFreeMapIndexEntry<K, V>& LockFreeMapIndex<K, V>::entry(size_t position) const
{
    return rawEntry(position + (m_decision.load() == 2 && position >= m_omitted ? 1 : 0));
}

template <typename K, typename V>
const LockFreeMapIndexEntry<K, V>& LockFreeMapIndex<K, V>::rawEntry(size_t position) const
{
    return position == m_overridePosition ? m_overrideEntry : m_entries[position];
}

template <typename K, typename V, typename Less>
LockFreeMapIndexStore<K, V, Less>::LockFreeMapIndexStore(const LockFreeMapConfig& config) :
m_less(),
m_config(config),
m_domain(config.operationRecordLimit),
m_nodes(config.nodes, m_domain),
m_values(config.values, m_domain, true),
m_identity(new (std::nothrow) LockFreeMapIdentity),
m_root(nullptr),
m_version(0),
m_retries(0),
m_indexBytes(0),
m_indices(new (std::nothrow) LockFreeMapIndex<K, V>[config.operationRecordLimit * 4 + 4]),
m_buffers(new (std::nothrow) LockFreeMapIndexBuffer<K, V>[config.operationRecordLimit * 4 + 4]),
m_indexCount(m_indices == nullptr || m_buffers == nullptr ? 0 : config.operationRecordLimit * 4 + 4),
m_cursor(0),
m_span(0)
{

}

template <typename K, typename V, typename Less>
LockFreeMapIndexStore<K, V, Less>::~LockFreeMapIndexStore()
{
    freeMemory();
    delete[] m_indices;
    delete[] m_buffers;
}

template <typename K, typename V, typename Less>
LockFreeMapIndex<K, V>* LockFreeMapIndexStore<K, V, Less>::protect(LockFreeMapGuard& guard)
{
    for (;;)
    {
        LockFreeMapIndex<K, V>* root = m_root.load();
#ifdef __EMSCRIPTEN__
        // 领取时已发布同一源且登记完成，本次源读取就是发布后的验证
        // 提示不同则完整执行发布再验证，绝不解引用尚未验证的旧提示
        if (root != guard.m_record->m_pointer[0].load(std::memory_order_relaxed))
        {
            guard.m_record->m_pointer[0].store(root);
            if (root != m_root.load())
            {
                continue;
            }
        }
#else
        guard.m_record->m_pointer[0].store(root);
        if (root != m_root.load())
        {
            continue;
        }
#endif
        if (root != nullptr)
        {
            help(root);
        }
        return root;
    }
}

template <typename K, typename V, typename Less>
LockFreeMapPoolSlot<V>* LockFreeMapIndexStore<K, V, Less>::protectValue(
    LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* key, LockFreeMapGuard& guard)
{
    LockFreeMapPoolSlot<V>* value = nullptr;
    do
    {
        value = key->value().m_value.load();
        guard.m_record->m_pointer[1].store(value);
    }
    while (value != key->value().m_value.load());
    return value;
}

template <typename K, typename V, typename Less>
uint32_t LockFreeMapIndexStore<K, V, Less>::help(LockFreeMapIndex<K, V>* index)
{
    uint32_t decision = index->m_decision.load();
    if (decision == 1)
    {
        LockFreeMapPoolSlot<V>* expected = index->m_expected;
        const bool removed = index->m_target->mutableValue().m_value.compare_exchange_strong(expected, nullptr);
        const uint32_t result = removed || expected == nullptr ? 2 : 3;
        // 其他非空版本不可能重新成为 m_expected
        // 计划拥有该值槽，直到所有帮助者释放索引保护
        index->m_decision.compare_exchange_strong(decision, result);
        if (removed)
        {
            m_values.release(index->m_expected);
        }
        decision = index->m_decision.load();
    }
    if (decision == 2 && index->m_count.load() == 1)
    {
        LockFreeMapIndex<K, V>* current = index;
        m_root.compare_exchange_strong(current, nullptr);
    }
    return decision;
}

template <typename K, typename V, typename Less>
size_t LockFreeMapIndexStore<K, V, Less>::find(LockFreeMapIndex<K, V>* index, const K& key, bool& found, LockFreeMapHazardRecord* record) const
{
    if (index == nullptr)
    {
        found = false;
        return 0;
    }
    // protect 已完成描述符决策，持有根保护期间决策不会改变
    // 因此只需读取一次逻辑视图，再执行本次查找
    const bool removed = index->m_decision.load() == 2;
    const size_t count = index->m_count.load() - (removed ? 1 : 0);
    const size_t omitted = removed ? index->m_omitted : count;
    size_t begin = 0;
    size_t end = count;
    if (record != nullptr && record->m_searchLocality && record->m_searchHint < count)
    {
        const size_t hint = record->m_searchHint;
        const K& candidate = index->rawEntry(hint + (hint >= omitted ? 1 : 0)).m_key->value().m_key;
        if (m_less(key, candidate))
        {
            if (hint == 0)
            {
                found = false;
                return 0;
            }
            const size_t previous = hint - 1;
            const K& preceding = index->rawEntry(previous + (previous >= omitted ? 1 : 0)).m_key->value().m_key;
            // 删除后重插及反向相邻访问优先检查提示左邻，避免在已经夹住键的区间重新二分
            if (m_less(preceding, key))
            {
                found = false;
                return hint;
            }
            if (!m_less(key, preceding))
            {
                found = true;
                record->m_searchHint = previous;
                return previous;
            }
            end = previous;
        }
        else if (!m_less(candidate, key))
        {
            found = true;
            return hint;
        }
        else
        {
            begin = hint + 1;
            if (begin < count)
            {
                const K& next = index->rawEntry(begin + (begin >= omitted ? 1 : 0)).m_key->value().m_key;
                if (!m_less(next, key))
                {
                    found = !m_less(key, next);
                    record->m_searchHint = begin;
                    return begin;
                }
                ++begin;
            }
        }
    }
    while (begin < end)
    {
        const size_t middle = begin + (end - begin) / 2;
        const size_t raw = middle + (middle >= omitted ? 1 : 0);
        if (m_less(index->rawEntry(raw).m_key->value().m_key, key))
        {
            begin = middle + 1;
        }
        else
        {
            end = middle;
        }
    }
    const size_t raw = begin + (begin >= omitted ? 1 : 0);
    found = begin < count && !m_less(key, index->rawEntry(raw).m_key->value().m_key);
    if (record != nullptr)
    {
        record->m_searchLocality = begin == record->m_searchHint ||
            (begin != 0 && begin - 1 == record->m_searchHint);
        record->m_searchHint = begin;
    }
    return begin;
}

template <typename K, typename V, typename Less>
bool LockFreeMapIndexStore<K, V, Less>::nextVersion(uint64_t& version)
{
    uint64_t previous = m_version.load();
    while (previous != (std::numeric_limits<uint64_t>::max)())
    {
        if (m_version.compare_exchange_weak(previous, previous + 1))
        {
            version = previous + 1;
            return true;
        }
    }
    return false;
}

template <typename K, typename V, typename Less>
bool LockFreeMapIndexStore<K, V, Less>::nextVersion(uint64_t& version, LockFreeMapGuard& guard)
{
    LockFreeMapHazardRecord& record = *guard.m_record;
    if (record.m_versionNext == 0)
    {
        const uint64_t maximum = (std::numeric_limits<uint64_t>::max)();
        uint64_t previous = m_version.load();
        for (;;)
        {
            if (previous == maximum)
            {
                return false;
            }
            const uint64_t amount = maximum - previous < 64 ? maximum - previous : 64;
            if (m_version.compare_exchange_weak(previous, previous + amount))
            {
                record.m_versionNext = previous + 1;
                record.m_versionLast = previous + amount;
                break;
            }
        }
    }
    version = record.m_versionNext;
    record.m_versionNext = version == record.m_versionLast ? 0 : version + 1;
    return true;
}

template <typename K, typename V, typename Less>
LockFreeMapIndexBuffer<K, V>* LockFreeMapIndexStore<K, V, Less>::buffer(size_t count, LockFreeMapStatus& status)
{
    status = LOCK_FREE_MAP_OK;
    if (count == 0)
    {
        return nullptr;
    }
    // 扩容时新旧数组短暂共存，把该峰值也纳入字节溢出上限
    size_t maximum = (std::numeric_limits<size_t>::max)() / 2 / sizeof(LockFreeMapIndexEntry<K, V>) / m_indexCount;
    if (m_config.nodes.maximumCapacity >= 0 && static_cast<uint64_t>(m_config.nodes.maximumCapacity) < maximum)
    {
        maximum = static_cast<size_t>(m_config.nodes.maximumCapacity);
    }
    if (count > maximum)
    {
        status = LOCK_FREE_MAP_CAPACITY;
        return nullptr;
    }
    for (size_t slot = 0; slot < m_indexCount; ++slot)
    {
        LockFreeMapIndexBuffer<K, V>& record = m_buffers[slot];
        size_t expected = 0;
        if (record.m_references.load(std::memory_order_relaxed) != 0 ||
            !record.m_references.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            continue;
        }
        if (record.m_capacity < count)
        {
            size_t capacity = record.m_capacity == 0 ? 1 : record.m_capacity;
            while (capacity < count)
            {
                capacity = capacity > maximum / 2 ? maximum : capacity * 2;
            }
            LockFreeMapIndexEntry<K, V>* entries = nullptr;
            try
            {
                entries = new (std::nothrow) LockFreeMapIndexEntry<K, V>[capacity];
            }
            catch (...)
            {
                entries = nullptr;
            }
            if (entries == nullptr)
            {
                record.m_references.store(0, std::memory_order_release);
                status = LOCK_FREE_MAP_ALLOCATION_FAILURE;
                return nullptr;
            }
            m_indexBytes.fetch_add((capacity - record.m_capacity) * sizeof(LockFreeMapIndexEntry<K, V>));
            delete[] record.m_entries;
            record.m_entries = entries;
            record.m_capacity = capacity;
        }
        return &record;
    }
    status = LOCK_FREE_MAP_RECORD_LIMIT;
    return nullptr;
}

template <typename K, typename V, typename Less>
void LockFreeMapIndexStore<K, V, Less>::copyEntries(LockFreeMapIndexEntry<K, V>* destination,
    LockFreeMapIndex<K, V>* source, size_t begin, size_t count)
{
    if (count == 0)
    {
        return;
    }
    const bool omitted = source->m_decision.load() == 2;
    const size_t position = source->m_omitted;
    size_t raw = begin + (omitted && begin >= position ? 1 : 0);
    const size_t overridden = source->m_overridePosition;
    while (count != 0)
    {
        if (omitted && raw == position)
        {
            ++raw;
            continue;
        }
        if (raw == overridden)
        {
            *destination++ = source->m_overrideEntry;
            ++raw;
            --count;
            continue;
        }
        size_t amount = count;
        if (omitted && position > raw && position - raw < amount)
        {
            amount = position - raw;
        }
        if (overridden > raw && overridden - raw < amount)
        {
            amount = overridden - raw;
        }
        std::memcpy(destination, source->m_entries + raw, amount * sizeof(*destination));
        destination += amount;
        raw += amount;
        count -= amount;
    }
}

template <typename K, typename V, typename Less>
#if defined(__EMSCRIPTEN__) && defined(__clang__) && defined(__OPTIMIZE__)
// 仅合并 Wasm 热点调用边界，保留异常捕获和资源归还路径
__attribute__((always_inline)) inline
#endif
LockFreeMapIndex<K, V>* LockFreeMapIndexStore<K, V, Less>::build(LockFreeMapIndex<K, V>* source,
    size_t position, bool found, LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* key, LockFreeMapPoolSlot<V>* value,
    LockFreeMapGuard& guard, LockFreeMapStatus& status, bool logicalOnly)
{
    const size_t oldCount = source == nullptr ? 0 : source->count();
    const size_t count = oldCount + (key != nullptr && !found ? 1 : 0);
    status = LOCK_FREE_MAP_OK;
    LockFreeMapIndex<K, V>* candidate = nullptr;
    if (guard.m_record->m_indexCache != nullptr)
    {
        candidate = static_cast<LockFreeMapIndex<K, V>*>(guard.m_record->m_indexCache);
        guard.m_record->m_indexCache = nullptr;
        // 缓存记录始终保持私有且不拥有键或数组；清空内存同步清空全部缓存提示
    }
    for (size_t slot = 0; candidate == nullptr && slot < m_indexCount; ++slot)
    {
        uint32_t expected = 0;
        if (m_indices[slot].m_state.load(std::memory_order_relaxed) == 0 &&
            m_indices[slot].m_state.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            size_t span = m_span.load();
            while (span <= slot && !m_span.compare_exchange_weak(span, slot + 1))
            {

            }
            candidate = &m_indices[slot];
            break;
        }
    }
    if (candidate == nullptr)
    {
        status = LOCK_FREE_MAP_RECORD_LIMIT;
        return nullptr;
    }
#ifndef __EMSCRIPTEN__
    candidate->m_added = nullptr;
    candidate->m_target = nullptr;
    candidate->m_expected = nullptr;
#endif
    // 初始记录、完成回收的记录和空缓存均不持有上述引用；Wasm 复用时保留这些空字段
    const bool sourceOmitted = source != nullptr && source->m_decision.load() == 2;
    const bool merged = source == nullptr || source->m_merged.load(std::memory_order_acquire) != 0;
    const bool fillsHole = key != nullptr && sourceOmitted && position == source->m_omitted &&
        (merged || source->m_overridePosition == position || source->m_overridePosition == source->m_count.load());
    candidate->m_borrowedBuffer = (key == nullptr && source != nullptr && !sourceOmitted) || fillsHole;
    candidate->m_buffer = candidate->m_borrowedBuffer ? source->m_buffer : buffer(count, status);
    if (status != LOCK_FREE_MAP_OK)
    {
#ifdef __EMSCRIPTEN__
        // 缓存可能保留旧计数；失败转为空闲前必须清零，静止期释放才不会再次释放旧条目
        candidate->m_count.store(0, std::memory_order_relaxed);
#endif
        candidate->m_state.store(0, std::memory_order_release);
        return nullptr;
    }
    candidate->m_entries = candidate->m_buffer == nullptr ? nullptr : candidate->m_buffer->m_entries;
#ifdef __EMSCRIPTEN__
    // 只在私有构造阶段省略同值原子写；弱一致统计仍使用原子字段
    if (candidate->m_count.load(std::memory_order_relaxed) != count)
    {
        candidate->m_count.store(count, std::memory_order_relaxed);
    }
#else
    candidate->m_count.store(count, std::memory_order_relaxed);
#endif
    candidate->m_reclaimed = 0;
    candidate->m_omitted = position;
    candidate->m_omittedReleased = false;
    candidate->m_overridePosition = candidate->m_borrowedBuffer && !merged ? source->m_overridePosition : count;
    if (candidate->m_overridePosition < count)
    {
        candidate->m_overrideEntry = source->m_overrideEntry;
    }
    if (fillsHole)
    {
        candidate->m_overridePosition = position;
        candidate->m_overrideEntry.m_key = key;
    }
#ifdef __EMSCRIPTEN__
    const uint32_t mergeState = candidate->m_overridePosition == count ? 1 : 0;
    if (candidate->m_merged.load(std::memory_order_relaxed) != mergeState)
    {
        candidate->m_merged.store(mergeState, std::memory_order_relaxed);
    }
    const uint32_t decision = key == nullptr ? logicalOnly ? 2 : 1 : 0;
    if (candidate->m_decision.load(std::memory_order_relaxed) != decision)
    {
        candidate->m_decision.store(decision, std::memory_order_relaxed);
    }
#else
    candidate->m_merged.store(candidate->m_overridePosition == count ? 1 : 0, std::memory_order_relaxed);
    candidate->m_decision.store(key == nullptr ? logicalOnly ? 2 : 1 : 0, std::memory_order_relaxed);
#endif
    if (key == nullptr)
    {
        if (!logicalOnly)
        {
            if (!m_values.tryRetain(value))
            {
                discard(candidate);
                status = LOCK_FREE_MAP_CONFLICT;
                return nullptr;
            }
            candidate->m_expected = value;
            candidate->m_target = source->entry(position).m_key;
            m_nodes.retain(candidate->m_target);
        }
        if (!candidate->m_borrowedBuffer)
        {
            copyEntries(candidate->m_entries, source, 0, oldCount);
        }
    }
    else
    {
        if (!fillsHole)
        {
            copyEntries(candidate->m_entries, source, 0, position);
            candidate->m_entries[position].m_key = key;
            copyEntries(candidate->m_entries + position + 1, source, position, oldCount - position);
        }
        // 只有完整构造成功才接管调用者的新键引用
        candidate->m_added = key;
    }
    // 私有状态和构造者的源保护共同覆盖借用，完成转换前不能开放回收
    (void)guard;
    return candidate;
}

template <typename K, typename V, typename Less>
size_t LockFreeMapIndexStore<K, V, Less>::clean(LockFreeMapIndex<K, V>* index, LockFreeMapHazardRecord* completed)
{
    size_t released = 0;
    const uint32_t decision = index->m_decision.load();
    if (decision == 2 && !index->m_omittedReleased && index->m_count.load() != 0)
    {
        // 仅归还本索引拥有的引用；共享数组内容不变，旧视图仍有自己的拥有引用
        // 本视图已省略该键；其他旧视图若仍可复制它，必然还拥有各自的键引用
        m_nodes.releaseDetached(index->rawEntry(index->m_omitted).m_key, completed);
        index->m_omittedReleased = true;
        released = 1;
    }
    if (index->m_target != nullptr && decision >= 2 && !m_domain.protects(index, completed))
    {
        m_nodes.release(index->m_target, completed);
        m_values.release(index->m_expected, completed);
        index->m_target = nullptr;
        index->m_expected = nullptr;
    }
    return released;
}

template <typename K, typename V, typename Less>
void LockFreeMapIndexStore<K, V, Less>::complete(LockFreeMapIndex<K, V>* candidate,
    LockFreeMapIndex<K, V>* source, LockFreeMapGuard& guard)
{
    bool transferred = source == nullptr;
    if (source != nullptr)
    {
        uint32_t expected = 2;
        if (source->m_state.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            // 成功的根 CAS 已摘除 source，当前源保护使其不可能回收并再次发布
            // 源的其他私有构造者也保持源 hazard；先转为私有，避免与扫描器同时维护
            if (!m_domain.protects(source, guard.m_record))
            {
                clean(source, guard.m_record);
                if (!candidate->m_borrowedBuffer && source->m_buffer != nullptr)
                {
                    source->m_buffer->m_references.fetch_sub(1, std::memory_order_release);
                }
                source->m_buffer = nullptr;
#ifndef __EMSCRIPTEN__
                source->m_count.store(0, std::memory_order_relaxed);
#endif
                // Wasm 空缓存保留计数以减少下次同规模发布的原子写；私有状态不拥有条目引用
                // 只缓存无资源的固定描述符；最多每操作记录一个，不缓存旧键或值
                guard.m_record->m_indexCache = source;
                transferred = true;
            }
            else
            {
                source->m_state.store(2, std::memory_order_release);
            }
        }
    }
    if (!transferred)
    {
        // 暂停或并发读者存在时独立取得引用，不能等待源记录的持有者
        if (candidate->m_borrowedBuffer)
        {
            candidate->m_buffer->m_references.fetch_add(1);
        }
        for (size_t position = 0; position < candidate->m_count.load(); ++position)
        {
            LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* key = candidate->rawEntry(position).m_key;
            if (key != candidate->m_added)
            {
                m_nodes.retain(key);
            }
        }
    }
    candidate->m_added = nullptr;
    candidate->m_borrowedBuffer = false;
    if (candidate->m_overridePosition < candidate->m_count.load() && candidate->m_buffer->m_references.load() == 1)
    {
        // 旧的独立数组拥有者已全部退出，现有借用者只能通过覆盖描述访问该位置
        // 补写后以 release 开放后继直接读取；已有视图仍使用自己的不可变覆盖条目
        candidate->m_entries[candidate->m_overridePosition] = candidate->m_overrideEntry;
        candidate->m_merged.store(1, std::memory_order_release);
    }
    if (guard.m_record->m_pointer[1].load(std::memory_order_relaxed) != nullptr)
    {
        guard.m_record->m_pointer[1].store(nullptr, std::memory_order_release);
    }
    clean(candidate, guard.m_record);
    candidate->m_state.store(2, std::memory_order_release);
}

template <typename K, typename V, typename Less>
void LockFreeMapIndexStore<K, V, Less>::discard(LockFreeMapIndex<K, V>* candidate)
{
    m_nodes.release(candidate->m_added);
    m_nodes.release(candidate->m_target);
    m_values.release(candidate->m_expected);
    if (!candidate->m_borrowedBuffer && candidate->m_buffer != nullptr)
    {
        candidate->m_buffer->m_references.fetch_sub(1, std::memory_order_release);
    }
    candidate->m_added = nullptr;
    candidate->m_target = nullptr;
    candidate->m_expected = nullptr;
    candidate->m_buffer = nullptr;
    candidate->m_entries = nullptr;
    candidate->m_count.store(0, std::memory_order_relaxed);
    candidate->m_state.store(0, std::memory_order_release);
}

template <typename K, typename V, typename Less>
size_t LockFreeMapIndexStore<K, V, Less>::collect(size_t budget, LockFreeMapHazardRecord* completed, bool releaseBuffers)
{
    size_t reclaimed = 0;
    const size_t span = m_span.load();
    size_t position = span == 0 ? 0 : m_cursor.fetch_add(1, std::memory_order_relaxed) % span;
    for (size_t visit = 0; visit < span && reclaimed < budget; ++visit)
    {
        LockFreeMapIndex<K, V>& index = m_indices[position];
        if (++position == span)
        {
            position = 0;
        }
        const uint32_t state = index.m_state.load(std::memory_order_relaxed);
        if ((state != 2 && state != 3) || (state == 2 && index.m_decision.load() == 0 && m_root.load() == &index))
        {
            continue;
        }
        uint32_t expected = 2;
        if (index.m_state.compare_exchange_strong(expected, 4, std::memory_order_acquire, std::memory_order_relaxed))
        {
            reclaimed += clean(&index, completed);
            if (m_root.load() == &index || m_domain.protects(&index))
            {
                index.m_state.store(2, std::memory_order_release);
                continue;
            }
        }
        else
        {
            expected = 3;
            if (!index.m_state.compare_exchange_strong(expected, 4, std::memory_order_acquire, std::memory_order_relaxed))
            {
                continue;
            }
        }
        while (index.m_reclaimed < index.m_count.load() && reclaimed < budget)
        {
            if (!index.m_omittedReleased || index.m_reclaimed != index.m_omitted)
            {
                m_nodes.release(index.rawEntry(index.m_reclaimed).m_key, completed);
                ++reclaimed;
            }
            ++index.m_reclaimed;
        }
        if (index.m_reclaimed == index.m_count.load())
        {
            m_nodes.release(index.m_target, completed);
            m_values.release(index.m_expected, completed);
            index.m_target = nullptr;
            index.m_expected = nullptr;
            if (index.m_buffer != nullptr)
            {
                index.m_buffer->m_references.fetch_sub(1, std::memory_order_release);
            }
            index.m_buffer = nullptr;
            index.m_entries = nullptr;
            index.m_count.store(0, std::memory_order_relaxed);
            index.m_state.store(0, std::memory_order_release);
        }
        else
        {
            index.m_state.store(3, std::memory_order_release);
        }
    }
    if (releaseBuffers && budget != 0)
    {
        for (size_t slot = 0; slot < m_indexCount; ++slot)
        {
            LockFreeMapIndexBuffer<K, V>& record = m_buffers[slot];
            size_t expected = 0;
            if (record.m_references.load(std::memory_order_relaxed) == 0 &&
                record.m_references.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
            {
                m_indexBytes.fetch_sub(record.m_capacity * sizeof(LockFreeMapIndexEntry<K, V>));
                delete[] record.m_entries;
                record.m_entries = nullptr;
                record.m_capacity = 0;
                record.m_references.store(0, std::memory_order_release);
            }
        }
    }
    return reclaimed;
}

template <typename K, typename V, typename Less>
void LockFreeMapIndexStore<K, V, Less>::freeMemory()
{
    m_root.store(nullptr);
    m_domain.clearIndexCaches();
    for (size_t slot = 0; slot < m_indexCount; ++slot)
    {
        LockFreeMapIndex<K, V>& index = m_indices[slot];
        if (index.m_state.load() == 1)
        {
            m_nodes.release(index.m_added);
        }
        else
        {
            while (index.m_reclaimed < index.m_count.load())
            {
                if (!index.m_omittedReleased || index.m_reclaimed != index.m_omitted)
                {
                    m_nodes.release(index.rawEntry(index.m_reclaimed).m_key);
                }
                ++index.m_reclaimed;
            }
        }
        m_nodes.release(index.m_target);
        m_values.release(index.m_expected);
        index.m_target = nullptr;
        index.m_expected = nullptr;
        index.m_added = nullptr;
        index.m_buffer = nullptr;
        index.m_entries = nullptr;
        index.m_count.store(0, std::memory_order_relaxed);
        index.m_reclaimed = 0;
        index.m_state.store(0, std::memory_order_release);
    }
    for (size_t slot = 0; slot < m_indexCount; ++slot)
    {
        delete[] m_buffers[slot].m_entries;
        m_buffers[slot].m_entries = nullptr;
        m_buffers[slot].m_capacity = 0;
        m_buffers[slot].m_references.store(0, std::memory_order_release);
    }
    m_indexBytes.store(0);
    m_span.store(0);
    m_nodes.freeMemory();
    m_values.freeMemory();
}

#endif