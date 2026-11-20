#ifndef LOCK_FREE_QUEUE_CHANNEL_INL
#define LOCK_FREE_QUEUE_CHANNEL_INL

#include <algorithm>

template <class Node, class SegmentPool, class RecordType>
LockFreeQueueChannel<Node, SegmentPool, RecordType>::LockFreeQueueChannel(Segment* first) :
m_cancelableReservations(false),
m_reservedConsumerConcurrent(false),
m_head(first),
m_headIndex(0),
m_frontCell(first != nullptr ? &first->m_cells[0] : nullptr),
m_pendingSeal(nullptr),
m_consumerRemaining(4096),
m_consumerInterval(4096),
m_tail(first)
{

}

template <class Node, class SegmentPool, class RecordType>
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::reset(Segment* first, bool cancelableReservations)
{
    // External quiescence; the owning arena ends the previous generation.
    m_cancelableReservations = cancelableReservations;
    m_reservedConsumerConcurrent = false;
    m_head = first;
    m_headIndex = 0;
    m_frontCell = first != nullptr ? &first->m_cells[0] : nullptr;
    m_pendingSeal = nullptr;
    m_consumerRemaining = m_consumerInterval;
    m_tail.store(first, std::memory_order_relaxed);
}

template <class Node, class SegmentPool, class RecordType>
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::configureConsumerYieldInterval(uint32_t operations)
{
    m_consumerInterval = operations;
    m_consumerRemaining = operations;
}

template <class Node, class SegmentPool, class RecordType>
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::advanceConsumer(size_t count)
{
    if (count == 1)
    {
        if (--m_consumerRemaining == 0)
        {
            yieldConsumer();
        }
    }
    else if (count >= m_consumerRemaining)
    {
        yieldConsumer();
    }
    else
    {
        m_consumerRemaining -= static_cast<uint32_t>(count);
    }
}

template <class Node, class SegmentPool, class RecordType>
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::yieldConsumer()
{
    m_consumerRemaining = m_consumerInterval;
    std::this_thread::yield();
}

template <class Node, class SegmentPool, class RecordType>
size_t LockFreeQueueChannel<Node, SegmentPool, RecordType>::readyPrefix(Node** nodes, size_t maximum) const
{
    return readyPrefixAs(nodes, maximum, 0);
}

template <class Node, class SegmentPool, class RecordType>
template <class Output>
size_t LockFreeQueueChannel<Node, SegmentPool, RecordType>::readyPrefixAs(Output** nodes, size_t maximum, uintptr_t clearMask) const
{
    // 本函数不移动唯一消费者的游标；冻结本次段地址和起点，避免每个原子读取后重复加载。
    Segment* const head = m_head;
    const size_t begin = m_headIndex;
    if (head == nullptr)
    {
        return 0;
    }
    const size_t count = (std::min)(maximum, Segment::kCellCount - begin);
    size_t ready = 0;
    // 先处理至多三个未对齐位置，使后续四项不会跨越16列转置的列边界。
    while (ready < count && ((begin + ready) & 3) != 0)
    {
        const uintptr_t bits = head->m_cells[Segment::mapped(begin + ready)].load(std::memory_order_acquire);
        if (bits <= 1)
        {
            return ready;
        }
        if (nodes != nullptr)
        {
            nodes[ready] = reinterpret_cast<Output*>(bits & ~clearMask);
        }
        ++ready;
    }
    // 四个独立 acquire 读取重叠缓存访问；只输出首个未就绪位置之前的连续前缀。
    // 预读的是整个段生命周期内稳定的原子单元，不解引用尚未交接的载荷。
    for (; ready + 4 <= count; ready += 4)
    {
        // mapped采用16乘16转置；已对齐的四个逻辑位置对应固定16单元间距。
        const size_t mapped = Segment::mapped(begin + ready);
        const uintptr_t first = head->m_cells[mapped].load(std::memory_order_acquire);
        const uintptr_t second = head->m_cells[mapped + 16].load(std::memory_order_acquire);
        const uintptr_t third = head->m_cells[mapped + 32].load(std::memory_order_acquire);
        const uintptr_t fourth = head->m_cells[mapped + 48].load(std::memory_order_acquire);
        if (first <= 1)
        {
            return ready;
        }
        if (nodes != nullptr)
        {
            nodes[ready] = reinterpret_cast<Output*>(first & ~clearMask);
        }
        if (second <= 1)
        {
            return ready + 1;
        }
        if (nodes != nullptr)
        {
            nodes[ready + 1] = reinterpret_cast<Output*>(second & ~clearMask);
        }
        if (third <= 1)
        {
            return ready + 2;
        }
        if (nodes != nullptr)
        {
            nodes[ready + 2] = reinterpret_cast<Output*>(third & ~clearMask);
        }
        if (fourth <= 1)
        {
            return ready + 3;
        }
        if (nodes != nullptr)
        {
            nodes[ready + 3] = reinterpret_cast<Output*>(fourth & ~clearMask);
        }
    }
    for (; ready < count; ++ready)
    {
        const uintptr_t bits = head->m_cells[Segment::mapped(begin + ready)].load(std::memory_order_acquire);
        if (bits <= 1)
        {
            break;
        }
        if (nodes != nullptr)
        {
            nodes[ready] = reinterpret_cast<Output*>(bits & ~clearMask);
        }
    }
    return ready;
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
            // 换保护对象时先使本地提示失效；即使此次验证失败，也不能沿用上一段的提示。
            record.m_generation = 0;
            record.m_hazard.store(segment);
        }
        // SC publication/validation pairs with detach and the pool's SC scan.
        // A recycled address is safe too: no old hint survives a cleared hazard.
        if (m_tail.load() == segment)
        {
            // 保持同一有效hazard期间段不能被复用；无需逐项读取与预约计数共行的不可变代次。
            // 空闲保护被清除后changed必为true，0标记要求重新验证和取得当前代次。
            if (record.m_generation == 0)
            {
                record.m_hint = segment->m_tail.load(std::memory_order_acquire);
                record.m_generation = segment->m_generation;
            }
            return segment;
        }
        const uint64_t retries = record.m_retries.load(std::memory_order_relaxed);
        record.m_retries.store(retries + (retries != UINT64_MAX ? 1 : 0), std::memory_order_relaxed);
        if (!record.m_progress.retry())
        {
            return nullptr;
        }
    }
}

template <class Node, class SegmentPool, class RecordType>
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::help(Segment* segment, size_t position)
{
    size_t shared = segment->m_tail.load(std::memory_order_relaxed);
    if (shared < position)
    {
        // Default publication proves an occupied prefix. During a reserved
        // transition this also exposes the completed slot beyond any reserved
        // holes; frontReserved cancels such holes without waiting for owners.
        segment->m_tail.compare_exchange_strong(shared, position, std::memory_order_release, std::memory_order_relaxed);
    }
}

template <class Node, class SegmentPool, class RecordType>
#ifdef _MSC_VER
__forceinline
#else
inline
#endif
bool LockFreeQueueChannel<Node, SegmentPool, RecordType>::publish(Node* node, Record& record, SegmentPool& pool)
{
    if (m_cancelableReservations)
    {
        if (!record.m_reservedConcurrent)
        {
            if (pool.singlePublisherRegistered())
            {
                return publishSequential<true>(node, record, pool);
            }
            // Registration history cannot decrease before quiescent reset.
            // Cache only the irreversible multi-record result under this lease.
            record.m_reservedConcurrent = true;
        }
        return publishReserved(node, record, pool);
    }
    return publishSequential<false>(node, record, pool);
}

template <class Node, class SegmentPool, class RecordType>
template <bool Reserved>
bool LockFreeQueueChannel<Node, SegmentPool, RecordType>::publishSequential(Node* node, Record& record, SegmentPool& pool)
{
    for (;;)
    {
        Segment* segment = protect(record);
        if (segment == nullptr)
        {
            return false;
        }
        size_t position = (std::max)(record.m_hint, segment->m_tail.load(std::memory_order_acquire));
        Segment* next = segment->m_next.load(std::memory_order_acquire);
        if (next != nullptr)
        {
            // A consumer may request a partial close to release an idle grown
            // block. Any producer can publish the marker; no closer is awaited.
            close(segment, position);
            m_tail.compare_exchange_strong(segment, next);
            record.m_progress.advance(Segment::kCellCount);
            continue;
        }
        while (position < Segment::kCellCount)
        {
            LockFreeQueueAtomic<uintptr_t>& cell = segment->m_cells[Segment::mapped(position)];
            uintptr_t expected = cell.load(std::memory_order_acquire);
            if (expected == 1)
            {
                if (Reserved)
                {
                    // A reserved tombstone closes one slot, not this segment.
                    return publishReserved(node, record, pool);
                }
                break;
            }
            if (expected == 0)
            {
                if (cell.compare_exchange_strong(expected, reinterpret_cast<uintptr_t>(node), std::memory_order_acq_rel,
                        std::memory_order_acquire))
                {
                    // Enqueue LP; the consumer may immediately free the payload.
                    // Default mode publishes a contiguous prefix. A sole reserved
                    // publisher skips reservation until another record joins;
                    // its next call then uses publishReserved. No node access follows.
                    record.m_hint = position + 1;
                    if (((position + 1) & 15) == 0 ||
                        (Reserved && !pool.singlePublisherRegistered()))
                    {
                        help(segment, position + 1);
                    }
                    return true;
                }
                if (Reserved)
                {
                    // A joining publisher or consumer won this slot. Retry our
                    // own still-private payload using the general reserved path.
                    const uint64_t retries = record.m_retries.load(std::memory_order_relaxed);
                    record.m_retries.store(retries + (retries != UINT64_MAX ? 1 : 0), std::memory_order_relaxed);
                    return record.m_progress.retry() && publishReserved(node, record, pool);
                }
                if (expected == 1)
                {
                    break;
                }
                const uint64_t retries = record.m_retries.load(std::memory_order_relaxed);
                record.m_retries.store(retries + (retries != UINT64_MAX ? 1 : 0), std::memory_order_relaxed);
                if (record.m_progress.waiting() && pool.amplePublisherCapacity())
                {
                    LockFreeQueueProgress::pausePublisher();
                }
                else
                {
                    // 保留短暂 CPU 退避以降低缓存行反复争抢；周期调度让步仍由 retry 完成。
#if (defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))) || defined(__i386__) || defined(__x86_64__)
                    const size_t kContentionPauses = 2048;
#else
                    const size_t kContentionPauses = 64;
#endif
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
                if (!record.m_progress.retry(64))
                {
                    return false;
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
        record.m_progress.advance(Segment::kCellCount);
    }
}

template <class Node, class SegmentPool, class RecordType>
bool LockFreeQueueChannel<Node, SegmentPool, RecordType>::possiblyReady() const
{
    if (m_frontCell == nullptr)
    {
        // 段边界不能误报为空，已链接后继可能持有完整发布项。
        return m_head != nullptr;
    }
    // 预约模式在生产者链接后继前已耗尽本段；消费者 seal 返回前也封闭 tail。
    // 段内 tail 等于当前游标时无需再读 next；段末由上面的分支处理。
    return m_frontCell->load(std::memory_order_acquire) != 0 ||
        (m_cancelableReservations && m_head->m_tail.load(std::memory_order_acquire) != m_headIndex);
}

template <class Node, class SegmentPool, class RecordType>
Node* LockFreeQueueChannel<Node, SegmentPool, RecordType>::front(SegmentPool& pool)
{
    advanceConsumer();
    if (m_cancelableReservations)
    {
        if (m_frontCell != nullptr)
        {
            const uintptr_t bits = m_frontCell->load(std::memory_order_acquire);
            if (bits > 1)
            {
                return reinterpret_cast<Node*>(bits);
            }
            if (bits == 0)
            {
                if (!m_reservedConsumerConcurrent && !pool.singlePublisherRegistered())
                {
                    // Registration history only increases within this generation.
                    // Cache only the multi-record observation, never a sole owner.
                    m_reservedConsumerConcurrent = true;
                }
                if ((m_pendingSeal != m_head && !m_reservedConsumerConcurrent) ||
                    m_head->m_tail.load(std::memory_order_acquire) == m_headIndex)
                {
                    // A pending consumer seal must still inspect the reservation
                    // counter so a linked successor cannot be hidden by this cache.
                    return nullptr;
                }
            }
        }
        return frontReserved(pool);
    }
    // Only the consumer changes this address; the segment stays alive until
    // advance retires it. Empty probes need one pointer load and one cell load.
    if (m_frontCell == nullptr)
    {
        return advance(pool);
    }
    const uintptr_t bits = m_frontCell->load(std::memory_order_acquire);
    if (bits == 1)
    {
        return advance(pool);
    }
    return reinterpret_cast<Node*>(bits);
}

template <class Node, class SegmentPool, class RecordType>
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
Node* LockFreeQueueChannel<Node, SegmentPool, RecordType>::advance(SegmentPool& pool)
{
    if (m_head == nullptr)
    {
        return nullptr;
    }
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
    m_frontCell = &next->m_cells[0];
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
    if (m_cancelableReservations)
    {
        position = segment->m_tail.load(std::memory_order_acquire);
        while (position < Segment::kCellCount && !segment->m_tail.compare_exchange_strong(position,
            Segment::kCellCount, std::memory_order_acq_rel, std::memory_order_acquire))
        {

        }
        return;
    }
    for (; position < Segment::kCellCount; ++position)
    {
        LockFreeQueueAtomic<uintptr_t>& cell = segment->m_cells[Segment::mapped(position)];
        uintptr_t expected = cell.load(std::memory_order_acquire);
        // A failed CAS refreshes expected, including a marker that is already present.
        if ((expected == 0 && cell.compare_exchange_strong(expected, 1, std::memory_order_acq_rel,
            std::memory_order_acquire)) || expected == 1)
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
void LockFreeQueueChannel<Node, SegmentPool, RecordType>::consume(size_t count)
{
    if (count > 1)
    {
        advanceConsumer(count - 1);
    }
    // Sole consumer invokes this only after front succeeded and assignment
    // completed. No cell is cleared until the entire detached segment is safe.
    m_headIndex += count;
    m_frontCell = m_head != nullptr && m_headIndex < Segment::kCellCount
        ? &m_head->m_cells[Segment::mapped(m_headIndex)] : nullptr;
}

template <class Node, class SegmentPool, class RecordType>
typename LockFreeQueueChannel<Node, SegmentPool, RecordType>::Segment* LockFreeQueueChannel<Node, SegmentPool,
    RecordType>::consumerSegment() const
{
    return m_head;
}

template <class Node, class SegmentPool, class RecordType>
bool LockFreeQueueChannel<Node, SegmentPool, RecordType>::publishReserved(Node* node, Record& record, SegmentPool& pool)
{
    for (;;)
    {
        Segment* segment = protect(record);
        if (segment == nullptr)
        {
            return false;
        }
        size_t position = segment->m_tail.load(std::memory_order_relaxed);
        if (position < Segment::kCellCount)
        {
            position = segment->m_tail.fetch_add(1, std::memory_order_acq_rel);
            if (position < Segment::kCellCount)
            {
                uintptr_t expected = 0;
                if (segment->m_cells[Segment::mapped(position)].compare_exchange_strong(expected,
                    reinterpret_cast<uintptr_t>(node), std::memory_order_acq_rel, std::memory_order_acquire))
                {
                    return true;
                }
                // 唯一消费者已经取消本预约；载荷从未移交，重新申请位置。
                const uint64_t retries = record.m_retries.load(std::memory_order_relaxed);
                record.m_retries.store(retries + (retries != UINT64_MAX ? 1 : 0), std::memory_order_relaxed);
                if (!record.m_progress.retry())
                {
                    return false;
                }
                continue;
            }
        }
        Segment* next = segment->m_next.load(std::memory_order_acquire);
        if (next == nullptr)
        {
            Segment* candidate = pool.acquire();
            if (candidate == nullptr)
            {
                return false;
            }
            if (segment->m_next.compare_exchange_strong(next, candidate,
                std::memory_order_acq_rel, std::memory_order_acquire))
            {
                next = candidate;
            }
            else
            {
                pool.release(candidate);
            }
        }
        m_tail.compare_exchange_strong(segment, next);
        record.m_progress.advance(Segment::kCellCount);
    }
}

template <class Node, class SegmentPool, class RecordType>
Node* LockFreeQueueChannel<Node, SegmentPool, RecordType>::frontReserved(SegmentPool& pool)
{
    if (m_head == nullptr)
    {
        return nullptr;
    }
    Segment* boundary = nullptr;
    size_t boundaryIndex = 0;
    bool observedReady = true;
    for (;;)
    {
        if (m_head == boundary && m_headIndex >= boundaryIndex)
        {
            return nullptr;
        }
        if (m_headIndex == Segment::kCellCount)
        {
            Segment* next = m_head->m_next.load(std::memory_order_acquire);
            if (next == nullptr)
            {
                return nullptr;
            }
            Segment* previous = m_head;
            Segment* expected = previous;
            m_tail.compare_exchange_strong(expected, next);
            m_head = next;
            m_headIndex = 0;
            m_frontCell = &next->m_cells[0];
            if (m_pendingSeal == previous)
            {
                m_pendingSeal = nullptr;
            }
            pool.retire(previous);
        }
        LockFreeQueueAtomic<uintptr_t>& cell = m_head->m_cells[Segment::mapped(m_headIndex)];
        uintptr_t bits = cell.load(std::memory_order_acquire);
        if (bits > 1)
        {
            return reinterpret_cast<Node*>(bits);
        }
        if (bits == 0)
        {
            if (boundary == nullptr)
            {
                // 冻结本次空观察的范围；后来不断产生的预约不能让一次 pop 永不返回。
                boundary = m_tail.load(std::memory_order_acquire);
                boundaryIndex = (std::min)(boundary->m_tail.load(std::memory_order_acquire), Segment::kCellCount);
                observedReady = publishedBefore(boundary, boundaryIndex);
            }
            if (m_head == boundary && (m_headIndex >= boundaryIndex || !observedReady))
            {
                // 空观察仍越过已经封闭的旧段，但保留活动尾段中尚未完成的预约。
                return nullptr;
            }
            if (!cell.compare_exchange_strong(bits, 1, std::memory_order_acq_rel, std::memory_order_acquire))
            {
                return reinterpret_cast<Node*>(bits);
            }
        }
        // 未完成的生产者不构成队头阻塞；取消后它必须在新位置重新发布。
        advanceConsumer();
        ++m_headIndex;
        m_frontCell = m_headIndex < Segment::kCellCount ? &m_head->m_cells[Segment::mapped(m_headIndex)] : nullptr;
    }
}

template <class Node, class SegmentPool, class RecordType>
bool LockFreeQueueChannel<Node, SegmentPool, RecordType>::publishedBefore(Segment* boundary, size_t boundaryIndex)
{
    Segment* segment = m_head;
    size_t begin = m_headIndex;
    while (segment != nullptr)
    {
        const size_t end = segment == boundary ? boundaryIndex : Segment::kCellCount;
        for (size_t i = begin; i < end; ++i)
        {
            if (segment->m_cells[Segment::mapped(i)].load(std::memory_order_acquire) > 1)
            {
                return true;
            }
        }
        // 单发布者留下的尾提示可能落后于消费游标。只给实际扫描过的槽位计费。
        if (begin < end)
        {
            advanceConsumer(end - begin);
        }
        if (segment == boundary)
        {
            return false;
        }
        segment = segment->m_next.load(std::memory_order_acquire);
        begin = 0;
    }
    return false;
}

#endif