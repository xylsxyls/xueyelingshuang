#ifndef LOCK_FREE_QUEUE_ARENA_ALLOCATION_INL
#define LOCK_FREE_QUEUE_ARENA_ALLOCATION_INL

template <class T>
typename LockFreeQueueArena<T>::Node* LockFreeQueueArena<T>::takeCached(Record& record)
{
    const uint32_t cached = record.m_cachedMask.load(std::memory_order_relaxed);
    if (cached == 0)
    {
        return nullptr;
    }
    if (m_entryOnly)
    {
        record.m_cachedMask.store(cached - 1, std::memory_order_relaxed);
        return reinterpret_cast<Node*>(record.m_cachedBlock);
    }
    // Separate successive payloads in one private batch across cache lines.
    // The bitmap and physical node identities are unchanged.
    uint32_t selected = cached;
    if (sizeof(Node) < 64)
    {
        selected = cached & UINT32_C(0x88888888);
        if (selected == 0)
        {
            selected = cached & UINT32_C(0x44444444);
        }
        if (selected == 0)
        {
            selected = cached & UINT32_C(0x22222222);
        }
        if (selected == 0)
        {
            selected = cached & UINT32_C(0x11111111);
        }
    }
    const uint32_t bit = selected & (uint32_t(0) - selected);
    Node* node = &record.m_cachedBlock->m_nodes[(record.m_wordHint - 1) * 32 + lowBit(bit)];
    record.m_cachedMask.store(cached ^ bit, std::memory_order_relaxed);
    return node;
}

template <class T>
typename LockFreeQueueArena<T>::Node* LockFreeQueueArena<T>::takeBlock(Block* block, Record& record)
{
    if (block->m_state.load() != 1 || block->freeNodes() == 0)
    {
        return nullptr;
    }
    // 所有生产者都可批量领取；小池按实际登记规模限制缓存总量。
    // 连续相除避免参与者数乘四溢出；外部额度无需节点位图，可领取至多 128 个。
    const size_t participants = (std::max)(size_t(1), m_high.load(std::memory_order_relaxed));
    size_t batch = (std::max)(size_t(1),
        (std::min)(m_entryOnly ? size_t(128) : size_t(32), m_capacity.load(std::memory_order_relaxed) / participants / 4));
    if (batch > 1 && m_config.growThreshold != 0)
    {
        // Do not preclaim across the positive threshold. Leave threshold+1
        // reusable nodes until this cache is consumed, then the next single-node
        // claim evaluates the threshold at its original serial trigger point.
        const size_t free = available();
        const size_t margin = free > m_config.growThreshold ? free - m_config.growThreshold : 0;
        batch = margin > 1 ? (std::min)(batch, margin - 1) : 1;
    }
    if (m_entryOnly)
    {
        const uint64_t capacity = block->m_capacity - (block->m_initial ? 1 : 0);
        uint64_t claimed = block->m_externalClaims.load(std::memory_order_acquire);
        for (;;)
        {
            // 两种归还只增加且合计不超过累计领取；先读领取，再读归还。
            // 领取 CAS 验证本次快照；失败只因其他生产者已推进，不等待归还者。
            const uint64_t returned = block->m_externalReturns.load(std::memory_order_acquire) +
                block->m_externalRefunds.load(std::memory_order_acquire);
            if (returned > claimed)
            {
                if (!record.m_progress.retry())
                {
                    return nullptr;
                }
                claimed = block->m_externalClaims.load(std::memory_order_acquire);
                continue;
            }
            const uint64_t used = claimed - returned;
            if (used >= capacity)
            {
                return nullptr;
            }
            const size_t count = static_cast<size_t>((std::min)(static_cast<uint64_t>(batch), capacity - used));
            if (claimed > UINT64_MAX - count)
            {
                record.m_creditExhausted = true;
                return nullptr;
            }
            if (block->m_externalClaims.compare_exchange_weak(claimed, claimed + count,
                std::memory_order_acquire, std::memory_order_relaxed))
            {
                m_availableBase.fetch_sub(count, std::memory_order_relaxed);
                block->m_freeNodes.fetch_sub(count, std::memory_order_relaxed);
                if (block->m_state.load() != 1)
                {
                    m_availableBase.fetch_add(count, std::memory_order_relaxed);
                    block->m_freeNodes.fetch_add(count, std::memory_order_release);
                    block->m_externalRefunds.fetch_add(count, std::memory_order_release);
                    return nullptr;
                }
                record.m_cachedBlock = block;
                record.m_cachedMask.store(static_cast<uint32_t>(count), std::memory_order_relaxed);
                return takeCached(record);
            }
            if (!record.m_progress.retry())
            {
                return nullptr;
            }
        }
    }
    size_t word = record.m_wordHint % block->m_nodeWords;
    for (size_t visited = 0; visited < block->m_nodeWords; ++visited)
    {
        uint32_t bits = block->m_nodeBits[word].load(std::memory_order_relaxed);
        while (bits != 0)
        {
            const uint32_t low = bits & (uint32_t(0) - bits);
            const uint32_t window = batch == 32 ? UINT32_MAX : (uint32_t(0) - low) ^ ((uint32_t(0) - low) << batch);
            const uint32_t taken = bits & window;
            if (block->m_nodeBits[word].compare_exchange_weak(
                    bits, bits ^ taken, std::memory_order_acquire, std::memory_order_relaxed))
            {
                const size_t count = bitCount(taken);
                m_availableBase.fetch_sub(count, std::memory_order_relaxed);
                block->m_freeNodes.fetch_sub(count, std::memory_order_relaxed);
                // A closer first publishes CLOSED, then scans these hazards.
                // No node claim commits after its close validation fails.
                if (block->m_state.load() != 1)
                {
                    m_availableBase.fetch_add(count, std::memory_order_relaxed);
                    block->m_freeNodes.fetch_add(count, std::memory_order_release);
                    block->m_nodeBits[word].fetch_or(taken, std::memory_order_release);
                    return nullptr;
                }
                // Construct all exclusively claimed Nodes, including the private
                // cache, so quiescent destruction can use the allocation bitmap
                // without touching unconstructed free storage. No T is built here.
                uint32_t initialize = taken;
                while (initialize != 0)
                {
                    const uint32_t bit = initialize & (uint32_t(0) - initialize);
                    new (&block->m_nodes[word * 32 + lowBit(bit)]) Node(block);
                    initialize ^= bit;
                }
                record.m_cachedBlock = block;
                record.m_cachedMask.store(taken, std::memory_order_relaxed);
                record.m_wordHint = word + 1;
                return takeCached(record);
            }
            if (!record.m_progress.retry())
            {
                return nullptr;
            }
        }
        word = word + 1 == block->m_nodeWords ? 0 : word + 1;
    }
    return nullptr;
}

template <class T>
typename LockFreeQueueArena<T>::Block* LockFreeQueueArena<T>::protectedFirst(Record& record, bool& interrupted)
{
    for (;;)
    {
        Block* block = m_blocks.load();
        record.m_block.store(block);
        if (m_blocks.load() == block)
        {
            if (block != nullptr && block->m_state.load() == 3)
            {
                interrupted = true;
                record.m_progress.interrupt();
                return nullptr;
            }
            if (block != nullptr && !block->m_initial)
            {
                increment(block->m_activity);
            }
            return block;
        }
        // A changed head proves another operation progressed; no owner is awaited.
        if (!record.m_progress.retry())
        {
            interrupted = true;
            return nullptr;
        }
    }
}

template <class T>
typename LockFreeQueueArena<T>::Block* LockFreeQueueArena<T>::protectedNext(
    Block* block, Record& record, bool& interrupted)
{
    Block* next = block->m_next.load();
    LockFreeQueueAtomic<Block*>& traversal = m_traversal[&record - m_records];
    traversal.store(next);
    // A detached predecessor cannot validate its obsolete next pointer.
    if (block->m_state.load() == 3 || block->m_next.load() != next)
    {
        traversal.store(nullptr, std::memory_order_release);
        interrupted = true;
        record.m_progress.interrupt();
        return nullptr;
    }
    record.m_block.store(next);
    traversal.store(nullptr, std::memory_order_release);
    if (next != nullptr && !next->m_initial)
    {
        increment(next->m_activity);
    }
    return next;
}

template <class T>
typename LockFreeQueueArena<T>::Node* LockFreeQueueArena<T>::findNode(Record& record, bool& interrupted)
{
    Block* block = protectedFirst(record, interrupted);
    while (block != nullptr)
    {
        Node* node = takeBlock(block, record);
        if (node != nullptr)
        {
            return node;
        }
        if (record.m_progress.interrupted())
        {
            interrupted = true;
            return nullptr;
        }
        block = protectedNext(block, record, interrupted);
    }
    return nullptr;
}

template <class T>
void LockFreeQueueArena<T>::releaseNode(Node* node, Record* producer, Block* externalOwner)
{
    Block* block = externalOwner != nullptr ? externalOwner : creditOwner(node);
    if (producer != nullptr)
    {
        producer->m_block.store(block);
    }
    if (!m_entryOnly && externalOwner == nullptr)
    {
        node->reset();
    }
    if (producer == nullptr)
    {
        if (m_fixedEntryBlock == nullptr)
        {
            m_returned.store(m_returned.load(std::memory_order_relaxed) + 1, std::memory_order_release);
        }
        if (!m_entryOnly)
        {
            block->m_returnedNodes.store(
                block->m_returnedNodes.load(std::memory_order_relaxed) + 1, std::memory_order_release);
        }
    }
    else
    {
        m_availableBase.fetch_add(1, std::memory_order_relaxed);
        block->m_freeNodes.fetch_add(1, std::memory_order_release);
    }
    // Return LP. No payload or block access follows publication of the last bit.
    if (m_entryOnly)
    {
        if (producer == nullptr)
        {
            block->m_externalReturns.store(block->m_externalReturns.load(std::memory_order_relaxed) + 1,
                std::memory_order_release);
        }
        else
        {
            block->m_externalRefunds.fetch_add(1, std::memory_order_release);
        }
    }
    else
    {
        const size_t index = static_cast<size_t>(node - block->m_nodes);
        block->m_nodeBits[index / 32].fetch_or(uint32_t(1) << (index % 32), std::memory_order_release);
    }
}

template <class T>
void LockFreeQueueArena<T>::releaseNodes(Node* const* nodes, size_t count, Block* const* externalOwners)
{
    if (m_entryOnly)
    {
        if (m_fixedEntryBlock == nullptr)
        {
            m_returned.store(m_returned.load(std::memory_order_relaxed) + count, std::memory_order_release);
        }
        for (size_t i = 0; i < count;)
        {
            Block* block = reinterpret_cast<Block*>(nodes[i]);
            const size_t begin = i++;
            while (i < count && nodes[i] == nodes[begin])
            {
                ++i;
            }
            const size_t returned = i - begin;
            block->m_externalReturns.store(block->m_externalReturns.load(std::memory_order_relaxed) + returned,
                std::memory_order_release);
        }
        return;
    }
    if (count == 1)
    {
        releaseNode(nodes[0], nullptr, externalOwners != nullptr ? externalOwners[0] : nullptr);
        return;
    }
    Block* blocks[64];
    size_t words[64];
    uint32_t masks[64];
    // 本次至多 64 个节点，128 槽本地索引总有空位；不访问共享表、不申请内存。
    uint8_t positions[128] = {};
    size_t groups = 0;
    for (size_t i = 0; i < count; ++i)
    {
        Node* node = nodes[i];
        Block* externalOwner = externalOwners != nullptr ? externalOwners[i] : nullptr;
        Block* block = externalOwner != nullptr ? externalOwner : node->block();
        const size_t index = static_cast<size_t>(node - block->m_nodes);
        const size_t word = index / 32;
        size_t position = ((reinterpret_cast<uintptr_t>(block) >> 6) ^ word) & 127;
        size_t group = 0;
        while (positions[position] != 0)
        {
            group = positions[position] - 1;
            if (blocks[group] == block && words[group] == word)
            {
                break;
            }
            position = (position + 1) & 127;
        }
        if (positions[position] == 0)
        {
            group = groups++;
            positions[position] = static_cast<uint8_t>(group + 1);
            blocks[group] = block;
            words[group] = word;
            masks[group] = 0;
        }
        masks[group] |= uint32_t(1) << (index % 32);
        if (externalOwner == nullptr)
        {
            node->reset();
        }
    }
    m_returned.store(m_returned.load(std::memory_order_relaxed) + count, std::memory_order_release);
    for (size_t group = 0; group < groups; ++group)
    {
        Block* block = blocks[group];
        block->m_returnedNodes.store(block->m_returnedNodes.load(std::memory_order_relaxed) + bitCount(masks[group]), std::memory_order_release);
        block->m_nodeBits[words[group]].fetch_or(masks[group], std::memory_order_release);
    }
}

template <class T>
void LockFreeQueueArena<T>::releaseEntryCredits(Block& block, size_t count)
{
    // 固定 Entry 池只有一个不可退役块；其外部归还量同时就是全池消费量。
    // 可扩容池仍保留跨块累计量，块内空闲量直接使用外部归还计数。
    if (m_fixedEntryBlock == nullptr)
    {
        m_returned.store(m_returned.load(std::memory_order_relaxed) + count, std::memory_order_release);
    }
    block.m_externalReturns.store(block.m_externalReturns.load(std::memory_order_relaxed) + count, std::memory_order_release);
}

template <class T>
void LockFreeQueueArena<T>::releaseCache(Record& record)
{
    const uint32_t bits = record.m_cachedMask.load(std::memory_order_relaxed);
    if (bits == 0)
    {
        return;
    }
    Block* block = record.m_cachedBlock;
    const size_t word = record.m_wordHint - 1;
    const size_t count = m_entryOnly ? static_cast<size_t>(bits) : bitCount(bits);
    record.m_block.store(block);
    record.m_cachedMask.store(0, std::memory_order_relaxed);
    record.m_cachedBlock = nullptr;
    m_availableBase.fetch_add(count, std::memory_order_relaxed);
    block->m_freeNodes.fetch_add(count, std::memory_order_release);
    if (m_entryOnly)
    {
        block->m_externalRefunds.fetch_add(count, std::memory_order_release);
    }
    else
    {
        block->m_nodeBits[word].fetch_or(bits, std::memory_order_release);
    }
}

template <class T>
bool LockFreeQueueArena<T>::reclaimCaches(Record& caller)
{
    bool reclaimed = false;
    const size_t high = m_high.load();
    for (size_t i = 0; i < high; ++i)
    {
        Record& record = m_records[i];
        if (&record == &caller || record.m_cachedMask.load(std::memory_order_relaxed) == 0)
        {
            continue;
        }
        uint32_t expected = 0;
        if (record.m_active.compare_exchange_strong(expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            releaseCache(record);
            leave(record, true);
            reclaimed = true;
        }
    }
    return reclaimed;
}

template <class T>
LockFreeQueueResult LockFreeQueueArena<T>::unavailableResult() const
{
    // 已申请但尚未发布、私有缓存和待回收块都计入预算，不能当作 CAS 竞争。
    return m_budget.load() >= m_limit ? LockFreeQueueCapacityLimit : LockFreeQueueResourceUnavailable;
}

template <class T>
LockFreeQueueResult LockFreeQueueArena<T>::grow(Record* record)
{
    LockFreeQueueProgress committedProgress;
    const size_t before = m_budget.load();
    // Exhausted budget takes precedence; do not invoke a growth policy without room.
    if (before >= m_limit)
    {
        return LockFreeQueueCapacityLimit;
    }
    LockFreeQueueGrowContext context;
    context.initialCapacity = m_config.initialCapacity;
    context.successfulGrowCount = m_grows.load();
    context.currentCapacity = m_capacity.load();
    size_t amount = context.currentCapacity;
    try
    {
        if (m_growPolicy)
        {
            amount = m_growPolicy(context);
        }
    }
    catch (...)
    {
        return LockFreeQueuePolicyFailure;
    }
    if (amount == 0)
    {
        return LockFreeQueueGrowthDeclined;
    }
    size_t budget = m_budget.load();
    for (;;)
    {
        if (budget >= m_limit)
        {
            return LockFreeQueueCapacityLimit;
        }
        if (!m_growPolicy)
        {
            // Preserve proportional growth when no candidate is outstanding.
            // Competing default growers reserve only the uncovered demand, or
            // one initial-sized batch so a paused allocator cannot own progress.
            const size_t current = m_capacity.load();
            const size_t pending = budget > current ? budget - current : 0;
            const size_t minimum = (std::max)(m_config.initialCapacity, current / 8);
            amount = current > pending ? (std::max)(minimum, current - pending) : minimum;
        }
        const size_t granted = (std::min)(amount, m_limit - budget);
        if (m_budget.compare_exchange_weak(budget, budget + granted))
        {
            amount = granted;
            break;
        }
        if (record != nullptr && !record->m_progress.retry())
        {
            return LockFreeQueueBusy;
        }
        if (record == nullptr)
        {
            committedProgress.retry();
        }
    }
    size_t peak = m_peak.load(std::memory_order_relaxed);
    const size_t reserved = budget + amount;
    while (peak < reserved && !m_peak.compare_exchange_weak(peak, reserved, std::memory_order_relaxed))
    {
        committedProgress.retry();
    }
    // Recheck reusable resources after the reservation. Concurrent publishers
    // often satisfy this demand; avoid allocating redundant candidate blocks.
    if (available() > m_config.growThreshold)
    {
        m_budget.fetch_sub(amount);
        return LockFreeQueueSuccess;
    }
    Block* block = createBlock(amount, false);
    if (block == nullptr)
    {
        m_budget.fetch_sub(amount);
        return LockFreeQueueAllocationFailure;
    }
    if (available() > m_config.growThreshold)
    {
        deleteBlock(block, true);
        m_budget.fetch_sub(amount);
        return LockFreeQueueSuccess;
    }
    Block* head = m_blocks.load();
    for (;;)
    {
        block->m_next.store(head, std::memory_order_relaxed);
        if (m_blocks.compare_exchange_weak(head, block))
        {
            break;
        }
        if (record != nullptr && !record->m_progress.retry())
        {
            deleteBlock(block, true);
            m_budget.fetch_sub(amount);
            return LockFreeQueueBusy;
        }
        if (record == nullptr)
        {
            committedProgress.retry();
        }
    }
    // A published-but-not-open block remains charged and cannot be allocated or
    // shrunk. Other growers can reserve their own budget without waiting for it.
    m_capacity.fetch_add(amount);
    m_availableBase.fetch_add(amount, std::memory_order_relaxed);
    m_bytes.fetch_add(block->m_bytes);
    m_blockCount.fetch_add(1);
    increment(m_grows);
    const uint64_t time = now();
    uint64_t last = m_lastGrow.load(std::memory_order_relaxed);
    while (time > last && !m_lastGrow.compare_exchange_weak(last, time, std::memory_order_relaxed))
    {
        committedProgress.retry();
    }
    block->m_state.store(1);
    return LockFreeQueueSuccess;
}

template <class T>
typename LockFreeQueueArena<T>::Node* LockFreeQueueArena<T>::acquireNode(Record& record, LockFreeQueueResult& result)
{
    Node* node = takeCached(record);
    if (node != nullptr)
    {
        // These nodes were charged and the threshold evaluated at batch claim.
        // Using one changes no reusable capacity; do not poll shared counters or
        // repeatedly invoke a failed proactive policy for the same reservation.
        result = LockFreeQueueSuccess;
        return node;
    }
    bool interrupted = false;
    record.m_creditExhausted = false;
    node = findNode(record, interrupted);
    if (node == nullptr && !interrupted && reclaimCaches(record))
    {
        node = findNode(record, interrupted);
    }
    if (node == nullptr)
    {
        if (record.m_creditExhausted)
        {
            result = LockFreeQueueSequenceExhausted;
            return nullptr;
        }
        if (interrupted)
        {
            record.m_progress.interrupt();
            result = LockFreeQueueBusy;
            return nullptr;
        }
        if (available() != 0)
        {
            result = unavailableResult();
            return nullptr;
        }
        result = grow(&record);
        if (record.m_progress.interrupted())
        {
            return nullptr;
        }
        // The traversal may have preceded another grower's publication. Recheck
        // before reporting a limit: reserved budget becoming published capacity
        // is not evidence that the newly published block is already full.
        node = findNode(record, interrupted);
        if (node == nullptr)
        {
            if (result == LockFreeQueueSuccess || interrupted || available() != 0)
            {
                result = interrupted ? LockFreeQueueBusy : unavailableResult();
            }
            if (interrupted)
            {
                record.m_progress.interrupt();
            }
            return nullptr;
        }
    }
    result = LockFreeQueueSuccess;
    if (m_config.growThreshold != 0 && creditOwner(node)->freeNodes() <= m_config.growThreshold)
    {
        const size_t current = m_capacity.load();
        const size_t reserved = m_budget.load();
        const size_t pending = reserved > current ? reserved - current : 0;
        if (available() <= m_config.growThreshold && pending <= m_config.growThreshold)
        {
            // Proactive failure never revokes the acquired node.
            grow(&record);
            // 提前扩容竞争失败不能撤销本次已经取得的节点。
            record.m_progress.resume();
        }
    }
    return node;
}

template <class T>
uint64_t LockFreeQueueArena<T>::now()
{
#if defined(_MSC_VER) && _MSC_VER < 1900
    LARGE_INTEGER counter, frequency;
    if (!QueryPerformanceCounter(&counter) || !QueryPerformanceFrequency(&frequency) || counter.QuadPart < 0 ||
        frequency.QuadPart <= 0)
    {
        return 0;
    }
    const uint64_t seconds = static_cast<uint64_t>(counter.QuadPart / frequency.QuadPart);
    if (seconds > UINT64_MAX / 1000)
    {
        return 0;
    }
    const uint64_t fraction = static_cast<uint64_t>(counter.QuadPart % frequency.QuadPart);
    const uint64_t actual =
        seconds * 1000 + static_cast<uint64_t>(static_cast<double>(fraction) * 1000.0 / frequency.QuadPart);
#else
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    const uint64_t actual = elapsed > 0 ? static_cast<uint64_t>(elapsed) : 0;
#endif
    return actual;
}

#endif