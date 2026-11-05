#ifndef LOCK_FREE_QUEUE_CHANNEL_INL
#define LOCK_FREE_QUEUE_CHANNEL_INL

template <class Node, class SegmentPool, class RecordType>
LockFreeQueueChannel<Node, SegmentPool, RecordType>::LockFreeQueueChannel(Segment* first) :
m_head(first), m_headIndex(0), m_pendingSeal(nullptr), m_tail(first)
{

}

template <class Node, class SegmentPool, class RecordType>
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::reset(Segment* first)
{
    // External quiescence; the owning arena ends the previous generation.
    m_head = first;
    m_headIndex = 0;
    m_pendingSeal = nullptr;
    m_tail.store(first, std::memory_order_relaxed);
}

template <class Node, class SegmentPool, class RecordType>
typename LockFreeQueueChannel<Node, SegmentPool, RecordType>::Segment* LockFreeQueueChannel<Node, SegmentPool,
    RecordType>::protect(Record& record)
{
    for (;;)
    {
        Segment* segment = m_tail.load();
        const bool changed = record.m_hazard.load(std::memory_order_relaxed) != segment;
        if (changed)
        {
            record.m_hazard.store(segment);
        }
        // SC publication/validation pairs with detach and the pool's SC scan.
        // A recycled address is safe too: no old hint survives a cleared hazard.
        if (m_tail.load() == segment)
        {
            if (record.m_generation != segment->m_generation)
            {
                record.m_hint = segment->m_tail.load(std::memory_order_acquire);
                record.m_generation = segment->m_generation;
            }
            return segment;
        }
        const uint64_t retries = record.m_retries.load(std::memory_order_relaxed);
        record.m_retries.store(retries + (retries != UINT64_MAX ? 1 : 0), std::memory_order_relaxed);
    }
}

template <class Node, class SegmentPool, class RecordType>
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::help(Segment* segment, size_t position)
{
    size_t shared = segment->m_tail.load(std::memory_order_relaxed);
    if (shared < position)
    {
        // Only proved occupied prefixes move this auxiliary hint forward.
        segment->m_tail.compare_exchange_strong(shared, position, std::memory_order_release, std::memory_order_relaxed);
    }
}

template <class Node, class SegmentPool, class RecordType>
bool LockFreeQueueChannel<Node, SegmentPool, RecordType>::publish(Node* node, Record& record, SegmentPool& pool)
{
    for (;;)
    {
        Segment* segment = protect(record);
        size_t position = record.m_hint;
        Segment* next = segment->m_next.load(std::memory_order_acquire);
        if (next != nullptr)
        {
            // A consumer may request a partial close to release an idle grown
            // block. Any producer can publish the marker; no closer is awaited.
            close(segment, position);
            m_tail.compare_exchange_strong(segment, next);
            continue;
        }
        while (position < Segment::kCellCount)
        {
            LockFreeQueueAtomic<uintptr_t>& cell = segment->m_cells[Segment::mapped(position)];
            uintptr_t expected = cell.load(std::memory_order_acquire);
            if (expected == 1)
            {
                break;
            }
            if (expected == 0)
            {
                if (cell.compare_exchange_strong(expected, reinterpret_cast<uintptr_t>(node), std::memory_order_acq_rel,
                        std::memory_order_acquire))
                {
                    // Enqueue LP. All earlier positions are published; no private
                    // reserved index precedes this CAS. The consumer may now free
                    // the payload; nothing below dereferences node.
                    record.m_hint = position + 1;
                    if (((position + 1) & 15) == 0)
                    {
                        help(segment, position + 1);
                    }
                    return true;
                }
                if (expected == 1)
                {
                    break;
                }
                const uint64_t retries = record.m_retries.load(std::memory_order_relaxed);
                record.m_retries.store(retries + (retries != UINT64_MAX ? 1 : 0), std::memory_order_relaxed);
                // Bounded contention hints; no particular owner is awaited.
                for (size_t pause = 0; pause < kContentionPauses; ++pause)
                {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
                    _mm_pause();
#elif defined(__i386__) || defined(__x86_64__)
                    __asm__ __volatile__("pause");
#else
                    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
                }
            }
            ++position;
            position = (std::max)(position, segment->m_tail.load(std::memory_order_acquire));
        }
        help(segment, position);
        next = segment->m_next.load(std::memory_order_acquire);
        if (next == nullptr)
        {
            Segment* candidate = pool.acquire();
            if (candidate == nullptr)
            {
                // Failure is before this payload's LP; caller still owns node.
                return false;
            }
            if (segment->m_next.compare_exchange_strong(
                    next, candidate, std::memory_order_acq_rel, std::memory_order_acquire))
            {
                next = candidate;
            }
            else
            {
                pool.release(candidate);
            }
        }
        // A complete old segment precedes its successor. Anyone can advance the
        // hint, including the consumer. Merely publishing next commits no item.
        m_tail.compare_exchange_strong(segment, next);
    }
}

template <class Node, class SegmentPool, class RecordType>
Node* LockFreeQueueChannel<Node, SegmentPool, RecordType>::front(SegmentPool& pool)
{
    // The sole consumer owns head. Lifecycle changes require quiescence, so
    // this pointer also identifies an uninitialized channel without pool loads.
    if (m_head == nullptr)
    {
        return nullptr;
    }
    uintptr_t bits = m_headIndex == Segment::kCellCount
                         ? 1
                         : m_head->m_cells[Segment::mapped(m_headIndex)].load(std::memory_order_acquire);
    if (bits == 1)
    {
        return advance(pool);
    }
    return reinterpret_cast<Node*>(bits);
}

template <class Node, class SegmentPool, class RecordType>
Node* LockFreeQueueChannel<Node, SegmentPool, RecordType>::advance(SegmentPool& pool)
{
    Segment* next = m_head->m_next.load(std::memory_order_acquire);
    if (next == nullptr)
    {
        // Every preceding item was consumed, and no successor is linked.
        return nullptr;
    }
    Segment* previous = m_head;
    Segment* expected = previous;
    m_tail.compare_exchange_strong(expected, next);
    m_head = next;
    m_headIndex = 0;
    if (m_pendingSeal == previous)
    {
        m_pendingSeal = nullptr;
    }
    pool.retire(previous);
    const uintptr_t bits = m_head->m_cells[Segment::mapped(0)].load(std::memory_order_acquire);
    // At most one local segment transition per call, independent of backlog.
    return reinterpret_cast<Node*>(bits);
}

template <class Node, class SegmentPool, class RecordType>
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::close(Segment* segment, size_t position)
{
    for (; position < Segment::kCellCount; ++position)
    {
        LockFreeQueueAtomic<uintptr_t>& cell = segment->m_cells[Segment::mapped(position)];
        uintptr_t expected = cell.load(std::memory_order_acquire);
        if (expected == 1 || (expected == 0 && cell.compare_exchange_strong(
                                                   expected, 1, std::memory_order_acq_rel, std::memory_order_acquire)))
        {
            return;
        }
        if (expected == 1)
        {
            return;
        }
    }
}

template <class Node, class SegmentPool, class RecordType>
bool LockFreeQueueChannel<Node, SegmentPool, RecordType>::seal(SegmentPool& pool)
{
    // Sole consumer/maintainer. Normalize one boundary and require an observed
    // empty head. Seal THAT head, even if a concurrent producer has since moved
    // tail onward. Together with the single pending seal, this prevents empty
    // marker chains, including repeated maintenance while producers race.
    if (front(pool) != nullptr || m_pendingSeal != nullptr)
    {
        return false;
    }
    Segment* segment = m_head;
    Segment* next = segment->m_next.load(std::memory_order_acquire);
    if (next == nullptr)
    {
        Segment* candidate = pool.acquireInitial();
        if (candidate == nullptr)
        {
            return false;
        }
        if (segment->m_next.compare_exchange_strong(
                next, candidate, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            next = candidate;
        }
        else
        {
            pool.release(candidate);
        }
    }
    m_pendingSeal = segment;
    close(segment, segment->m_tail.load(std::memory_order_acquire));
    m_tail.compare_exchange_strong(segment, next);
    return true;
}

template <class Node, class SegmentPool, class RecordType>
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::consume()
{
    // Sole consumer invokes this only after front succeeded and assignment
    // completed. No cell is cleared until the entire detached segment is safe.
    ++m_headIndex;
}

template <class Node, class SegmentPool, class RecordType>
typename LockFreeQueueChannel<Node, SegmentPool, RecordType>::Segment* LockFreeQueueChannel<Node, SegmentPool,
    RecordType>::consumerSegment() const
{
    return m_head;
}

#endif