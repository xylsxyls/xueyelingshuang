#ifndef LOCK_FREE_MAP_POOL_INL
#define LOCK_FREE_MAP_POOL_INL

template <typename T>
void LockFreeMapPoolDestroy<T>::destroy(T& value, LockFreeMapHazardRecord* record)
{
    (void)record;
    value.~T();
}

template <typename T>
LockFreeMapPoolSlot<T>::LockFreeMapPoolSlot() :
m_references(0),
m_block(nullptr),
m_version(0)
{

}

template <typename T>
const T& LockFreeMapPoolSlot<T>::value() const
{
    return *reinterpret_cast<const T*>(&m_storage);
}

template <typename T>
T& LockFreeMapPoolSlot<T>::mutableValue()
{
    return *reinterpret_cast<T*>(&m_storage);
}

template <typename T>
LockFreeMapPoolBlock<T>::LockFreeMapPoolBlock(size_t count) :
m_occupancy(0),
m_activity(1),
m_slots(new (std::nothrow) LockFreeMapPoolSlot<T>[count]),
m_count(m_slots == nullptr ? 0 : count),
m_hint(0),
m_directory(0),
m_idleActivity(0),
m_idleStart(0)
{
    for (size_t index = 0; index < m_count; ++index)
    {
        m_slots[index].m_block = this;
    }
}

template <typename T>
LockFreeMapPoolBlock<T>::~LockFreeMapPoolBlock()
{
    delete[] m_slots;
}

template <typename T>
LockFreeMapPoolRecord<T>::LockFreeMapPoolRecord() :
m_state(0),
m_block(nullptr),
m_retired(nullptr)
{

}

template <typename T>
LockFreeMapPool<T>::LockFreeMapPool(const LockFreeMapPoolConfig& config, LockFreeMapReclamation& domain, bool deferred) :
m_config(config),
m_domain(&domain),
m_records(new (std::nothrow) LockFreeMapPoolRecord<T>[config.blockRecordLimit]),
m_budget(0),
m_capacity(0),
m_openCapacity(0),
m_retained(0),
m_inUse(0),
m_bytes(0),
m_cursor(0),
m_grows(0),
m_allocations(0),
m_lastGrow(0),
m_deferred(deferred),
m_canShrink(true),
m_pending(0),
m_span(0),
m_hint(0)
{
    const auto* policy = m_config.shrink.target<LockFreeMapShrinkDecision(*)(const LockFreeMapShrinkContext&)>();
    m_canShrink = m_config.shrink && (policy == nullptr || *policy != &LockFreeMapPoolConfig::defaultShrink);
    // 标准 new[] 只保证基本对齐，扩展对齐需要其他分配后端
    // 当前通过编译期检查拒绝，防止构造未正确对齐的对象
    static_assert(std::alignment_of<T>::value <= std::alignment_of<std::max_align_t>::value,
        "LockFreeMap payload requires unsupported extended alignment");
}

template <typename T>
LockFreeMapPool<T>::~LockFreeMapPool()
{
    freeMemory();
    delete[] m_records;
}

template <typename T>
bool LockFreeMapPool<T>::valid(const LockFreeMapPoolConfig& config)
{
    const size_t limit = (std::numeric_limits<size_t>::max)();
    const size_t countLimit = limit / (sizeof(LockFreeMapPoolBlock<T>) + sizeof(LockFreeMapPoolSlot<T>));
    if (config.maximumCapacity < -1 || config.blockRecordLimit == 0 ||
        config.blockRecordLimit > limit / sizeof(LockFreeMapPoolRecord<T>) ||
        config.initialCapacity > countLimit || config.initialCapacity >= limit / 2)
    {
        return false;
    }
    if (config.maximumCapacity >= 0)
    {
        const uint64_t maximum = static_cast<uint64_t>(config.maximumCapacity);
        if (maximum > countLimit || maximum >= limit / 2 || maximum < config.initialCapacity)
        {
            return false;
        }
    }
    return true;
}

template <typename T>
void LockFreeMapPool<T>::increment(std::atomic<uint64_t>& counter)
{
    uint64_t current = counter.load();
    while (current != (std::numeric_limits<uint64_t>::max)() &&
        !counter.compare_exchange_weak(current, current + 1))
    {

    }
}

template <typename T>
uint64_t LockFreeMapPool<T>::now()
{
    return LockFreeMapClock::milliseconds();
}

template <typename T>
LockFreeMapStatus LockFreeMapPool<T>::initialize()
{
    if (m_records == nullptr)
    {
        return LOCK_FREE_MAP_ALLOCATION_FAILURE;
    }
    return m_config.initialCapacity == 0 ? LOCK_FREE_MAP_OK : grow(true);
}

template <typename T>
LockFreeMapStatus LockFreeMapPool<T>::grow(bool initial)
{
    if (m_records == nullptr)
    {
        return LOCK_FREE_MAP_ALLOCATION_FAILURE;
    }
    size_t requested = m_config.initialCapacity;
    if (!initial)
    {
        LockFreeMapGrowContext context;
        context.initialCapacity = m_config.initialCapacity;
        context.successfulGrowCount = m_grows.load();
        context.currentCapacity = m_capacity.load();
        try
        {
            requested = m_config.grow ? m_config.grow(context) : 0;
        }
        catch (...)
        {
            return LOCK_FREE_MAP_POLICY_EXCEPTION;
        }
    }
    if (requested == 0)
    {
        return LOCK_FREE_MAP_GROW_DENIED;
    }
    const size_t limit = (std::numeric_limits<size_t>::max)();
    // 总字节上限包含每槽独占一块时的最坏块头开销
    size_t maximum = limit / (sizeof(LockFreeMapPoolSlot<T>) + sizeof(LockFreeMapPoolBlock<T>));
    if (m_config.maximumCapacity >= 0 && static_cast<uint64_t>(m_config.maximumCapacity) < maximum)
    {
        maximum = static_cast<size_t>(m_config.maximumCapacity);
    }
    size_t previous = m_budget.load();
    size_t count = 0;
    for (;;)
    {
        if (previous >= maximum)
        {
            return m_capacity.load() < previous ? LOCK_FREE_MAP_RESERVED : LOCK_FREE_MAP_CAPACITY;
        }
        count = requested < maximum - previous ? requested : maximum - previous;
        if (m_budget.compare_exchange_weak(previous, previous + count))
        {
            break;
        }
    }
    LockFreeMapPoolRecord<T>* record = nullptr;
    for (size_t index = 0; index < m_config.blockRecordLimit; ++index)
    {
        uint32_t expected = 0;
        if (m_records[index].m_state.compare_exchange_strong(expected, 1))
        {
            record = &m_records[index];
            break;
        }
    }
    if (record == nullptr)
    {
        m_budget.fetch_sub(count);
        return LOCK_FREE_MAP_RECORD_LIMIT;
    }
    LockFreeMapPoolBlock<T>* block = nullptr;
    try
    {
        block = new (std::nothrow) LockFreeMapPoolBlock<T>(count);
    }
    catch (...)
    {
        record->m_state.store(0);
        m_budget.fetch_sub(count);
        return LOCK_FREE_MAP_ALLOCATION_FAILURE;
    }
    if (block == nullptr || block->m_slots == nullptr)
    {
        delete block;
        record->m_state.store(0);
        m_budget.fetch_sub(count);
        return LOCK_FREE_MAP_ALLOCATION_FAILURE;
    }
    increment(m_allocations);
    increment(m_allocations);
    m_bytes.fetch_add(sizeof(*block) + count * sizeof(LockFreeMapPoolSlot<T>));
    size_t span = m_span.load();
    const size_t position = static_cast<size_t>(record - m_records);
    while (span <= position && !m_span.compare_exchange_weak(span, position + 1))
    {

    }
    block->m_directory = position;
    record->m_block.store(block);
    // 先发布再记账，并发统计可能暂时少计本批容量
    // 但不能把尚未申请的预留额度报告成已分配容量
    // 状态 1 保证全部记账完成前不能退役该块
    m_capacity.fetch_add(count);
    m_openCapacity.fetch_add(count);
    if (!initial)
    {
        increment(m_grows);
        // 延迟发布者不能用旧时间覆盖较新的扩容样本
        const uint64_t sample = now();
        uint64_t previous = m_lastGrow.load();
        while (previous < sample && !m_lastGrow.compare_exchange_weak(previous, sample))
        {

        }
    }
    record->m_state.store(2);
    return LOCK_FREE_MAP_OK;
}

template <typename T>
LockFreeMapPoolSlot<T>* LockFreeMapPool<T>::take(LockFreeMapGuard& guard)
{
    const size_t closed = (std::numeric_limits<size_t>::max)() / 2 + 1;
    const size_t channel = m_deferred ? 1 : 0;
    if (!m_canShrink && guard.m_record->m_poolOwner[channel] == this && guard.m_record->m_poolCache[channel] != nullptr)
    {
        LockFreeMapPoolSlot<T>* cached = static_cast<LockFreeMapPoolSlot<T>*>(guard.m_record->m_poolCache[channel]);
        guard.m_record->m_poolCache[channel] = nullptr;
        // 原始槽仍由本记录独占，缓存态和构造态均不含有效对象，无需中间状态写入
        return cached;
    }
    const size_t span = m_span.load();
    const size_t hint = m_canShrink ? m_hint.load(std::memory_order_relaxed) : guard.m_record->m_directoryHint[channel];
    size_t directory = hint < span ? hint : 0;
    for (size_t visit = 0; visit < span; ++visit)
    {
        const size_t directoryIndex = directory;
        if (++directory == span)
        {
            directory = 0;
        }
        LockFreeMapPoolRecord<T>& record = m_records[directoryIndex];
        LockFreeMapPoolBlock<T>* block = record.m_block.load();
        if (block == nullptr)
        {
            continue;
        }
        bool pinned = false;
        if (m_canShrink)
        {
            guard.m_record->m_pointer[2].store(block);
            if (record.m_block.load() != block)
            {
                continue;
            }
            size_t occupied = block->m_occupancy.load();
            while (occupied < closed - 1 && occupied < block->m_count)
            {
                if (block->m_occupancy.compare_exchange_weak(occupied, occupied + 1))
                {
                    increment(block->m_activity);
                    pinned = true;
                    break;
                }
            }
            guard.m_record->m_pointer[2].store(nullptr);
            if (!pinned)
            {
                continue;
            }
        }
        // 永久禁用自动缩容时，目录和块保持到外部静止期释放
        // 槽所有权仍由 CAS 取得
        // 此时无需额外块保护和连续空闲计数
        size_t position = m_canShrink ? block->m_hint.load(std::memory_order_relaxed) : guard.m_record->m_slotHint[channel];
        if (!m_canShrink && m_deferred && m_domain->multipleRecords())
        {
            // 并发覆盖优先复用各操作记录附近的槽，避免所有写者追逐刚释放的同一缓存行
            // 槽仍由 CAS 取得；本段占满后继续检查整块，不保留私有空槽
            position = guard.m_record->m_poolHome % block->m_count;
        }
        if (position >= block->m_count)
        {
            position = 0;
        }
        for (size_t slotIndex = 0; slotIndex < block->m_count; ++slotIndex)
        {
            LockFreeMapPoolSlot<T>& slot = block->m_slots[position];
            if (++position == block->m_count)
            {
                position = 0;
            }
            size_t expected = 0;
            if (slot.m_references.load(std::memory_order_relaxed) == 0 &&
                slot.m_references.compare_exchange_strong(expected, (std::numeric_limits<size_t>::max)() - 2))
            {
                if (m_canShrink)
                {
                    block->m_hint.store(position, std::memory_order_relaxed);
                    m_hint.store(directoryIndex, std::memory_order_relaxed);
                    m_inUse.fetch_add(1);
                }
                else
                {
                    guard.m_record->m_slotHint[channel] = position;
                    guard.m_record->m_directoryHint[channel] = directoryIndex;
                }
                return &slot;
            }
        }
        if (pinned)
        {
            block->m_occupancy.fetch_sub(1);
        }
    }
    if (m_canShrink)
    {
        guard.m_record->m_pointer[2].store(nullptr);
    }
    else
    {
        // 扩容或容量失败前先转取空闲记录的缓存；暂停的活动记录不参与转取
        LockFreeMapPoolSlot<T>* cached = static_cast<LockFreeMapPoolSlot<T>*>(m_domain->takeIdleCache(this, channel));
        if (cached != nullptr)
        {
            return cached;
        }
    }
    return nullptr;
}

template <typename T>
#if defined(__EMSCRIPTEN__) && defined(__clang__) && defined(__OPTIMIZE__)
// 复用已有构造异常处理，避免值池薄包装单独跨越异常边界
__attribute__((always_inline)) inline
#endif
LockFreeMapPoolSlot<T>* LockFreeMapPool<T>::create(const T& value, uint64_t version,
    LockFreeMapGuard& guard, LockFreeMapStatus& status)
{
    return emplace(version, guard, status, value);
}

template <typename T>
template <typename... Arguments>
#if defined(__EMSCRIPTEN__) && defined(__clang__) && defined(__OPTIMIZE__)
// 仅合并 Wasm 热点调用边界，保留异常捕获和资源归还路径
__attribute__((always_inline)) inline
#endif
LockFreeMapPoolSlot<T>* LockFreeMapPool<T>::emplace(uint64_t version,
    LockFreeMapGuard& guard, LockFreeMapStatus& status, Arguments&&... arguments)
{
    LockFreeMapPoolSlot<T>* slot = take(guard);
    if (slot == nullptr)
    {
        size_t released = 0;
        maintain(m_config.blockRecordLimit, released);
        slot = take(guard);
        if (slot == nullptr)
        {
            status = grow(false);
            slot = take(guard);
            if (slot == nullptr)
            {
                if (status == LOCK_FREE_MAP_OK)
                {
                    // 本次已发布实际槽位，但被其他竞争者抢先取得
                    // 返回冲突允许重试，这表示其他操作已经推进
                    // 不表示等待唯一分配者发布私人批次
                    status = LOCK_FREE_MAP_CONFLICT;
                }
                return nullptr;
            }
        }
    }
    try
    {
        new (&slot->m_storage) T(std::forward<Arguments>(arguments)...);
    }
    catch (...)
    {
        if (m_canShrink)
        {
            m_inUse.fetch_sub(1);
        }
        slot->m_references.store(0, std::memory_order_release);
        if (m_canShrink)
        {
            slot->m_block->m_occupancy.fetch_sub(1);
        }
        status = LOCK_FREE_MAP_USER_EXCEPTION;
        return nullptr;
    }
    slot->m_version = version;
    if (m_canShrink)
    {
        m_retained.fetch_add(1);
    }
    slot->m_references.store(1, std::memory_order_release);
    status = LOCK_FREE_MAP_OK;
    if (m_config.growFreeThreshold != 0)
    {
        size_t capacity = m_openCapacity.load();
        size_t occupied = used();
        if (capacity <= occupied || capacity - occupied <= m_config.growFreeThreshold)
        {
            size_t released = 0;
            maintain(m_config.blockRecordLimit, released);
            capacity = m_openCapacity.load();
            occupied = used();
            // 预扩容失败不得撤销已经构造成功的候选
            if (capacity <= occupied || capacity - occupied <= m_config.growFreeThreshold)
            {
                grow(false);
            }
        }
    }
    return slot;
}

template <typename T>
void LockFreeMapPool<T>::retain(LockFreeMapPoolSlot<T>* slot)
{
    slot->m_references.fetch_add(1);
}

template <typename T>
bool LockFreeMapPool<T>::tryRetain(LockFreeMapPoolSlot<T>* slot)
{
    size_t references = slot->m_references.load();
    while (references != 0 && references < (std::numeric_limits<size_t>::max)() - 3)
    {
        if (slot->m_references.compare_exchange_weak(references, references + 1))
        {
            return true;
        }
    }
    return false;
}

template <typename T>
void LockFreeMapPool<T>::release(LockFreeMapPoolSlot<T>* slot, LockFreeMapHazardRecord* record)
{
    if (slot == nullptr)
    {
        return;
    }
    size_t references = slot->m_references.load();
    for (;;)
    {
        const size_t desired = references == 1 ? (std::numeric_limits<size_t>::max)() : references - 1;
        if (slot->m_references.compare_exchange_weak(references, desired))
        {
            if (references == 1)
            {
                if (m_deferred && m_domain->protectsValue(slot))
                {
                    m_pending.fetch_add(1);
                    slot->m_references.store((std::numeric_limits<size_t>::max)() - 1);
                }
                else
                {
                    destroy(slot, record);
                }
            }
            return;
        }
    }
}

template <typename T>
void LockFreeMapPool<T>::destroy(LockFreeMapPoolSlot<T>* slot, LockFreeMapHazardRecord* record)
{
    LockFreeMapPoolDestroy<T>::destroy(slot->mutableValue(), record);
    const size_t channel = m_deferred ? 1 : 0;
    if (!m_canShrink && record != nullptr && record->m_poolCache[channel] == nullptr)
    {
        record->m_poolOwner[channel] = this;
        record->m_poolCache[channel] = slot;
        // 这里只保留原始存储，旧键、旧值和它们拥有的资源已经全部析构
        // 直接复用不读取块或计算坐标；只有归还公共空槽才需要刷新查找提示
        slot->m_references.store((std::numeric_limits<size_t>::max)() - 3, std::memory_order_release);
        return;
    }
    LockFreeMapPoolBlock<T>* block = slot->m_block;
    if (m_canShrink)
    {
        m_retained.fetch_sub(1);
        m_inUse.fetch_sub(1);
    }
    const size_t position = static_cast<size_t>(slot - block->m_slots);
    if (!m_canShrink && record != nullptr)
    {
        // 只缓存目录和槽坐标，不缓存借用指针
        // 显式释放可能清空全部块，take 必须重新校验两个坐标
        // 即使其他线程已复用该槽，也必须通过 CAS 重新取得所有权
        const size_t channel = m_deferred ? 1 : 0;
        record->m_directoryHint[channel] = block->m_directory;
        record->m_slotHint[channel] = position;
    }
    slot->m_references.store(0, std::memory_order_release);
    if (m_canShrink)
    {
        block->m_hint.store(position, std::memory_order_relaxed);
        block->m_occupancy.fetch_sub(1);
    }
}

template <typename T>
void LockFreeMapPool<T>::releaseDetached(LockFreeMapPoolSlot<T>* slot, LockFreeMapHazardRecord* record)
{
    if (slot == nullptr || slot->m_references.load() != 1)
    {
        release(slot, record);
        return;
    }
    // 唯一拥有者且没有新引用入口，其他线程不能再改变引用数
    // 值借用仍需检查 hazard；摘除入口的顺序一致发布已经先于本次扫描
#ifndef __EMSCRIPTEN__
    slot->m_references.store((std::numeric_limits<size_t>::max)(), std::memory_order_relaxed);
#endif
    // Wasm 保持独占引用到退役或归还；不会开放空槽，也不会跳过值保护检查
    if (m_deferred && m_domain->protectsValue(slot))
    {
        m_pending.fetch_add(1);
        slot->m_references.store((std::numeric_limits<size_t>::max)() - 1);
    }
    else
    {
        destroy(slot, record);
    }
}

template <typename T>
void LockFreeMapPool<T>::collect(LockFreeMapPoolBlock<T>* block)
{
    if (m_pending.load() == 0)
    {
        return;
    }
    const size_t retired = (std::numeric_limits<size_t>::max)() - 1;
    for (size_t index = 0; index < block->m_count; ++index)
    {
        LockFreeMapPoolSlot<T>* slot = &block->m_slots[index];
        size_t expected = retired;
        if (slot->m_references.load() == retired &&
            slot->m_references.compare_exchange_strong(expected, retired + 1))
        {
            if (m_domain->protectsValue(slot))
            {
                slot->m_references.store(retired);
            }
            else
            {
                m_pending.fetch_sub(1);
                destroy(slot);
            }
        }
    }
}

template <typename T>
LockFreeMapStatus LockFreeMapPool<T>::maintain(size_t budget, size_t& released)
{
    released = 0;
    if (m_records == nullptr)
    {
        return LOCK_FREE_MAP_ALLOCATION_FAILURE;
    }
    if (!m_canShrink)
    {
        // 增长判定前归还空闲记录占用的原始槽；活动记录的私人步骤不等待也不夺取
        const size_t channel = m_deferred ? 1 : 0;
        for (size_t visit = 0; visit < budget; ++visit)
        {
            LockFreeMapPoolSlot<T>* cached = static_cast<LockFreeMapPoolSlot<T>*>(m_domain->takeIdleCache(this, channel));
            if (cached == nullptr)
            {
                break;
            }
            cached->m_references.store(0, std::memory_order_release);
        }
        if (m_pending.load() != 0)
        {
            const size_t span = m_span.load();
            const size_t visits = budget < span ? budget : span;
            for (size_t visit = 0; visit < visits; ++visit)
            {
                LockFreeMapPoolRecord<T>& record = m_records[m_cursor.fetch_add(1) % span];
                uint32_t expected = 2;
                if (record.m_state.compare_exchange_strong(expected, 1))
                {
                    collect(record.m_block.load());
                    record.m_state.store(2);
                }
            }
        }
        return LOCK_FREE_MAP_OK;
    }
    LockFreeMapShrinkContext context;
    context.initialCapacity = m_config.initialCapacity;
    context.successfulGrowCount = m_grows.load();
    context.currentCapacity = m_capacity.load();
    const size_t retained = m_inUse.load();
    const size_t openCapacity = m_openCapacity.load();
    context.reusableCapacity = openCapacity > retained ? openCapacity - retained : 0;
    const uint64_t lastGrow = m_lastGrow.load();
    const uint64_t time = now();
    context.hasSuccessfulGrow = context.successfulGrowCount != 0;
    context.millisecondsSinceLastGrow = lastGrow != 0 && time >= lastGrow ? time - lastGrow : 0;
    LockFreeMapShrinkDecision decision;
    LockFreeMapStatus status = LOCK_FREE_MAP_OK;
    try
    {
        if (m_config.shrink)
        {
            decision = m_config.shrink(context);
        }
    }
    catch (...)
    {
        status = LOCK_FREE_MAP_POLICY_EXCEPTION;
    }
    size_t remaining = decision.idleMilliseconds == 0 ? 0 : decision.shrinkCount;
    const size_t closed = (std::numeric_limits<size_t>::max)() / 2 + 1;
    const size_t span = m_span.load();
    const size_t visits = budget < span ? budget : span;
    for (size_t iteration = 0; iteration < visits; ++iteration)
    {
        const size_t index = m_cursor.fetch_add(1) % span;
        LockFreeMapPoolRecord<T>& record = m_records[index];
        uint32_t expected = 3;
        if (record.m_state.compare_exchange_strong(expected, 1))
        {
            LockFreeMapPoolBlock<T>* retired = record.m_retired;
            if (!m_domain->protects(retired))
            {
                const size_t count = retired->m_count;
                delete retired;
                record.m_retired = nullptr;
                m_bytes.fetch_sub(sizeof(LockFreeMapPoolBlock<T>) + count * sizeof(LockFreeMapPoolSlot<T>));
                m_capacity.fetch_sub(count);
                m_budget.fetch_sub(count);
                released += count;
                record.m_state.store(0);
            }
            else
            {
                record.m_state.store(3);
            }
            continue;
        }
        expected = 2;
        if (!record.m_state.compare_exchange_strong(expected, 1))
        {
            continue;
        }
        LockFreeMapPoolBlock<T>* block = record.m_block.load();
        collect(block);
        const uint64_t activity = block->m_activity.load();
        if (block->m_occupancy.load() != 0 || activity == (std::numeric_limits<uint64_t>::max)())
        {
            block->m_idleStart = 0;
            record.m_state.store(2);
            continue;
        }
        if (block->m_idleStart == 0 || block->m_idleActivity != activity)
        {
            block->m_idleActivity = activity;
            // 不能把空闲起点回填为策略回调之前的时间
            // 因为此线程暂停期间，其他操作可能使用并归还过该块
            block->m_idleStart = now();
        }
        if (remaining < block->m_count || time < block->m_idleStart ||
            time - block->m_idleStart < decision.idleMilliseconds)
        {
            record.m_state.store(2);
            continue;
        }
        size_t open = m_openCapacity.load();
        bool reserved = false;
        while (open >= m_config.initialCapacity && open - m_config.initialCapacity >= block->m_count)
        {
            if (m_openCapacity.compare_exchange_weak(open, open - block->m_count))
            {
                reserved = true;
                break;
            }
        }
        size_t zero = 0;
        if (!reserved || !block->m_occupancy.compare_exchange_strong(zero, closed))
        {
            if (reserved)
            {
                m_openCapacity.fetch_add(block->m_count);
            }
            record.m_state.store(2);
            continue;
        }
        if (block->m_activity.load() != activity)
        {
            block->m_occupancy.store(0);
            m_openCapacity.fetch_add(block->m_count);
            record.m_state.store(2);
            continue;
        }
        remaining -= block->m_count;
        record.m_retired = record.m_block.exchange(nullptr);
        record.m_state.store(3);
    }
    return status;
}

template <typename T>
size_t LockFreeMapPool<T>::used() const
{
    if (m_canShrink)
    {
        return m_inUse.load();
    }
    size_t result = 0;
    const size_t span = m_span.load();
    for (size_t index = 0; index < span; ++index)
    {
        LockFreeMapPoolBlock<T>* block = m_records[index].m_block.load();
        if (block != nullptr)
        {
            for (size_t slot = 0; slot < block->m_count; ++slot)
            {
                const size_t references = block->m_slots[slot].m_references.load();
                result += references != 0 ? 1 : 0;
            }
        }
    }
    return result;
}

template <typename T>
LockFreeMapPoolStats LockFreeMapPool<T>::stats() const
{
    LockFreeMapPoolStats result;
    result.capacity = m_capacity.load();
    const size_t budget = m_budget.load();
    result.reserved = budget > result.capacity ? budget - result.capacity : 0;
    result.retained = m_retained.load();
    const size_t open = m_openCapacity.load();
    size_t occupied = m_inUse.load();
    if (!m_canShrink)
    {
        result.retained = 0;
        occupied = 0;
        const size_t span = m_span.load();
        for (size_t index = 0; index < span; ++index)
        {
            LockFreeMapPoolBlock<T>* block = m_records[index].m_block.load();
            if (block != nullptr)
            {
                for (size_t slot = 0; slot < block->m_count; ++slot)
                {
                    const size_t references = block->m_slots[slot].m_references.load();
                    if (references != 0)
                    {
                        ++occupied;
                        if (references != (std::numeric_limits<size_t>::max)() - 2 &&
                            references != (std::numeric_limits<size_t>::max)() - 3)
                        {
                            ++result.retained;
                        }
                    }
                }
            }
        }
    }
    result.reusable = open > occupied ? open - occupied : 0;
    result.bytes = m_bytes.load();
    result.successfulGrowCount = m_grows.load();
    result.systemAllocations = m_allocations.load();
    return result;
}

template <typename T>
size_t LockFreeMapPool<T>::metadataBytes() const
{
    return m_records == nullptr ? 0 : m_config.blockRecordLimit * sizeof(LockFreeMapPoolRecord<T>);
}

template <typename T>
void LockFreeMapPool<T>::freeMemory()
{
    m_domain->clearPoolCaches(this);
    if (m_records == nullptr)
    {
        return;
    }
    for (size_t index = 0; index < m_config.blockRecordLimit; ++index)
    {
        LockFreeMapPoolRecord<T>& record = m_records[index];
        LockFreeMapPoolBlock<T>* block = record.m_block.exchange(nullptr);
        if (block != nullptr)
        {
            for (size_t slot = 0; slot < block->m_count; ++slot)
            {
                const size_t references = block->m_slots[slot].m_references.load();
                if (references != 0 && references != (std::numeric_limits<size_t>::max)() - 3)
                {
                    block->m_slots[slot].mutableValue().~T();
                }
            }
        }
        delete block;
        delete record.m_retired;
        record.m_retired = nullptr;
        record.m_state.store(0);
    }
    m_budget.store(0);
    m_capacity.store(0);
    m_openCapacity.store(0);
    m_retained.store(0);
    m_inUse.store(0);
    m_bytes.store(0);
    m_pending.store(0);
    m_span.store(0);
    m_hint.store(0);
}

#endif