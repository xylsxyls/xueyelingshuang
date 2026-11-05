#ifndef LOCK_FREE_MAP_IMPLEMENTATION_INL
#define LOCK_FREE_MAP_IMPLEMENTATION_INL

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMap<K, V, Less, Levels>::LockFreeMap() :
m_store(nullptr),
m_initialStatus(LOCK_FREE_MAP_INVALID)
{
    static_assert(Levels > 0, "LockFreeMap MaxLevel must be positive");
    try
    {
        m_initialStatus = lockFreeMapConfigure(LockFreeMapConfig());
    }
    catch (const std::bad_alloc&)
    {
        m_initialStatus = LOCK_FREE_MAP_ALLOCATION_FAILURE;
    }
    catch (...)
    {
        m_initialStatus = LOCK_FREE_MAP_USER_EXCEPTION;
    }
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMap<K, V, Less, Levels>::~LockFreeMap()
{
    delete m_store;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::lockFreeMapAtomicsSupported() const
{
    std::atomic<void*> pointer;
    std::atomic<size_t> size;
    std::atomic<uint64_t> wide;
    std::atomic<uint32_t> word;
    return pointer.is_lock_free() && size.is_lock_free() && wide.is_lock_free() && word.is_lock_free();
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapConfigure(const LockFreeMapConfig& config)
{
    if (!lockFreeMapAtomicsSupported())
    {
        return LOCK_FREE_MAP_UNSUPPORTED;
    }
    const size_t limit = (std::numeric_limits<size_t>::max)();
    if (!LockFreeMapPool<LockFreeMapKey<K, V>>::valid(config.nodes) || !LockFreeMapPool<V>::valid(config.values) ||
        config.operationRecordLimit == 0 || config.maintenanceBudget == 0 ||
        config.operationRecordLimit > (limit / sizeof(LockFreeMapIndex<K, V>) - 4) / 4 ||
        config.operationRecordLimit > limit / sizeof(LockFreeMapHazardRecord) ||
        config.operationRecordLimit > limit / 16)
    {
        return LOCK_FREE_MAP_INVALID;
    }
    if (m_store != nullptr && (count() != 0 || m_store->m_domain.active() != 0))
    {
        return LOCK_FREE_MAP_INVALID;
    }
    LockFreeMapIndexStore<K, V, Less>* replacement = nullptr;
    LockFreeMapStatus status = LOCK_FREE_MAP_OK;
    try
    {
        replacement = new (std::nothrow) LockFreeMapIndexStore<K, V, Less>(config);
        status = replacement == nullptr || !replacement->m_identity || replacement->m_indexCount == 0 ||
            replacement->m_domain.bytes() == 0 ? LOCK_FREE_MAP_ALLOCATION_FAILURE : replacement->m_nodes.initialize();
        if (status == LOCK_FREE_MAP_OK)
        {
            status = replacement->m_values.initialize();
        }
    }
    catch (const std::bad_alloc&)
    {
        status = LOCK_FREE_MAP_ALLOCATION_FAILURE;
    }
    catch (...)
    {
        status = LOCK_FREE_MAP_USER_EXCEPTION;
    }
    if (status != LOCK_FREE_MAP_OK)
    {
        delete replacement;
        return status;
    }
    delete m_store;
    m_store = replacement;
    m_initialStatus = LOCK_FREE_MAP_OK;
    return LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::read(const K* key, K* outputKey, V* outputValue,
    LockFreeMapToken* outputToken, uint32_t mode) const
#ifdef __EMSCRIPTEN__
noexcept try
#endif
{
    if (m_store == nullptr)
    {
        return m_initialStatus;
    }
#ifdef __EMSCRIPTEN__
    // 领取与首次源发布合并，protect 仍完成登记后的根验证
    LockFreeMapGuard guard(m_store->m_domain, m_store->m_root.load());
#else
    LockFreeMapGuard guard(m_store->m_domain);
#endif
    if (guard.m_record == nullptr)
    {
        return LOCK_FREE_MAP_RECORD_LIMIT;
    }
    try
    {
        for (;;)
        {
            LockFreeMapIndex<K, V>* root = m_store->protect(guard);
#ifdef __EMSCRIPTEN__
            // 根保护及已完成的删除决策使本次视图计数不变，只读取一次原子元数据
            const size_t visibleCount = root == nullptr ? 0 : root->count();
            if (visibleCount == 0)
#else
            if (root == nullptr || root->count() == 0)
#endif
            {
                return LOCK_FREE_MAP_NOT_FOUND;
            }
            size_t position = 0;
            bool found = false;
            if (mode == 2)
            {
#ifdef __EMSCRIPTEN__
                position = visibleCount - 1;
#else
                position = root->count() - 1;
#endif
            }
            else if (mode != 1)
            {
                position = m_store->find(root, *key, found, guard.m_record);
                if (mode == 0 && !found)
                {
                    return LOCK_FREE_MAP_NOT_FOUND;
                }
                if (mode == 4 && found)
                {
                    ++position;
                }
            }
#ifdef __EMSCRIPTEN__
            if (position >= visibleCount)
#else
            if (position >= root->count())
#endif
            {
                return LOCK_FREE_MAP_NOT_FOUND;
            }
            LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* node = root->entry(position).m_key;
            LockFreeMapPoolSlot<V>* current = outputValue != nullptr || outputToken != nullptr ?
                m_store->protectValue(node, guard) : node->value().m_value.load();
            // 取得值快照后重新验证键集合，保证边界查询
            // 返回的版本曾存在于该次验证的有序键集合中
            if (current == nullptr || root != m_store->m_root.load())
            {
                continue;
            }
            if (outputKey != nullptr)
            {
                *outputKey = node->value().m_key;
            }
            if (outputValue != nullptr)
            {
                *outputValue = current->value();
            }
            if (outputToken != nullptr)
            {
                outputToken->m_identity = m_store->m_identity;
                outputToken->m_version = current->m_version;
            }
            return LOCK_FREE_MAP_OK;
        }
    }
    catch (...)
    {
        return LOCK_FREE_MAP_USER_EXCEPTION;
    }
}
#ifdef __EMSCRIPTEN__
catch (...)
{
    return LOCK_FREE_MAP_USER_EXCEPTION;
}
#endif

template <typename K, typename V, typename Less, size_t Levels>
template <uint32_t Mode>
#if defined(__EMSCRIPTEN__) && defined(__clang__) && defined(__OPTIMIZE__)
// 仅合并 Wasm 热点调用边界，保留异常捕获和资源归还路径
__attribute__((always_inline)) inline
#endif
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::write(const K* key, const V* value,
    const LockFreeMapToken* token, K* outputKey, V* outputValue, LockFreeMapToken* outputToken)
#ifdef __EMSCRIPTEN__
noexcept try
#endif
{
    if (m_store == nullptr)
    {
        return m_initialStatus;
    }
    if (Mode >= 3 && token != nullptr && (token->m_identity != m_store->m_identity || token->m_version == 0))
    {
        return LOCK_FREE_MAP_CONFLICT;
    }
    LockFreeMapIndexStore<K, V, Less>* const store = m_store;
#ifdef __EMSCRIPTEN__
    // 领取与首次源发布合并，protect 仍完成登记后的根验证
    LockFreeMapGuard guard(store->m_domain, store->m_root.load());
#else
    LockFreeMapGuard guard(store->m_domain);
#endif
    if (guard.m_record == nullptr)
    {
        return LOCK_FREE_MAP_RECORD_LIMIT;
    }
    LockFreeMapIndex<K, V>* root = store->protect(guard);
    const size_t count = root == nullptr ? 0 : root->count();
    guard.m_record->m_maintenanceTick = (guard.m_record->m_maintenanceTick + 1) & 63;
    LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* ownedKey = nullptr;
    LockFreeMapPoolSlot<V>* ownedValue = nullptr;
    LockFreeMapStatus status = LOCK_FREE_MAP_OK;
    uint64_t version = 0;
    try
    {
        bool found = false;
        size_t position = 0;
        if (Mode == 5)
        {
            found = count != 0;
            position = found ? count - 1 : 0;
        }
        else
        {
            position = store->find(root, *key, found, guard.m_record);
        }
        const bool deleting = Mode == 2 || Mode == 4 || Mode == 5;
        const bool logicalOnly = Mode == 2 && outputValue == nullptr && store->m_config.values.maximumCapacity < 0;
        // 无输出的逻辑删除只提交键集合；根 CAS 会验证存在性，不需要读取目标值缓存行
        LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* node = found && !logicalOnly ? root->entry(position).m_key : nullptr;
        LockFreeMapPoolSlot<V>* previous = nullptr;
        if (found && !logicalOnly)
        {
            // 无条件覆盖不解引用旧版本，只以地址进行 CAS
            // 条件操作及输出复制必须保护旧版本，防止并发回收
            previous = Mode >= 3 || (deleting && !logicalOnly) || Mode == 1 ?
                store->protectValue(node, guard) : node->value().m_value.load();
            if (previous == nullptr)
            {
                return LOCK_FREE_MAP_CONFLICT;
            }
        }
        if (Mode >= 3 && token != nullptr && (!found || previous->m_version != token->m_version))
        {
            return LOCK_FREE_MAP_CONFLICT;
        }
        if (Mode >= 2 && !found)
        {
            return LOCK_FREE_MAP_NOT_FOUND;
        }
        if (Mode == 1 && found)
        {
            if (root != store->m_root.load())
            {
                return LOCK_FREE_MAP_CONFLICT;
            }
            if ((Mode == 1 || Mode == 2 || Mode == 5) && outputValue != nullptr)
            {
                *outputValue = previous->value();
            }
            if (Mode == 1 && outputToken != nullptr)
            {
                outputToken->m_identity = store->m_identity;
                outputToken->m_version = previous->m_version;
            }
            return LOCK_FREE_MAP_EXISTS;
        }
        // 结构写入先回收旧索引，再判断满池是否需要扩容
        // 已有键覆盖只按固定操作次数分摊策略维护
        if (guard.m_record->m_maintenanceTick == 0 || ((!found || deleting) && store->m_domain.multipleRecords()) ||
            (!found && (root == nullptr || store->m_config.nodes.maximumCapacity >= 0 || store->m_config.values.maximumCapacity >= 0)))
        {
            const size_t extra = store->m_config.maintenanceBudget;
            const size_t maximum = (std::numeric_limits<size_t>::max)();
            store->collect(count > maximum - extra ? maximum : count + extra, guard.m_record);
            size_t released = 0;
            store->m_nodes.maintain(extra, released);
            store->m_values.maintain(extra, released);
        }
        if (Mode == 5 && outputKey != nullptr)
        {
            *outputKey = node->value().m_key;
        }
        if ((Mode == 1 || Mode == 2 || Mode == 5) && outputValue != nullptr)
        {
            *outputValue = deleting ? previous->value() : *value;
        }
        if (!deleting)
        {
            if (!store->nextVersion(version, guard))
            {
                return LOCK_FREE_MAP_VERSION_EXHAUSTED;
            }
            ownedValue = store->m_values.create(*value, version, guard, status);
            if (ownedValue == nullptr)
            {
                return status;
            }
            if (!found)
            {
#ifdef __EMSCRIPTEN__
                // 候选仍是私有对象，直接构造最终指针，根 CAS 才负责发布
                ownedKey = store->m_nodes.emplace(0, guard, status, *key, store->m_values, ownedValue);
#else
                ownedKey = store->m_nodes.emplace(0, guard, status, *key, store->m_values);
#endif
                if (ownedKey == nullptr)
                {
                    store->m_values.release(ownedValue);
                    return status;
                }
                // 新键尚未发布，根 CAS 负责向读者发布完整键和值
#ifndef __EMSCRIPTEN__
                ownedKey->mutableValue().m_value.store(ownedValue, std::memory_order_relaxed);
#endif
                ownedValue = nullptr;
            }
        }
        if (found && !deleting)
        {
            if (Mode == 0)
            {
                previous = node->value().m_value.load();
            }
            bool committed = false;
            // 无条件覆盖复用完整候选，仅在值竞争时重试；根变化或键关闭立即返回冲突
            while (previous != nullptr && root == store->m_root.load())
            {
                if (node->mutableValue().m_value.compare_exchange_strong(previous, ownedValue))
                {
                    committed = true;
                    break;
                }
                if (Mode != 0)
                {
                    break;
                }
                const uint64_t retries = guard.m_record->m_valueRetries.load(std::memory_order_relaxed);
                if (retries != (std::numeric_limits<uint64_t>::max)())
                {
                    guard.m_record->m_valueRetries.store(retries + 1, std::memory_order_relaxed);
                }
            }
            if (committed)
            {
                ownedValue = nullptr;
                if (Mode >= 3 && token != nullptr)
                {
                    guard.m_record->m_pointer[1].store(nullptr);
                }
                store->m_values.release(previous, guard.m_record);
                status = LOCK_FREE_MAP_COMMITTED;
            }
            else
            {
                status = LOCK_FREE_MAP_CONFLICT;
            }
        }
        else
        {
            LockFreeMapIndex<K, V>* candidate = store->build(root, position, found,
                ownedKey, deleting ? previous : nullptr, guard, status, logicalOnly);
            if (status == LOCK_FREE_MAP_RECORD_LIMIT)
            {
                const size_t extra = store->m_config.maintenanceBudget;
                const size_t maximum = (std::numeric_limits<size_t>::max)();
                store->collect(count > maximum - extra ? maximum : count + extra, guard.m_record);
                candidate = store->build(root, position, found, ownedKey, deleting ? previous : nullptr, guard, status, logicalOnly);
            }
            if (status == LOCK_FREE_MAP_OK)
            {
                // build 成功后新键属于候选，即使发布失败也由 discard 归还
                ownedKey = nullptr;
                LockFreeMapIndex<K, V>* source = root;
                if (store->m_root.compare_exchange_strong(root, candidate))
                {
                    status = !deleting || store->help(candidate) == 2 ? LOCK_FREE_MAP_COMMITTED : LOCK_FREE_MAP_CONFLICT;
                    store->complete(candidate, source, guard);
                }
                else
                {
                    status = LOCK_FREE_MAP_CONFLICT;
                    store->discard(candidate);
                }
            }
        }
        if (Mode == 1 && status == LOCK_FREE_MAP_COMMITTED && outputToken != nullptr)
        {
            outputToken->m_identity = store->m_identity;
            outputToken->m_version = version;
        }
        if (status == LOCK_FREE_MAP_CONFLICT)
        {
            LockFreeMapPool<V>::increment(store->m_retries);
        }
    }
    catch (const std::bad_alloc&)
    {
        status = LOCK_FREE_MAP_ALLOCATION_FAILURE;
    }
    catch (...)
    {
        status = LOCK_FREE_MAP_USER_EXCEPTION;
    }
    if (ownedKey != nullptr)
    {
        store->m_nodes.release(ownedKey);
    }
    if (ownedValue != nullptr)
    {
        store->m_values.release(ownedValue);
    }
    return status;
}
#ifdef __EMSCRIPTEN__
// 完整包住保护记录取得、提交及清理，私有入口才可声明不向调用者抛出
catch (const std::bad_alloc&)
{
    return LOCK_FREE_MAP_ALLOCATION_FAILURE;
}
catch (...)
{
    return LOCK_FREE_MAP_USER_EXCEPTION;
}
#endif

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapGet(const K& key, V& value, LockFreeMapToken& token) const
{
    return read(&key, nullptr, &value, &token, 0);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapGetOrInsert(const K& key, const V& candidate, V& value, LockFreeMapToken& token)
{
    return write<1>(&key, &candidate, nullptr, nullptr, &value, &token);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapUpdate(const K& key, const LockFreeMapToken& token, const V& value)
{
    return write<3>(&key, &value, &token, nullptr, nullptr, nullptr);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapErase(const K& key, const LockFreeMapToken& token)
{
    return write<4>(&key, nullptr, &token, nullptr, nullptr, nullptr);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapExtract(const K& key, V& value)
{
    return write<2>(&key, nullptr, nullptr, nullptr, &value, nullptr);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapLast(K& key, V& value, LockFreeMapToken& token) const
{
    return read(nullptr, &key, &value, &token, 2);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapExtractLast(const LockFreeMapToken& token, K& key, V& value)
{
    return write<5>(nullptr, nullptr, &token, &key, &value, nullptr);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapSet(const K& key, const V& value)
{
    return write<0>(&key, &value, nullptr, nullptr, nullptr, nullptr);
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::set(const K& key, const V& value)
{
    LockFreeMapStatus status;
    do
    {
        status = lockFreeMapSet(key, value);
    }
    while (status == LOCK_FREE_MAP_CONFLICT);
    return status == LOCK_FREE_MAP_COMMITTED;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::insert(const K& key, const V& value)
{
    LockFreeMapStatus status;
    do
    {
        status = write<1>(&key, &value, nullptr, nullptr, nullptr, nullptr);
    }
    while (status == LOCK_FREE_MAP_CONFLICT);
    return status == LOCK_FREE_MAP_COMMITTED;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::erase(const K& key)
{
    LockFreeMapStatus status;
    do
    {
        status = write<2>(&key, nullptr, nullptr, nullptr, nullptr, nullptr);
    }
    while (status == LOCK_FREE_MAP_CONFLICT);
    return status == LOCK_FREE_MAP_COMMITTED;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::remove(const K& key)
{
    return erase(key);
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::get(const K& key, V& value) const
{
    return read(&key, nullptr, &value, nullptr, 0) == LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
V LockFreeMap<K, V, Less, Levels>::value(const K& key, const V& defaultValue) const
{
    V result = defaultValue;
    get(key, result);
    return result;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::contains(const K& key) const
{
    return read(&key, nullptr, nullptr, nullptr, 0) == LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
size_t LockFreeMap<K, V, Less, Levels>::count(const K& key) const
{
    return contains(key) ? 1 : 0;
}

template <typename K, typename V, typename Less, size_t Levels>
size_t LockFreeMap<K, V, Less, Levels>::count() const
{
    if (m_store == nullptr)
    {
        return 0;
    }
    // 索引记录存活至静止期析构，此处只读取原子计数
    // 旧统计接口为弱一致观察，无需取得保护槽
    LockFreeMapIndex<K, V>* root = m_store->m_root.load();
    return root == nullptr ? 0 : root->count();
}

template <typename K, typename V, typename Less, size_t Levels>
size_t LockFreeMap<K, V, Less, Levels>::size() const
{
    return count();
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::empty() const
{
    return count() == 0;
}

template <typename K, typename V, typename Less, size_t Levels>
void LockFreeMap<K, V, Less, Levels>::clear()
{
    if (m_store != nullptr)
    {
        m_store->m_root.exchange(nullptr);
    }
}

template <typename K, typename V, typename Less, size_t Levels>
void LockFreeMap<K, V, Less, Levels>::clearAndFreeMemory()
{
    if (m_store != nullptr)
    {
        m_store->freeMemory();
    }
}

template <typename K, typename V, typename Less, size_t Levels>
template <typename Function>
size_t LockFreeMap<K, V, Less, Levels>::forEach(Function function) const
{
    if (m_store == nullptr)
    {
        return 0;
    }
    LockFreeMapGuard guard(m_store->m_domain);
    if (guard.m_record == nullptr)
    {
        return 0;
    }
    LockFreeMapIndex<K, V>* root = m_store->protect(guard);
    size_t completed = 0;
    try
    {
        if (root != nullptr)
        {
            for (size_t position = 0; position < root->count(); ++position)
            {
                LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* node = root->entry(position).m_key;
                LockFreeMapPoolSlot<V>* value = m_store->protectValue(node, guard);
                if (value != nullptr)
                {
                    function(node->value().m_key, value->value());
                    ++completed;
                }
            }
        }
    }
    catch (...)
    {
        return completed;
    }
    return completed;
}

template <typename K, typename V, typename Less, size_t Levels>
std::vector<K> LockFreeMap<K, V, Less, Levels>::keys() const
{
    std::vector<K> result;
    bool failed = false;
    // 将回调内借用的键复制到调用方容器；复制失败标记整次输出无效
    forEach([&result, &failed](const K& key, const V&) -> void
    {
        try
        {
            result.push_back(key);
        }
        catch (...)
        {
            failed = true;
        }
    });
    if (failed)
    {
        result.clear();
    }
    return result;
}

template <typename K, typename V, typename Less, size_t Levels>
std::vector<V> LockFreeMap<K, V, Less, Levels>::values() const
{
    std::vector<V> result;
    bool failed = false;
    // 将回调内借用的值复制到调用方容器；复制失败标记整次输出无效
    forEach([&result, &failed](const K&, const V& value) -> void
    {
        try
        {
            result.push_back(value);
        }
        catch (...)
        {
            failed = true;
        }
    });
    if (failed)
    {
        result.clear();
    }
    return result;
}

template <typename K, typename V, typename Less, size_t Levels>
std::vector<std::pair<K, V>> LockFreeMap<K, V, Less, Levels>::toVector() const
{
    std::vector<std::pair<K, V>> result;
    bool failed = false;
    // 复制回调借用的键值；任何一次复制异常均使整个结果无效
    forEach([&result, &failed](const K& key, const V& value) -> void
    {
        try
        {
            result.emplace_back(key, value);
        }
        catch (...)
        {
            failed = true;
        }
    });
    if (failed)
    {
        result.clear();
    }
    return result;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::first(K& key, V& value) const
{
    return read(nullptr, &key, &value, nullptr, 1) == LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::last(K& key, V& value) const
{
    return read(nullptr, &key, &value, nullptr, 2) == LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::lowerBound(const K& key, K& foundKey, V& value) const
{
    return read(&key, &foundKey, &value, nullptr, 3) == LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::upperBound(const K& key, K& foundKey, V& value) const
{
    return read(&key, &foundKey, &value, nullptr, 4) == LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
size_t LockFreeMap<K, V, Less, Levels>::maxLevel() const
{
    return Levels;
}

template <typename K, typename V, typename Less, size_t Levels>
size_t LockFreeMap<K, V, Less, Levels>::retainedNodeCount() const
{
    return m_store == nullptr ? 0 : m_store->m_nodes.stats().retained;
}

template <typename K, typename V, typename Less, size_t Levels>
size_t LockFreeMap<K, V, Less, Levels>::retainedValueCount() const
{
    return m_store == nullptr ? 0 : m_store->m_values.stats().retained;
}

template <typename K, typename V, typename Less, size_t Levels>
MapValueProxy<K, V, Less, Levels> LockFreeMap<K, V, Less, Levels>::operator[](const K& key)
{
    return MapValueProxy<K, V, Less, Levels>(this, key);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapMaintain(size_t budget, size_t& releasedCapacity)
{
    releasedCapacity = 0;
    if (m_store == nullptr)
    {
        return m_initialStatus;
    }
    LockFreeMapGuard guard(m_store->m_domain);
    if (guard.m_record == nullptr)
    {
        return LOCK_FREE_MAP_RECORD_LIMIT;
    }
    m_store->collect(budget, nullptr, true);
    size_t nodes = 0;
    size_t values = 0;
    LockFreeMapStatus first = m_store->m_nodes.maintain(budget, nodes);
    LockFreeMapStatus second = m_store->m_values.maintain(budget, values);
    releasedCapacity = nodes > (std::numeric_limits<size_t>::max)() - values ? (std::numeric_limits<size_t>::max)() : nodes + values;
    return first != LOCK_FREE_MAP_OK ? first : second;
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStats LockFreeMap<K, V, Less, Levels>::lockFreeMapStats() const
{
    LockFreeMapStats result = {};
    if (m_store != nullptr)
    {
        result.nodes = m_store->m_nodes.stats();
        result.values = m_store->m_values.stats();
        result.indexBytes = m_store->m_indexBytes.load();
        const size_t metadata[] = { sizeof(*m_store), sizeof(*this), sizeof(LockFreeMapIdentity),
            m_store->m_domain.bytes(), m_store->m_nodes.metadataBytes(), m_store->m_values.metadataBytes(),
            m_store->m_indexCount * sizeof(LockFreeMapIndex<K, V>),
            m_store->m_indexCount * sizeof(LockFreeMapIndexBuffer<K, V>) };
        const size_t limit = (std::numeric_limits<size_t>::max)();
        for (size_t index = 0; index < 8; ++index)
        {
            result.metadataBytes = result.metadataBytes > limit - metadata[index] ? limit : result.metadataBytes + metadata[index];
        }
        // 合计字节超过 size_t 时饱和，避免统计回绕
        const size_t parts[] = { result.nodes.bytes, result.values.bytes, result.indexBytes, result.metadataBytes };
        for (size_t index = 0; index < 4; ++index)
        {
            result.totalBytes = result.totalBytes > limit - parts[index] ? limit : result.totalBytes + parts[index];
        }
        result.activeOperations = m_store->m_domain.active();
        const uint64_t structural = m_store->m_retries.load();
        const uint64_t values = m_store->m_domain.valueRetries();
        const uint64_t maximum = (std::numeric_limits<uint64_t>::max)();
        result.casRetries = structural > maximum - values ? maximum : structural + values;
    }
    return result;
}

#endif