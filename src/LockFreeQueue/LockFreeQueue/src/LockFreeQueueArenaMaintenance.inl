#ifndef LOCK_FREE_QUEUE_ARENA_MAINTENANCE_INL
#define LOCK_FREE_QUEUE_ARENA_MAINTENANCE_INL

template <class T>
typename LockFreeQueueArena<T>::Segment* LockFreeQueueArena<T>::acquireSegment(
    Record& record, bool initialOnly, LockFreeQueueResult* result)
{
    if (result != nullptr)
    {
        *result = LockFreeQueueCapacityLimit;
    }
    bool interrupted = false;
    Block* block = initialOnly ? m_initial : protectedFirst(record, interrupted);
    while (block != nullptr)
    {
        if (block->m_state.load() == 1 && block->m_freeSegments.load(std::memory_order_relaxed) != 0)
        {
            for (size_t word = 0; word < block->m_segmentWords; ++word)
            {
                uint32_t bits = block->m_segmentBits[word].load(std::memory_order_relaxed);
                while (bits != 0)
                {
                    const uint32_t bit = bits & (uint32_t(0) - bits);
                    if (block->m_segmentBits[word].compare_exchange_weak(
                            bits, bits ^ bit, std::memory_order_acquire, std::memory_order_relaxed))
                    {
                        block->m_freeSegments.fetch_sub(1, std::memory_order_relaxed);
                        if (block->m_state.load() != 1)
                        {
                            block->m_freeSegments.fetch_add(1, std::memory_order_release);
                            block->m_segmentBits[word].fetch_or(bit, std::memory_order_release);
                            if (result != nullptr)
                            {
                                *result = unavailableResult();
                            }
                            return nullptr;
                        }
                        const size_t index = word * 32 + lowBit(bit);
                        Segment* segment = new (&block->m_segments[index]) Segment();
                        segment->m_poolIndex = index;
                        segment->m_owner = block;
                        if (!assignGeneration(segment, result, &record))
                        {
                            releaseSegment(segment, initialOnly ? nullptr : &record);
                            return nullptr;
                        }
                        return segment;
                    }
                    if (!record.m_progress.retry())
                    {
                        return nullptr;
                    }
                }
            }
        }
        if (initialOnly)
        {
            break;
        }
        block = protectedNext(block, record, interrupted);
    }
    return initialOnly || interrupted || record.m_progress.interrupted() ? nullptr : acquireAuxiliary(record, result);
}

template <class T>
void LockFreeQueueArena<T>::releaseSegment(Segment* segment, Record* producer)
{
    Block* block = static_cast<Block*>(segment->m_owner);
    if (block == nullptr)
    {
        Auxiliary& slot = m_auxiliary[segment->m_poolIndex];
        segment->m_idleSince = 0;
        slot.m_bits.store(reinterpret_cast<uintptr_t>(segment), std::memory_order_release);
        return;
    }
    if (producer != nullptr)
    {
        producer->m_block.store(block);
    }
    const size_t index = segment->m_poolIndex;
    block->m_freeSegments.fetch_add(1, std::memory_order_release);
    block->m_segmentBits[index / 32].fetch_or(uint32_t(1) << (index % 32), std::memory_order_release);
}

template <class T>
void LockFreeQueueArena<T>::retireSegment(Segment* segment)
{
    segment->m_retiredNext = nullptr;
    if (m_retiredSegmentTail != nullptr)
    {
        m_retiredSegmentTail->m_retiredNext = segment;
    }
    else
    {
        m_retiredSegments = segment;
    }
    m_retiredSegmentTail = segment;
}

template <class T>
bool LockFreeQueueArena<T>::protectedSegment(Segment* segment) const
{
    const size_t high = m_high.load();
    for (size_t i = 0; i < high; ++i)
    {
        if (m_records[i].m_hazard.load() == segment)
        {
            // A completed ordinary call may retain its hint protection. Claim
            // an INACTIVE record before clearing it; active/attached owners are
            // never modified or awaited. This amortizes producer hazard stores.
            uint32_t inactive = 0;
            if (!m_records[i].m_active.compare_exchange_strong(
                    inactive, 1, std::memory_order_acquire, std::memory_order_relaxed))
            {
                return true;
            }
            m_records[i].m_hazard.store(nullptr, std::memory_order_release);
            m_records[i].m_active.store(0, std::memory_order_release);
        }
    }
    return false;
}

template <class T>
bool LockFreeQueueArena<T>::protectedBlock(Block* block) const
{
    const size_t high = m_high.load();
    for (size_t i = 0; i < high; ++i)
    {
        // Traversal protection is transferred secondary -> primary. Scan in
        // that same order: if the secondary was already cleared, the SC primary
        // store precedes our primary load. Reversing these loads can miss both.
        const bool secondary = m_traversal[i].load() == block;
        if (secondary || m_records[i].m_block.load() == block)
        {
            return true;
        }
    }
    // A segment hazard pins its claimed segment bit until collectSegments.
    // Private payload/cache bits likewise keep this block nonempty.
    return false;
}

template <class T>
bool LockFreeQueueArena<T>::completelyFree(Block* block) const
{
    // This is a candidate test. Physical release ALSO requires closed+detached
    // state and the SC hazard scan. Producer returns publish their hazards before
    // the release increments observed here; their final bitmap stores are safe.
    return block->freeNodes() == block->m_capacity &&
           block->m_freeSegments.load(std::memory_order_acquire) == block->m_segmentCount;
}

template <class T>
size_t LockFreeQueueArena<T>::collectSegments(size_t budget)
{
    size_t reclaimed = 0;
    while (budget-- != 0 && m_retiredSegments != nullptr)
    {
        Segment* segment = m_retiredSegments;
        m_retiredSegments = segment->m_retiredNext;
        if (m_retiredSegments == nullptr)
        {
            m_retiredSegmentTail = nullptr;
        }
        if (protectedSegment(segment))
        {
            retireSegment(segment);
        }
        else
        {
            releaseSegment(segment);
            ++reclaimed;
        }
    }
    return reclaimed;
}

template <class T>
bool LockFreeQueueArena<T>::hasRetiredSegments() const
{
    return m_retiredSegments != nullptr;
}

template <class T>
LockFreeQueueMaintenanceResult LockFreeQueueArena<T>::maintain(size_t budget)
{
    LockFreeQueueMaintenanceResult result = {LockFreeQueueSuccess, 0, 0};
    if (budget == 0 || !initialized())
    {
        return result;
    }
    const uint64_t time = m_shrinkPolicy ? now() : 0;
    LockFreeQueueShrinkDecision decision = {0, 0};
    size_t allowance = 0;
    if (m_shrinkPolicy)
    {
        if (time == 0)
        {
            result.result = LockFreeQueueClockUnavailable;
        }
        else
        {
            LockFreeQueueShrinkContext context;
            context.initialCapacity = m_config.initialCapacity;
            context.currentCapacity = m_capacity.load();
            context.successfulGrowCount = m_grows.load();
            context.reusableFreeCapacity = available();
            const uint64_t last = m_lastGrow.load();
            context.hasLastSuccessfulGrow = last != 0;
            context.millisecondsSinceLastSuccessfulGrow = time >= last ? time - last : 0;
            try
            {
                decision = m_shrinkPolicy(context);
            }
            catch (...)
            {
                result.result = LockFreeQueuePolicyFailure;
            }
            const size_t retired = m_retiringCapacity.load();
            const size_t removable = context.currentCapacity > m_config.initialCapacity + retired
                                         ? context.currentCapacity - m_config.initialCapacity - retired
                                         : 0;
            if (decision.idleMilliseconds != 0)
            {
                allowance = (std::min)(decision.shrinkCount, removable);
            }
        }
    }
    // A budget is a maximum, not a request to repeat idle scans. Snapshot only
    // visit counts: concurrent arrivals remain eligible on the next call. The
    // sole consumer owns both retired lists and all persistent scan cursors.
    Segment* segmentEnd = m_retiredSegmentTail;
    const size_t blocks = m_blockCount.load();
    size_t remaining[5] = {segmentEnd != nullptr ? budget : 0, m_high.load(), blocks, blocks,
        decision.shrinkCount != 0 && decision.idleMilliseconds != 0 && time != 0 ? m_auxiliaryCount : 0};
    while (budget-- != 0 && (remaining[0] | remaining[1] | remaining[2] | remaining[3] | remaining[4]) != 0)
    {
        // Round-robin categories persist across calls: even budget=1 eventually
        // reaches cache, segment and block work. No full pool scan is performed.
        const size_t phase = m_maintenanceTurn++ % 5;
        if (remaining[phase] == 0)
        {
            continue;
        }
        --remaining[phase];
        if (phase == 0)
        {
            // A protected segment rotates to the tail; do not poll it again in
            // this call. Empty pop and subsequent maintenance still retry it.
            if (m_retiredSegments == segmentEnd)
            {
                remaining[0] = 0;
            }
            collectSegments(1);
            continue;
        }
        if (phase == 1)
        {
            const size_t high = m_high.load();
            if (high != 0)
            {
                Record& record = m_records[m_recordScan++ % high];
                uint32_t expected = 0;
                if (record.m_active.compare_exchange_strong(
                        expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
                {
                    const uint32_t cached = record.m_cachedMask.load(std::memory_order_relaxed);
                    result.reclaimedNodes += m_entryOnly ? static_cast<size_t>(cached) : bitCount(cached);
                    releaseCache(record);
                    leave(record, true);
                }
            }
            continue;
        }
        if (phase == 2)
        {
            // A pinned first block rotates to the end; it cannot starve older
            // safe blocks. This runs even when policy/clock evaluation failed.
            Block* block = m_retiredBlocks;
            if (block == nullptr)
            {
                remaining[2] = 0;
            }
            if (block != nullptr)
            {
                m_retiredBlocks = block->m_retiredNext;
                if (m_retiredBlocks == nullptr)
                {
                    m_retiredBlockTail = nullptr;
                }
                if (!protectedBlock(block) && completelyFree(block))
                {
                    const size_t amount = block->m_capacity;
                    const size_t bytes = block->m_bytes;
                    deleteBlock(block, true);
                    m_capacity.fetch_sub(amount);
                    m_budget.fetch_sub(amount);
                    m_retiringCapacity.fetch_sub(amount);
                    m_bytes.fetch_sub(bytes);
                    m_blockCount.fetch_sub(1);
                    result.releasedCapacity += amount;
                }
                else
                {
                    block->m_retiredNext = nullptr;
                    if (m_retiredBlockTail != nullptr)
                    {
                        m_retiredBlockTail->m_retiredNext = block;
                    }
                    else
                    {
                        m_retiredBlocks = block;
                    }
                    m_retiredBlockTail = block;
                }
            }
            continue;
        }
        if (phase == 4)
        {
            if (decision.shrinkCount != 0 && decision.idleMilliseconds != 0 && time != 0)
            {
                Auxiliary& slot = m_auxiliary[m_auxiliaryScan++ % m_auxiliaryCount];
                uintptr_t cached = slot.m_bits.load(std::memory_order_relaxed);
                if (cached > 1 && (cached & 1) == 0 &&
                    slot.m_bits.compare_exchange_strong(
                        cached, cached | 1, std::memory_order_acquire, std::memory_order_relaxed))
                {
                    Segment* segment = reinterpret_cast<Segment*>(cached);
                    if (segment->m_idleSince == 0 || time < segment->m_idleSince)
                    {
                        segment->m_idleSince = time;
                    }
                    else if (time - segment->m_idleSince >= decision.idleMilliseconds)
                    {
                        delete segment;
                        cached = 0;
                        m_bytes.fetch_sub(sizeof(Segment));
                    }
                    slot.m_bits.store(cached, std::memory_order_release);
                }
            }
            continue;
        }
        if (m_scan == nullptr)
        {
            m_scan = m_blocks.load();
            m_previous = nullptr;
        }
        Block* block = m_scan;
        if (block == nullptr)
        {
            continue;
        }
        Block* next = block->m_next.load();
        const uint64_t activity = block->m_activity.load();
        bool detached = false;
        if (!block->m_initial && block->m_state.load() == 1 && activity != UINT64_MAX && completelyFree(block) &&
            !protectedBlock(block))
        {
            const uint64_t observedTime = time != 0 ? time : now();
            if (observedTime == 0)
            {
                block->m_idleObserved = false;
            }
            else if (!block->m_idleObserved || block->m_seenActivity != activity || observedTime < block->m_idleSince)
            {
                block->m_idleObserved = true;
                block->m_seenActivity = activity;
                block->m_idleSince = observedTime;
            }
            else if (allowance != 0 && observedTime - block->m_idleSince >= decision.idleMilliseconds &&
                     block->m_capacity <= allowance)
            {
                uint32_t open = 1;
                if (block->m_state.compare_exchange_strong(open, 2))
                {
                    if (!protectedBlock(block) && completelyFree(block) && block->m_activity.load() == activity)
                    {
                        Block* expected = block;
                        detached = m_previous == nullptr ? m_blocks.compare_exchange_strong(expected, next)
                                                         : m_previous->m_next.compare_exchange_strong(expected, next);
                    }
                    if (detached)
                    {
                        // The sole maintainer publishes DETACHED before touching
                        // any successor. A traversal can skip a closing-but-linked
                        // block; it cannot validate an obsolete detached chain.
                        block->m_state.store(3);
                        block->m_retiredNext = nullptr;
                        if (m_retiredBlockTail != nullptr)
                        {
                            m_retiredBlockTail->m_retiredNext = block;
                        }
                        else
                        {
                            m_retiredBlocks = block;
                        }
                        m_retiredBlockTail = block;
                        // Newly detached blocks get a reclaim opportunity even
                        // when the retired list was empty earlier in this call.
                        if (remaining[2] == 0)
                        {
                            remaining[2] = 1;
                        }
                        m_retiringCapacity.fetch_add(block->m_capacity);
                        m_availableBase.fetch_sub(block->m_capacity, std::memory_order_relaxed);
                        allowance -= block->m_capacity;
                    }
                    else
                    {
                        block->m_idleObserved = false;
                        block->m_state.store(1);
                    }
                }
            }
        }
        else
        {
            block->m_idleObserved = false;
        }
        m_scan = next;
        if (!detached)
        {
            m_previous = block;
        }
    }
    return result;
}

template <class T>
size_t LockFreeQueueArena<T>::available() const
{
    const size_t returned = consumed();
    const size_t count = m_availableBase.load(std::memory_order_acquire) + returned;
    const size_t capacity = m_capacity.load();
    // A concurrent mixed snapshot is never an admission gate. Clamp modular
    // underflow to zero rather than interpreting it as unlimited free storage.
    return count <= capacity ? count : 0;
}

template <class T>
LockFreeQueuePoolStatistics LockFreeQueueArena<T>::statistics() const
{
    LockFreeQueuePoolStatistics result = {};
    result.currentCapacity = m_capacity.load();
    result.retiringBlockCapacity = m_retiringCapacity.load();
    const size_t budget = m_budget.load();
    result.reservedCapacity = budget > result.currentCapacity ? budget - result.currentCapacity : 0;
    result.peakReservedCapacity = m_peak.load();
    result.retainedBytes = sizeof(LockFreeQueueArena) + m_bytes.load();
    result.protectionBytes = m_recordBytes;
    result.blockCount = m_blockCount.load();
    result.successfulGrowCount = m_grows.load();
    result.allocationCalls = m_allocations.load();
    if (!initialized())
    {
        return result;
    }
    result.reusableFreeCapacity = available();
    const size_t unavailable = (std::min)(result.currentCapacity, result.retiringBlockCapacity);
    const size_t free = (std::min)(result.currentCapacity - unavailable, result.reusableFreeCapacity);
    result.usedNodes = result.currentCapacity - unavailable - free;
    const size_t high = m_high.load();
    for (size_t i = 0; i < high; ++i)
    {
        if (m_records[i].m_active.load(std::memory_order_relaxed) != 0)
        {
            ++result.activeProducers;
        }
        const uint64_t retries = m_records[i].m_retries.load(std::memory_order_relaxed);
        result.casRetries = retries > UINT64_MAX - result.casRetries ? UINT64_MAX : result.casRetries + retries;
    }
    return result;
}

template <class T>
size_t LockFreeQueueArena<T>::published() const
{
    size_t count = 0;
    const size_t high = m_high.load(std::memory_order_acquire);
    for (size_t i = 0; i < high; ++i)
    {
        count += m_records[i].m_published.load(std::memory_order_acquire);
    }
    return count;
}

template <class T>
size_t LockFreeQueueArena<T>::consumed() const
{
    if (m_fixedEntryBlock != nullptr)
    {
        return static_cast<size_t>(m_fixedEntryBlock->m_externalReturns.load(std::memory_order_acquire));
    }
    return m_returned.load(std::memory_order_acquire);
}

#endif