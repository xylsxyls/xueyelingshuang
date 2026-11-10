#ifndef LOCK_FREE_QUEUE_MODELS_INL
#define LOCK_FREE_QUEUE_MODELS_INL

template <class T>
LockFreeQueueChannelPool<T>::LockFreeQueueChannelPool(LockFreeQueueArena<T>& arena, Record* record) :
m_arena(&arena), m_record(record), m_result(LockFreeQueueResourceUnavailable)
{

}

template <class T>
typename LockFreeQueueChannelPool<T>::Segment* LockFreeQueueChannelPool<T>::acquire()
{
    return m_record == nullptr ? nullptr : m_arena->acquireSegment(*m_record, false, &m_result);
}

template <class T>
typename LockFreeQueueChannelPool<T>::Segment* LockFreeQueueChannelPool<T>::acquireInitial()
{
    // The initial block is never detached, so this consumer-only claim needs no
    // registered producer hazard and creates no persistent registration.
    Record initialClaim;
    initialClaim.m_progress.begin(false);
    return m_arena->acquireSegment(initialClaim, true);
}

template <class T>
void LockFreeQueueChannelPool<T>::release(Segment* segment)
{
    m_arena->releaseSegment(segment, m_record);
}

template <class T>
void LockFreeQueueChannelPool<T>::retire(Segment* segment)
{
    m_arena->retireSegment(segment);
    // More than one retirement visit per segment keeps paused hazards from
    // accumulating an unbounded queue of otherwise safe descriptors.
    m_arena->collectSegments(2);
}

template <class T>
LockFreeQueueResult LockFreeQueueChannelPool<T>::result() const
{
    return m_result;
}

template <class T>
LockFreeQueueArenaLease<T>::LockFreeQueueArenaLease(LockFreeQueueArena<T>& arena, Record* record, bool releaseLease) :
m_arena(&arena), m_record(record), m_releaseLease(releaseLease)
{

}

template <class T>
LockFreeQueueArenaLease<T>::~LockFreeQueueArenaLease()
{
    if (m_record != nullptr)
    {
        m_arena->leave(*m_record, m_releaseLease, true);
    }
}

#endif