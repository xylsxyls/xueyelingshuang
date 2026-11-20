#ifndef LOCK_FREE_MAP_IMPLEMENTATION_INL
#define LOCK_FREE_MAP_IMPLEMENTATION_INL

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMap<K, V, Less, Levels>::LockFreeMap() :
LockFreeMap(true)
{

}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMap<K, V, Less, Levels>::LockFreeMap(bool initializeImmediately) :
m_store(nullptr),
m_initialStatus(LOCK_FREE_MAP_INVALID)
{
    static_assert(Levels > 0, "LockFreeMap MaxLevel must be positive");
    // 比较器内嵌在普通 C++11 new 分配的存储中，过度对齐会使整块存储失去对齐保证。
    static_assert(std::alignment_of<Less>::value <= std::alignment_of<std::max_align_t>::value,
        "LockFreeMap comparator requires unsupported extended alignment");
    try
    {
        if (initializeImmediately)
        {
            m_initialStatus = lockFreeMapConfigure(LockFreeMapConfig());
        }
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
        config.operationRecordLimit == 0 || config.maintenanceBudget == 0 || config.maintenanceInterval == 0 ||
        config.capacitySleepMilliseconds <= 0 ||
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
template <uint32_t Mode, uint32_t Outputs>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::read(const K* key, K* outputKey, V* outputValue,
    LockFreeMapToken* outputToken) const
#ifdef __EMSCRIPTEN__
noexcept try
#endif
{
    // 配置和析构要求外部静止；当前调用复用固定存储地址，避免跨辅助调用重复加载。
    LockFreeMapIndexStore<K, V, Less>* const store = m_store;
    if (store == nullptr)
    {
        return m_initialStatus;
    }
    // 领取与首次源发布合并，protect 仍完成登记后的根验证
    LockFreeMapGuard guard(store->m_domain, store->m_root);
    if (guard.m_record == nullptr)
    {
        return LOCK_FREE_MAP_RECORD_LIMIT;
    }
    try
    {
        for (;;)
        {
            LockFreeMapIndex<K, V>* root = store->protect(guard);
            // 指定键由 find 同时确认存在及位置；只有边界查询需要另取可见计数。
            const size_t visibleCount = Mode == 0 || root == nullptr ? 0 : root->count();
            if (Mode != 0 && visibleCount == 0)
            {
                return LOCK_FREE_MAP_NOT_FOUND;
            }
            LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* node = nullptr;
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
            if (Mode == 0)
            {
                node = store->findKey(root, *key, guard.m_record);
                if (node == nullptr)
                {
                    return LOCK_FREE_MAP_NOT_FOUND;
                }
            }
            else
#endif
            {
                size_t position = 0;
                bool found = false;
                if (Mode == 2)
                {
                    position = visibleCount - 1;
                }
                else if (Mode != 1)
                {
                    position = store->find(root, *key, found, guard.m_record);
                    if (Mode == 0 && !found)
                    {
                        return LOCK_FREE_MAP_NOT_FOUND;
                    }
                    if (Mode == 4 && found)
                    {
                        ++position;
                    }
                }
                if (Mode != 0 && position >= visibleCount)
                {
                    return LOCK_FREE_MAP_NOT_FOUND;
                }
                node = root->entry(position).m_key;
            }
            LockFreeMapPoolSlot<V>* current = (Outputs & 2) != 0 || (Outputs & 4) != 0 ?
                store->protectValue(node, guard) : node->value().m_value.load();
            // 指定键查询持有该键的旧索引及值保护，可在与删除重叠的区间内完成观察
            // 不相关键改变集合不应迫使它重来；边界查询仍须验证整个集合
            if (Mode == 0 && current == nullptr)
            {
                return LOCK_FREE_MAP_NOT_FOUND;
            }
            if (Mode != 0 && (current == nullptr || root != store->m_root.load()))
            {
                guard.conflict();
                continue;
            }
            if ((Outputs & 1) != 0)
            {
                *outputKey = node->value().m_key;
            }
            if ((Outputs & 2) != 0)
            {
                *outputValue = current->value();
            }
            if ((Outputs & 4) != 0)
            {
                outputToken->m_identity = store->m_identity;
                outputToken->m_version = current->m_version;
            }
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
            if ((Outputs & 6) != 0)
            {
                // 有值或版本输出时已发布值保护；所有输出完成后统一归还，不提前开放记录。
                guard.finishRead();
            }
#endif
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
template <typename FunctionType>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapVisit(const K& key, const FunctionType& function) const
{
    // 配置和析构要求外部静止；当前调用复用固定存储地址，避免跨辅助调用重复加载。
    LockFreeMapIndexStore<K, V, Less>* const store = m_store;
    if (store == nullptr)
    {
        return m_initialStatus;
    }
    LockFreeMapGuard guard(store->m_domain, store->m_root);
    if (guard.m_record == nullptr)
    {
        return LOCK_FREE_MAP_RECORD_LIMIT;
    }
    try
    {
        LockFreeMapIndex<K, V>* root = store->protect(guard);
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
        auto* node = store->findKey(root, key, guard.m_record);
        if (node == nullptr)
        {
            return LOCK_FREE_MAP_NOT_FOUND;
        }
#else
        bool found = false;
        const size_t position = store->find(root, key, found, guard.m_record);
        if (!found)
        {
            return LOCK_FREE_MAP_NOT_FOUND;
        }
        LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* node = root->entry(position).m_key;
#endif
        LockFreeMapPoolSlot<V>* current = store->protectValue(node, guard);
        if (current == nullptr)
        {
            return LOCK_FREE_MAP_NOT_FOUND;
        }
        // 所有重试发生在回调之前；回调一旦开始就不能因并发更新再次执行
        function(static_cast<const V&>(current->value()));
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
        guard.finishRead();
#endif
        return LOCK_FREE_MAP_OK;
    }
    catch (...)
    {
        return LOCK_FREE_MAP_USER_EXCEPTION;
    }
}

template <typename K, typename V, typename Less, size_t Levels>
template <uint32_t Mode, bool RetryContention, uint32_t Outputs>
#if defined(__EMSCRIPTEN__) && defined(__clang__) && defined(__OPTIMIZE__)
// 内部重试仍复用原有异常捕获，避免增加热路径的异常调用边界
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
    // 维护期间根已变化时，下一次重新保护后跳过一次重复维护
    bool maintainedBeforeRetry = false;
    // 每轮持有独立守卫；内部竞争直接重试，候选及保护记录先在本轮归还
    // 条件操作始终使用原凭据，不将版本失效转成无条件写入
    for (;;)
    {
        // 领取与首次源发布合并，protect 仍完成登记后的根验证
        LockFreeMapGuard guard(store->m_domain, store->m_root);
        if (guard.m_record == nullptr)
        {
            return LOCK_FREE_MAP_RECORD_LIMIT;
        }
        LockFreeMapIndex<K, V>* root = store->protect(guard);
        const size_t count = root == nullptr ? 0 : root->count();
        const bool maintenanceDue = ++guard.m_record->m_maintenanceTick == store->m_maintenanceInterval;
        if (maintenanceDue)
        {
            guard.m_record->m_maintenanceTick = 0;
        }
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
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
                if (std::is_integral<K>::value && std::is_same<Less, std::less<K>>::value && count == 1)
                {
                    // 单项逻辑视图直接核对真实键；entry仍处理省略项及未合并的覆盖项。
                    // 根保护覆盖键的生命期，不把历史位置提示作为排序或存在性的依据。
                    const K& candidate = root->entry(0).m_key->value().m_key;
                    position = store->m_less(candidate, *key) ? 1 : 0;
                    found = position == 0 && !store->m_less(*key, candidate);
                }
                else
                {
                    // 多记录且默认整数排序时尝试小步长预测；其他场景保持通用查找。
                    position = Mode == 0 && std::is_integral<K>::value && std::is_same<Less, std::less<K>>::value &&
                        count > 1 && store->m_domain.multipleRecords() ?
                        store->findForWrite(root, *key, found, guard.m_record) :
                        store->find(root, *key, found, guard.m_record);
                }
#else
                position = store->find(root, *key, found, guard.m_record);
#endif
            }
            const bool deleting = Mode == 2 || Mode == 4 || Mode == 5;
            const bool logicalOnly = Mode == 2 && (Outputs & 2) == 0 && !store->m_boundedValues;
            // 无输出的逻辑删除只提交键集合；根 CAS 会验证存在性，不需要读取目标值缓存行
            LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* node = found && !logicalOnly ? root->entry(position).m_key : nullptr;
            LockFreeMapPoolSlot<V>* previous = nullptr;
            if (found && !logicalOnly && Mode != 0)
            {
                // 条件及输出路径需要借用旧版本；无条件覆盖在提交前才读取一次旧地址
                // 条件操作及输出复制必须保护旧版本；无输出插入仅观察入口是否关闭，不解引用旧值。
                previous = Mode == 1 && Outputs == 0 ? node->value().m_value.load() : store->protectValue(node, guard);
                if (previous == nullptr)
                {
                    guard.conflict(true);
                    if (!RetryContention)
                    {
                        return LOCK_FREE_MAP_BUSY;
                    }
                    continue;
                }
            }
            if (Mode >= 3 && token != nullptr && (!found || previous->m_version != token->m_version))
            {
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
                if (guard.m_record->m_writeFailures == 31)
                {
                    store->m_domain.requestConditional();
                }
#endif
                guard.conflict(true);
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
                    guard.conflict(true);
                    if (!RetryContention)
                    {
                        return LOCK_FREE_MAP_BUSY;
                    }
                    continue;
                }
                if ((Outputs & 2) != 0)
                {
                    *outputValue = previous->value();
                }
                if ((Outputs & 4) != 0)
                {
                    outputToken->m_identity = store->m_identity;
                    outputToken->m_version = previous->m_version;
                }
                return LOCK_FREE_MAP_EXISTS;
            }
            // 结构写入先回收旧索引，再判断满池是否需要扩容
            // 已有键覆盖只按固定操作次数分摊策略维护
            const bool maintenanceNeeded = maintenanceDue || ((!found || deleting) && store->m_domain.multipleRecords()) ||
                (!found && (root == nullptr || store->m_nodeMaximum >= 0 || store->m_boundedValues));
            const bool skippedMaintenance = maintainedBeforeRetry;
            const bool performMaintenance = maintenanceNeeded && !skippedMaintenance;
            maintainedBeforeRetry = false;
            if (performMaintenance)
            {
                store->collectForWrite(count, guard.m_record);
                size_t released = 0;
                // 自动维护保留已析构的有界槽缓存；容量压力和显式维护才归还空闲缓存，退役对象仍照常回收。
                const bool releaseIdleCaches = false;
                store->m_nodes.maintain(store->m_maintenanceBudget, released, releaseIdleCaches);
                store->m_values.maintain(store->m_maintenanceBudget, released, releaseIdleCaches);
                if (root != store->m_root.load())
                {
                    // 回收可能较慢，不能让旧快照继续构造必败候选；守卫先清理旧借用
                    // 仅紧接的一次尝试跳过重复维护，之后的竞争仍保留原回收频率
                    maintainedBeforeRetry = true;
                    guard.conflict(true);
                    if (!RetryContention)
                    {
                        return LOCK_FREE_MAP_BUSY;
                    }
                    continue;
                }
            }
            if (Mode == 5 && (Outputs & 1) != 0)
            {
                *outputKey = node->value().m_key;
            }
            if ((Mode == 1 || Mode == 2 || Mode == 5) && (Outputs & 2) != 0)
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
                if (ownedValue == nullptr && skippedMaintenance &&
                    (status == LOCK_FREE_MAP_CAPACITY || status == LOCK_FREE_MAP_RESERVED ||
                    status == LOCK_FREE_MAP_RECORD_LIMIT))
                {
                    // 旧根在上轮维护后才解除保护；报告资源不足前补回这次索引回收
                    store->collectForWrite(count, guard.m_record);
                    ownedValue = store->m_values.create(*value, version, guard, status);
                }
                if (ownedValue == nullptr)
                {
                    guard.conflict(true);
                    if (status == LOCK_FREE_MAP_CONFLICT)
                    {
                        if (!RetryContention)
                        {
                            return LOCK_FREE_MAP_BUSY;
                        }
                        continue;
                    }
                    return status;
                }
                if (!found)
                {
                    // 候选仍是私有对象，直接构造最终指针，根 CAS 才负责发布
                    ownedKey = store->m_nodes.emplace(0, guard, status, *key, store->m_values, ownedValue);
                    if (ownedKey == nullptr && skippedMaintenance &&
                        (status == LOCK_FREE_MAP_CAPACITY || status == LOCK_FREE_MAP_RESERVED ||
                        status == LOCK_FREE_MAP_RECORD_LIMIT))
                    {
                        store->collectForWrite(count, guard.m_record);
                        ownedKey = store->m_nodes.emplace(0, guard, status, *key, store->m_values, ownedValue);
                    }
                    if (ownedKey == nullptr)
                    {
                        store->m_values.release(ownedValue);
                        guard.conflict(true);
                        if (status == LOCK_FREE_MAP_CONFLICT)
                        {
                            if (!RetryContention)
                            {
                                return LOCK_FREE_MAP_BUSY;
                            }
                            continue;
                        }
                        return status;
                    }
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
#if defined(_MSC_VER) && defined(_M_IX86)
                // MSVC 三十二位的六十四位统计读写会执行原子读改写；按有界批次公布失败数
                uint32_t pendingRetries = 0;
#endif
                // 无条件覆盖在值竞争时复用候选；根变化或键关闭后由外层释放候选并重新查找
                while (previous != nullptr && root == store->m_root.load())
                {
                    if (node->mutableValue().m_value.compare_exchange_strong(previous, ownedValue))
                    {
                        committed = true;
                        break;
                    }
                    if (Mode != 0 || !RetryContention)
                    {
                        break;
                    }
                    guard.conflict(true);
#if defined(_MSC_VER) && defined(_M_IX86)
                    if (++pendingRetries == 32)
                    {
                        guard.m_record->addValueRetries(pendingRetries);
                        pendingRetries = 0;
                    }
#else
                    const uint64_t retries = guard.m_record->m_valueRetries.load(std::memory_order_relaxed);
                    if (retries != (std::numeric_limits<uint64_t>::max)())
                    {
                        guard.m_record->m_valueRetries.store(retries + 1, std::memory_order_relaxed);
                    }
#endif
                }
#if defined(_MSC_VER) && defined(_M_IX86)
                // 无论提交成功还是因根变化退出，都在结束本次循环前公布余数
                if (pendingRetries != 0)
                {
                    guard.m_record->addValueRetries(pendingRetries);
                }
#endif
                if (committed)
                {
                    ownedValue = nullptr;
                    if (Mode >= 3)
                    {
                        // 成功 CAS 已接管旧值的一份拥有引用；借用结束使用 release 发布此前读取完成
                        guard.m_record->m_pointer[1].m_value.store(nullptr, std::memory_order_release);
                    }
#ifndef __EMSCRIPTEN__
                    // 顺序一致摘除后登记高水位仍为一，没有其他调用提前借用旧值；新登记者只能取得新值。
                    // 历史描述符仍拥有引用时，releaseDetached 自行回到引用计数 CAS 路径。
                    if (!store->m_domain.multipleRecords())
                    {
                        store->m_values.releaseDetached(previous, guard.m_record);
                    }
                    else
#endif
                    {
                        store->m_values.release(previous, guard.m_record);
                    }
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
                    store->collectForWrite(count, guard.m_record);
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
                        guard.structuralConflict();
                    }
                }
            }
            if (Mode == 1 && status == LOCK_FREE_MAP_COMMITTED && (Outputs & 4) != 0)
            {
                outputToken->m_identity = store->m_identity;
                outputToken->m_version = version;
            }
            if (status == LOCK_FREE_MAP_COMMITTED)
            {
                // 提交分支已经转交全部候选所有权，成功返回前结束剩余根保护
                // 候选已转交，辅助保护已在提交分支解除；结束根保护后不再访问记录
                guard.finishWrite();
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
                if (Mode >= 3)
                {
                    // 只响应同类条件操作的竞争提示，普通覆盖及查询没有提交后退避
                    store->m_domain.finishConditional();
                }
#endif
                return LOCK_FREE_MAP_COMMITTED;
            }
            if (status == LOCK_FREE_MAP_CONFLICT)
            {
                guard.conflict(true);
                LockFreeMapPool<V>::increment(store->m_retries);
            }
            else if (status == LOCK_FREE_MAP_RECORD_LIMIT || status == LOCK_FREE_MAP_RESERVED || status == LOCK_FREE_MAP_CAPACITY)
            {
                guard.conflict(true);
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
        if (status != LOCK_FREE_MAP_CONFLICT)
        {
            return status;
        }
        if (!RetryContention)
        {
            return LOCK_FREE_MAP_BUSY;
        }
    }
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
bool LockFreeMap<K, V, Less, Levels>::overlaps(const void* input, size_t inputSize, const void* output, size_t outputSize)
{
    if (input == nullptr || output == nullptr)
    {
        return false;
    }
    const uintptr_t first = reinterpret_cast<uintptr_t>(input);
    const uintptr_t second = reinterpret_cast<uintptr_t>(output);
    return first <= second ? second - first < inputSize : first - second < outputSize;
}

template <typename K, typename V, typename Less, size_t Levels>
template <uint32_t Mode, uint32_t Outputs>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::writeRetry(const K* key, const V* value,
    const LockFreeMapToken* token, K* outputKey, V* outputValue, LockFreeMapToken* outputToken,
    bool retryContention, bool retryCapacity)
{
    // 输出赋值可发生在发布前并触发重试；仅在会覆盖输入时保存原对象。
    // 副本跨越全部重试，使用独立存储避免大键值占用调用栈。
    if (Outputs != 0 && m_store != nullptr)
    {
        const bool saveKey = overlaps(key, sizeof(K), outputValue, sizeof(V));
        const bool saveValue = overlaps(value, sizeof(V), outputValue, sizeof(V));
        const bool saveToken = overlaps(token, sizeof(LockFreeMapToken), outputKey, sizeof(K)) ||
            overlaps(token, sizeof(LockFreeMapToken), outputValue, sizeof(V));
        if (saveKey || saveValue || saveToken)
        {
            try
            {
                const auto stableKey = saveKey ? std::make_shared<K>(*key) : std::shared_ptr<K>();
                const auto stableValue = saveValue ? std::make_shared<V>(*value) : std::shared_ptr<V>();
                const LockFreeMapToken stableToken = saveToken ? *token : LockFreeMapToken();
                return writeRetry<Mode, Outputs>(saveKey ? stableKey.get() : key, saveValue ? stableValue.get() : value,
                    saveToken ? std::addressof(stableToken) : token, outputKey, outputValue, outputToken, retryContention, retryCapacity);
            }
            catch (const std::bad_alloc&)
            {
                return LOCK_FREE_MAP_ALLOCATION_FAILURE;
            }
            catch (...)
            {
                return LOCK_FREE_MAP_USER_EXCEPTION;
            }
        }
    }
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    uint32_t capacityRetries = 0;
#endif
    for (;;)
    {
        const LockFreeMapStatus status = retryContention ? write<Mode, true, Outputs>(key, value, token, outputKey, outputValue, outputToken) :
            write<Mode, false, Outputs>(key, value, token, outputKey, outputValue, outputToken);
        if (!retryCapacity || (status != LOCK_FREE_MAP_CAPACITY && status != LOCK_FREE_MAP_RESERVED &&
            status != LOCK_FREE_MAP_RECORD_LIMIT))
        {
            return status;
        }
        // 单线程宿主不能在同步调用期间释放资源，保留真实错误，不伪报系统故障。
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
        return status;
#else
        // 模块共享实例内的记录可识别跨 DLL 重入；命中只拒绝等待，不授予任何所有权。
        if (m_store == nullptr || m_store->m_domain.mayBeHeldByCurrentThread())
        {
            return status;
        }
        if (!LockFreeMapClock::waitCapacity(capacityRetries, m_store->m_capacityFastRetries, m_store->m_capacitySleepMilliseconds))
        {
            return LOCK_FREE_MAP_SYSTEM_ERROR;
        }
#endif
    }
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::trySet(const K& key, const V& value, bool retryContention, bool retryCapacity)
{
    return writeRetry<0>(std::addressof(key), std::addressof(value), nullptr, nullptr, nullptr, nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::tryInsert(const K& key, const V& value, bool retryContention, bool retryCapacity)
{
    return writeRetry<1>(std::addressof(key), std::addressof(value), nullptr, nullptr, nullptr, nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::tryGetOrInsert(const K& key, const V& candidate, V& value, LockFreeMapToken& token, bool retryContention, bool retryCapacity)
{
    return writeRetry<1, 6>(std::addressof(key), std::addressof(candidate), nullptr, nullptr, std::addressof(value), std::addressof(token), retryContention, retryCapacity);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::tryUpdate(const K& key, const LockFreeMapToken& token, const V& value, bool retryContention, bool retryCapacity)
{
    return writeRetry<3>(std::addressof(key), std::addressof(value), std::addressof(token), nullptr, nullptr, nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::tryErase(const K& key, bool retryContention, bool retryCapacity)
{
    return writeRetry<2>(std::addressof(key), nullptr, nullptr, nullptr, nullptr, nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::tryEraseIf(const K& key, const LockFreeMapToken& token, bool retryContention, bool retryCapacity)
{
    return writeRetry<4>(std::addressof(key), nullptr, std::addressof(token), nullptr, nullptr, nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::tryExtract(const K& key, V& value, bool retryContention, bool retryCapacity)
{
    return writeRetry<2, 2>(std::addressof(key), nullptr, nullptr, nullptr, std::addressof(value), nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::tryExtractLast(const LockFreeMapToken& token, K& key, V& value, bool retryContention, bool retryCapacity)
{
    return writeRetry<5, 3>(nullptr, nullptr, std::addressof(token), std::addressof(key), std::addressof(value), nullptr, retryContention, retryCapacity);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapGet(const K& key, V& value, LockFreeMapToken& token) const
{
    return read<0, 6>(std::addressof(key), nullptr, std::addressof(value), std::addressof(token));
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapGetOrInsert(const K& key, const V& candidate, V& value, LockFreeMapToken& token)
{
    return tryGetOrInsert(key, candidate, value, token, true, false);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapUpdate(const K& key, const LockFreeMapToken& token, const V& value)
{
    return write<3, true>(std::addressof(key), std::addressof(value), std::addressof(token), nullptr, nullptr, nullptr);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapErase(const K& key, const LockFreeMapToken& token)
{
    return write<4, true>(std::addressof(key), nullptr, std::addressof(token), nullptr, nullptr, nullptr);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapExtract(const K& key, V& value)
{
    return tryExtract(key, value, true, false);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapLast(K& key, V& value, LockFreeMapToken& token) const
{
    return read<2, 7>(nullptr, std::addressof(key), std::addressof(value), std::addressof(token));
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapExtractLast(const LockFreeMapToken& token, K& key, V& value)
{
    return tryExtractLast(token, key, value, true, false);
}

template <typename K, typename V, typename Less, size_t Levels>
LockFreeMapStatus LockFreeMap<K, V, Less, Levels>::lockFreeMapSet(const K& key, const V& value)
{
    return write<0, true>(std::addressof(key), std::addressof(value), nullptr, nullptr, nullptr, nullptr);
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::set(const K& key, const V& value)
{
    return lockFreeMapSet(key, value) == LOCK_FREE_MAP_COMMITTED;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::insert(const K& key, const V& value)
{
    return write<1, true>(std::addressof(key), std::addressof(value), nullptr, nullptr, nullptr, nullptr) == LOCK_FREE_MAP_COMMITTED;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::erase(const K& key)
{
    return write<2, true>(std::addressof(key), nullptr, nullptr, nullptr, nullptr, nullptr) == LOCK_FREE_MAP_COMMITTED;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::remove(const K& key)
{
    return erase(key);
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::get(const K& key, V& value) const
{
    return read<0, 2>(std::addressof(key), nullptr, std::addressof(value), nullptr) == LOCK_FREE_MAP_OK;
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
    return read<0, 0>(std::addressof(key), nullptr, nullptr, nullptr) == LOCK_FREE_MAP_OK;
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
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
        // 观察到空根即可在本次读取处完成清空；后来的写入可在线性化点之后提交
        if (m_store->m_root.load() == nullptr)
        {
            return;
        }
#endif
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
template <bool Plain, typename Function>
void LockFreeMap<K, V, Less, Levels>::visitView(LockFreeMapIndexStore<K, V, Less>* store,
    LockFreeMapIndex<K, V>* root, LockFreeMapGuard& guard, Function& function, size_t& completed)
{
    // 根已完成决策且受保护，逻辑条目数和省略位置在本次遍历中固定。
    const size_t rawCount = root->m_count.load();
    const bool removed = !Plain && rawCount != 0 && root->m_decision.load() == 2;
    const size_t omitted = removed ? root->m_omitted : rawCount;
    // 保护期间数组地址和覆盖位置固定；用户回调可重入，不能让别名迫使每项重读这些字段。
    const size_t overridden = Plain ? rawCount : root->m_overridePosition;
    const LockFreeMapIndexEntry<K, V>* const entries = root->m_entries;
    LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* const overrideKey =
        overridden < rawCount && overridden != omitted ? root->m_overrideEntry.m_key : nullptr;
    for (size_t position = 0; position < rawCount; ++position)
    {
        if (!Plain && position == omitted)
        {
            continue;
        }
        LockFreeMapPoolSlot<LockFreeMapKey<K, V>>* node = !Plain && position == overridden ? overrideKey : entries[position].m_key;
        LockFreeMapPoolSlot<V>* value = store->protectValue(node, guard);
        if (value != nullptr)
        {
            function(node->value().m_key, value->value());
            ++completed;
        }
    }
}

template <typename K, typename V, typename Less, size_t Levels>
template <typename Function>
void LockFreeMap<K, V, Less, Levels>::visitAll(Function& function, size_t& completed) const
{
    LockFreeMapIndexStore<K, V, Less>* const store = m_store;
    if (store == nullptr)
    {
        return;
    }
    LockFreeMapGuard guard(store->m_domain, store->m_root);
    if (guard.m_record == nullptr)
    {
        return;
    }
    LockFreeMapIndex<K, V>* root = store->protect(guard);
    if (root != nullptr)
    {
        // 根保护固定键视图；只有已合并且无省略项时才直接遍历连续数组。
        if (root->m_decision.load() != 2 && root->m_merged.load(std::memory_order_acquire) != 0)
        {
            visitView<true>(store, root, guard, function, completed);
        }
        else
        {
            visitView<false>(store, root, guard, function, completed);
        }
    }
}

template <typename K, typename V, typename Less, size_t Levels>
template <typename Function>
size_t LockFreeMap<K, V, Less, Levels>::forEach(Function function) const
{
    size_t completed = 0;
    try
    {
        visitAll(function, completed);
    }
    catch (...)
    {
        return completed;
    }
    return completed;
}

template <typename K, typename V, typename Less, size_t Levels>
template <typename Element, typename Append>
void LockFreeMap<K, V, Less, Levels>::copyAll(std::vector<Element>& result, Append append) const
{
    try
    {
        // 弱一致的数量只用于预留；实际导出项由读保护中的视图决定。
        result.reserve(count());
        size_t completed = 0;
        visitAll(append, completed);
    }
    catch (...)
    {
        // 首次复制失败即退出遍历，归还保护后再销毁已复制的前缀和预留空间。
        std::vector<Element>().swap(result);
    }
}

template <typename K, typename V, typename Less, size_t Levels>
std::vector<K> LockFreeMap<K, V, Less, Levels>::keys() const
{
    std::vector<K> result;
    /** 复制当前借用键
    @param [in] key 本项键
    @param [in] value 本项值，导出键时忽略
    */
    copyAll(result, [&result](const K& key, const V& value)
    {
        (void)value;
        result.push_back(key);
    });
    return result;
}

template <typename K, typename V, typename Less, size_t Levels>
std::vector<V> LockFreeMap<K, V, Less, Levels>::values() const
{
    std::vector<V> result;
    /** 复制当前借用值
    @param [in] key 本项键，导出值时忽略
    @param [in] value 本项值
    */
    copyAll(result, [&result](const K& key, const V& value)
    {
        (void)key;
        result.push_back(value);
    });
    return result;
}

template <typename K, typename V, typename Less, size_t Levels>
std::vector<std::pair<K, V>> LockFreeMap<K, V, Less, Levels>::toVector() const
{
    std::vector<std::pair<K, V>> result;
    /** 从受保护条目直接构造输出键值对
    @param [in] key 本项键
    @param [in] value 本项值
    */
    copyAll(result, [&result](const K& key, const V& value)
    {
        result.emplace_back(key, value);
    });
    return result;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::first(K& key, V& value) const
{
    return read<1, 3>(nullptr, std::addressof(key), std::addressof(value), nullptr) == LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::last(K& key, V& value) const
{
    return read<2, 3>(nullptr, std::addressof(key), std::addressof(value), nullptr) == LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::lowerBound(const K& key, K& foundKey, V& value) const
{
    return read<3, 3>(std::addressof(key), std::addressof(foundKey), std::addressof(value), nullptr) == LOCK_FREE_MAP_OK;
}

template <typename K, typename V, typename Less, size_t Levels>
bool LockFreeMap<K, V, Less, Levels>::upperBound(const K& key, K& foundKey, V& value) const
{
    return read<4, 3>(std::addressof(key), std::addressof(foundKey), std::addressof(value), nullptr) == LOCK_FREE_MAP_OK;
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
#ifdef _MSC_VER
__forceinline
#else
inline
#endif
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