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
    const uint32_t bit = cached & (uint32_t(0) - cached);
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
    // At most sixteen records retain batches, with at most 31 unused nodes each.
    // Scale small-pool batches by observed participants instead of always dividing
    // by 64: one producer otherwise pays the complete shared claim path per item.
    const size_t index = static_cast<size_t>(&record - m_records);
    const size_t participants = (std::max)(size_t(1), (std::min)(size_t(16), m_high.load(std::memory_order_relaxed)));
    size_t batch = index >= 16
                       ? 1
                       : (std::max)(size_t(1),
                             (std::min)(size_t(32), m_capacity.load(std::memory_order_relaxed) / (participants * 4)));
    if (batch > 1 && m_config.growThreshold != 0)
    {
        // Do not preclaim across the positive threshold. Leave threshold+1
        // reusable nodes until this cache is consumed, then the next single-node
        // claim evaluates the threshold at its original serial trigger point.
        const size_t free = available();
        const size_t margin = free > m_config.growThreshold ? free - m_config.growThreshold : 0;
        batch = margin > 1 ? (std::min)(batch, margin - 1) : 1;
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
                return nullptr;
            }
            if (block != nullptr && !block->m_initial)
            {
                increment(block->m_activity);
            }
            return block;
        }
        // A changed head proves another operation progressed; no owner is awaited.
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
        block = protectedNext(block, record, interrupted);
    }
    return nullptr;
}

template <class T>
void LockFreeQueueArena<T>::releaseNode(Node* node, Record* producer)
{
    Block* block = node->block();
    if (producer != nullptr)
    {
        producer->m_block.store(block);
    }
    const size_t index = static_cast<size_t>(node - block->m_nodes);
    node->reset();
    if (producer == nullptr)
    {
        m_returned.store(m_returned.load(std::memory_order_relaxed) + 1, std::memory_order_release);
        block->m_returnedNodes.store(
            block->m_returnedNodes.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }
    else
    {
        m_availableBase.fetch_add(1, std::memory_order_relaxed);
        block->m_freeNodes.fetch_add(1, std::memory_order_release);
    }
    // Return LP. No payload or block access follows publication of the last bit.
    block->m_nodeBits[index / 32].fetch_or(uint32_t(1) << (index % 32), std::memory_order_release);
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
    const size_t count = bitCount(bits);
    record.m_block.store(block);
    record.m_cachedMask.store(0, std::memory_order_relaxed);
    record.m_cachedBlock = nullptr;
    m_availableBase.fetch_add(count, std::memory_order_relaxed);
    block->m_freeNodes.fetch_add(count, std::memory_order_release);
    block->m_nodeBits[word].fetch_or(bits, std::memory_order_release);
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
bool LockFreeQueueArena<T>::hasCachedClaims() const
{
    const size_t count = (std::min)(size_t(16), m_high.load());
    for (size_t i = 0; i < count; ++i)
    {
        if (m_records[i].m_cachedMask.load(std::memory_order_relaxed) != 0)
        {
            return true;
        }
    }
    return false;
}

template <class T>
LockFreeQueueResult LockFreeQueueArena<T>::grow()
{
    const size_t before = m_budget.load();
    // Preserve the public custom-policy outcome: declined/failed calculation
    // precedes budget truncation. The default has no callback to evaluate.
    if (before >= m_limit && !m_growPolicy)
    {
        return before > m_capacity.load() || m_retiringCapacity.load() != 0 || hasCachedClaims()
                   ? LockFreeQueueTemporaryUnavailable
                   : LockFreeQueueCapacityLimit;
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
            return budget > m_capacity.load() || m_retiringCapacity.load() != 0 || hasCachedClaims()
                       ? LockFreeQueueTemporaryUnavailable
                       : LockFreeQueueCapacityLimit;
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
    }
    size_t peak = m_peak.load(std::memory_order_relaxed);
    const size_t reserved = budget + amount;
    while (peak < reserved && !m_peak.compare_exchange_weak(peak, reserved, std::memory_order_relaxed))
    {

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
    do
    {
        block->m_next.store(head, std::memory_order_relaxed);
    } while (!m_blocks.compare_exchange_weak(head, block));
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
    node = findNode(record, interrupted);
    if (node == nullptr && !interrupted && reclaimCaches(record))
    {
        node = findNode(record, interrupted);
    }
    if (node == nullptr)
    {
        if (interrupted)
        {
            result = LockFreeQueueTemporaryUnavailable;
            return nullptr;
        }
        if (available() != 0)
        {
            result = LockFreeQueueTemporaryUnavailable;
            return nullptr;
        }
        result = grow();
        // The traversal may have preceded another grower's publication. Recheck
        // before reporting a limit: reserved budget becoming published capacity
        // is not evidence that the newly published block is already full.
        node = findNode(record, interrupted);
        if (node == nullptr)
        {
            if (result == LockFreeQueueSuccess || interrupted || available() != 0)
            {
                result = LockFreeQueueTemporaryUnavailable;
            }
            return nullptr;
        }
    }
    result = LockFreeQueueSuccess;
    if (m_config.growThreshold != 0 && node->block()->freeNodes() <= m_config.growThreshold)
    {
        const size_t current = m_capacity.load();
        const size_t reserved = m_budget.load();
        const size_t pending = reserved > current ? reserved - current : 0;
        if (available() <= m_config.growThreshold && pending <= m_config.growThreshold)
        {
            // Proactive failure never revokes the acquired node.
            grow();
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