#ifndef LOCK_FREE_QUEUE_ARENA_AUXILIARY_INL
#define LOCK_FREE_QUEUE_ARENA_AUXILIARY_INL

template <class T>
bool LockFreeQueueArena<T>::assignGeneration(Segment* segment, LockFreeQueueResult* result)
{
    uint64_t generation = m_segmentSequence.load(std::memory_order_relaxed);
    while (generation != UINT64_MAX &&
           !m_segmentSequence.compare_exchange_weak(generation, generation + 1, std::memory_order_relaxed))
    {

    }
    if (generation == UINT64_MAX)
    {
        if (result != nullptr)
        {
            *result = LockFreeQueueSequenceExhausted;
        }
        return false;
    }
    segment->m_generation = generation + 1;
    if (result != nullptr)
    {
        *result = LockFreeQueueSuccess;
    }
    return true;
}

template <class T>
typename LockFreeQueueArena<T>::Segment* LockFreeQueueArena<T>::acquireAuxiliary(
    Record& record, LockFreeQueueResult* result)
{
    const size_t start = static_cast<size_t>(&record - m_records) * 4;
    // Exhaust reusable cached descriptors before allocating any new storage.
    for (size_t pass = 0; pass < 2; ++pass)
    {
        for (size_t step = 0; step < m_auxiliaryCount; ++step)
        {
            const size_t index = (start + step) % m_auxiliaryCount;
            Auxiliary& slot = m_auxiliary[index];
            uintptr_t bits = slot.m_bits.load(std::memory_order_relaxed);
            if ((bits & 1) != 0 || (pass == 0 ? bits == 0 : bits != 0))
            {
                continue;
            }
            if (!slot.m_bits.compare_exchange_strong(
                    bits, bits | 1, std::memory_order_acquire, std::memory_order_relaxed))
            {
                continue;
            }
            Segment* segment = reinterpret_cast<Segment*>(bits);
            if (segment == nullptr)
            {
                increment(m_allocations);
                try
                {
                    segment = new Segment();
                }
                catch (...)
                {
                    slot.m_bits.store(0, std::memory_order_release);
                    if (result != nullptr)
                    {
                        *result = LockFreeQueueAllocationFailure;
                    }
                    return nullptr;
                }
                segment->m_poolIndex = index;
                // Null owner distinguishes the bounded auxiliary arena from
                // segment storage embedded in a payload block.
                segment->m_owner = nullptr;
                m_bytes.fetch_add(sizeof(Segment));
            }
            segment->m_idleSince = 0;
            segment->reset();
            if (!assignGeneration(segment, result))
            {
                slot.m_bits.store(reinterpret_cast<uintptr_t>(segment), std::memory_order_release);
                return nullptr;
            }
            // Publish owned pointer too: externally quiescent destruction must
            // account for segments still in the queue or retirement list.
            slot.m_bits.store(reinterpret_cast<uintptr_t>(segment) | 1, std::memory_order_relaxed);
            return segment;
        }
    }
    if (result != nullptr)
    {
        *result = LockFreeQueueTemporaryUnavailable;
    }
    return nullptr;
}

#endif