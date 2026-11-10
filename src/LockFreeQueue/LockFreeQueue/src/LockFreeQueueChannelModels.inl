#ifndef LOCK_FREE_QUEUE_CHANNEL_MODELS_INL
#define LOCK_FREE_QUEUE_CHANNEL_MODELS_INL

template <class Node>
LockFreeQueueChannelSegment<Node>::LockFreeQueueChannelSegment() :
m_next(nullptr), m_tail(0), m_poolIndex(0), m_generation(0), m_owner(nullptr), m_retiredNext(nullptr), m_idleSince(0)
{

}

template <class Node>
void LockFreeQueueChannelSegment<Node>::reset()
{
    // Exclusive storage: no live queue or hazard may still name this generation.
    for (size_t i = 0; i < kCellCount; ++i)
    {
        m_cells[i].store(0, std::memory_order_relaxed);
    }
    m_next.store(nullptr, std::memory_order_relaxed);
    m_tail.store(0, std::memory_order_relaxed);
}

template <class Node>
size_t LockFreeQueueChannelSegment<Node>::mapped(size_t position)
{
    // Spread adjacent logical slots across cache lines so a near-tail consumer
    // does not repeatedly invalidate the producer next-slot cache line.
    return ((position & 15) << 4) | (position >> 4);
}

#endif