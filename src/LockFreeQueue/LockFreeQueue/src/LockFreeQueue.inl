#ifndef LOCK_FREE_QUEUE_INL
#define LOCK_FREE_QUEUE_INL

template <class T>
LockFreeQueue<T>::LockFreeQueue() : m_channel(nullptr), m_prepared(0), m_maintenance(0)
{
    init();
}

template <class T>
LockFreeQueue<T>::~LockFreeQueue()
{
    destroy();
}

template <class T>
void LockFreeQueue<T>::init()
{
    const LockFreeQueueResult result = tryInit();
    if (result != LockFreeQueueSuccess)
    {
        throw LockFreeQueueError(result);
    }
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::tryInit()
{
    destroy();
    LockFreeQueueResult result = m_arena.initialize();
    if (result != LockFreeQueueSuccess)
    {
        return result;
    }
    SegmentPool pool(m_arena, nullptr);
    typename Arena::Segment* first = pool.acquireInitial();
    if (first == nullptr)
    {
        m_arena.destroy();
        return LockFreeQueueAllocationFailure;
    }
    m_channel.reset(first);
    if (!atomicsLockFree())
    {
        destroy();
        return LockFreeQueueAtomicUnavailable;
    }
    return LockFreeQueueSuccess;
}

template <class T>
void LockFreeQueue<T>::destroy()
{
    // The arena destroys engaged payloads under the lifecycle's quiescence
    // contract. No O(backlog) pop traversal is needed for this explicit operation.
    m_channel.reset(nullptr);
    m_arena.destroy();
    m_prepared.store(0, std::memory_order_relaxed);
    m_maintenance = 0;
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::configurePool(const LockFreeQueuePoolConfig& config)
{
    return m_arena.configure(config);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::setGrowPolicy(const LockFreeQueueGrowPolicy& policy)
{
    return m_arena.setGrowPolicy(policy);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::setShrinkPolicy(const LockFreeQueueShrinkPolicy& policy)
{
    return m_arena.setShrinkPolicy(policy);
}

template <class T>
void LockFreeQueue<T>::push(const T& value)
{
    const LockFreeQueueResult result = pushImpl(value, true, nullptr);
    if (result != LockFreeQueueSuccess)
    {
        throw LockFreeQueueError(result);
    }
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::tryPush(const T& value)
{
    return pushImpl(value, false, nullptr);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::tryAcquireProducer(LockFreeQueueProducer<T>& producer)
{
    if (producer.attached())
    {
        return LockFreeQueueTemporaryUnavailable;
    }
    if (!m_arena.initialized())
    {
        return LockFreeQueueNotInitialized;
    }
    Record* record = m_arena.enter();
    if (record == nullptr)
    {
        return LockFreeQueueTemporaryUnavailable;
    }
    producer.m_owner = this;
    producer.m_record = record;
    return LockFreeQueueSuccess;
}

template <class T>
void LockFreeQueue<T>::releaseProducer(Record& record)
{
    m_arena.releaseCache(record);
    m_arena.leave(record, true);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::pushImpl(const T& value, bool propagate, Record* retained)
{
    if (!m_arena.initialized())
    {
        return LockFreeQueueNotInitialized;
    }
    Record* record = retained == nullptr ? m_arena.enter() : retained;
    LockFreeQueueArenaLease<T> lease(m_arena, record, retained == nullptr);
    if (record == nullptr)
    {
        return LockFreeQueueTemporaryUnavailable;
    }
    LockFreeQueueResult result = LockFreeQueueSuccess;
    Node* node = m_arena.acquireNode(*record, result);
    if (node == nullptr)
    {
        return result;
    }
    try
    {
        node->construct(value);
    }
    catch (...)
    {
        m_arena.releaseNode(node, record);
        m_arena.releaseCache(*record);
        if (propagate)
        {
            throw;
        }
        return LockFreeQueueConstructionFailure;
    }
    m_prepared.fetch_add(1, std::memory_order_relaxed);
    SegmentPool pool(m_arena, record);
    if (!m_channel.publish(node, *record, pool))
    {
        m_prepared.fetch_sub(1, std::memory_order_relaxed);
        m_arena.releaseNode(node, record);
        m_arena.releaseCache(*record);
        return pool.result();
    }
    return LockFreeQueueSuccess;
}

template <class T>
void LockFreeQueue<T>::assign(T& output, T& source, std::true_type)
{
    output = std::move(source);
}

template <class T>
void LockFreeQueue<T>::assign(T& output, T& source, std::false_type)
{
    output = static_cast<const T&>(source);
}

template <class T>
bool LockFreeQueue<T>::pop(T* output)
{
    SegmentPool pool(m_arena, nullptr);
    Node* node = m_channel.front(pool);
    if (node == nullptr)
    {
        // Producers may have released their last hazards after the final item
        // was consumed. Empty polling must also make bounded reclamation progress.
        if (m_arena.hasRetiredSegments())
        {
            m_arena.collectSegments(2);
        }
        return false;
    }
    return consumeNode(node, output);
}

template <class T>
bool LockFreeQueue<T>::consumeNode(Node* node, T* output)
{
    if (output != nullptr)
    {
        assign(*output, *node->data(), typename std::is_nothrow_move_assignable<T>::type());
    }
    m_channel.consume();
    m_arena.releaseNode(node);
    if (m_arena.automaticMaintenanceNeeded() && ++m_maintenance == 64)
    {
        m_maintenance = 0;
        maintain(8);
    }
    return true;
}

template <class T>
void LockFreeQueue<T>::clear()
{
    while (pop(nullptr))
    {

    }
}

template <class T>
bool LockFreeQueue<T>::empty() const
{
    return size() == 0;
}

template <class T>
uint64_t LockFreeQueue<T>::size() const
{
    const size_t consumed = m_arena.consumed();
    return static_cast<size_t>(m_prepared.load(std::memory_order_relaxed) - consumed);
}

template <class T>
LockFreeQueueMaintenanceResult LockFreeQueue<T>::maintain(size_t budget)
{
    if (budget != 0 && m_arena.initialized() && m_arena.shrinkEnabled())
    {
        typename Arena::Segment* head = m_channel.consumerSegment();
        if (head->m_owner != m_arena.initialBlock())
        {
            SegmentPool pool(m_arena, nullptr);
            m_channel.seal(pool);
        }
    }
    return m_arena.maintain(budget);
}

template <class T>
LockFreeQueuePoolStatistics LockFreeQueue<T>::poolStatistics() const
{
    LockFreeQueuePoolStatistics result = m_arena.statistics();
    result.retainedBytes += sizeof(LockFreeQueue) - sizeof(Arena);
    return result;
}

template <class T>
bool LockFreeQueue<T>::atomicsLockFree() const
{
    return m_arena.atomicsLockFree() && m_prepared.is_lock_free();
}



#endif