#ifndef LOCK_FREE_QUEUE_ARENA_MODELS_INL
#define LOCK_FREE_QUEUE_ARENA_MODELS_INL

template <class T>
LockFreeQueueArenaAuxiliary<T>::LockFreeQueueArenaAuxiliary() : m_bits(0)
{

}

template <class T>
LockFreeQueueArenaNode<T>::LockFreeQueueArenaNode(LockFreeQueueArenaBlock<T>* block) :
m_owner(reinterpret_cast<uintptr_t>(block))
{

}

template <class T>
LockFreeQueueArenaNode<T>::~LockFreeQueueArenaNode()
{
    reset();
}

template <class T>
LockFreeQueueArenaBlock<T>* LockFreeQueueArenaNode<T>::block() const
{
    return reinterpret_cast<LockFreeQueueArenaBlock<T>*>(m_owner & ~uintptr_t(1));
}

template <class T>
T* LockFreeQueueArenaNode<T>::data()
{
    if (std::alignment_of<T>::value <= std::alignment_of<uintptr_t>::value)
    {
        // Storage follows one pointer-width scalar in an equally aligned Node.
        return reinterpret_cast<T*>(m_storage);
    }
    const uintptr_t alignment = std::alignment_of<T>::value;
    const uintptr_t raw = reinterpret_cast<uintptr_t>(m_storage);
    return reinterpret_cast<T*>((raw + alignment - 1) & ~(alignment - 1));
}

template <class T>
void LockFreeQueueArenaNode<T>::construct(const T& value)
{
    new (data()) T(value);
    m_owner |= 1;
}

template <class T>
void LockFreeQueueArenaNode<T>::reset()
{
    if ((m_owner & 1) != 0)
    {
        data()->~T();
        m_owner &= ~uintptr_t(1);
    }
}

template <class T>
LockFreeQueueArenaBlock<T>::LockFreeQueueArenaBlock(size_t capacity, bool initial) :
m_next(nullptr), m_state(0), m_freeNodes(capacity - (initial ? 1 : 0)),
m_freeSegments((capacity + Segment::kCellCount - 1) / Segment::kCellCount + 2), m_activity(0), m_nodes(nullptr),
m_segments(nullptr), m_nodeBits(nullptr), m_segmentBits(nullptr), m_capacity(capacity),
m_nodeWords((capacity + 31) / 32), m_segmentCount((capacity + Segment::kCellCount - 1) / Segment::kCellCount + 2),
m_segmentWords((m_segmentCount + 31) / 32), m_bytes(0), m_storage(nullptr), m_initial(initial), m_seenActivity(0),
m_idleSince(0), m_idleObserved(false), m_retiredNext(nullptr), m_returnedNodes(0)
{

}

template <class T>
size_t LockFreeQueueArenaBlock<T>::freeNodes() const
{
    const size_t returned = m_returnedNodes.load(std::memory_order_acquire);
    const size_t free = m_freeNodes.load(std::memory_order_acquire) + returned;
    return free <= m_capacity ? free : 0;
}

template <class T>
LockFreeQueueArenaRecord<T>::LockFreeQueueArenaRecord() :
m_hazard(nullptr), m_hint(0), m_retries(0), m_generation(0), m_active(0), m_cachedMask(0), m_block(nullptr),
m_cachedBlock(nullptr), m_wordHint(0)
{

}

#endif