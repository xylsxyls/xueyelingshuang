#ifndef LOCK_FREE_QUEUE_INL
#define LOCK_FREE_QUEUE_INL

#include <thread>

template <class T>
LockFreeQueue<T>::LockFreeQueue() :
m_channel(nullptr),
m_cancelableReservations(false),
m_prepared(0),
m_distributedCounters(false),
m_maintenance(0),
m_prefixSingleBudget(0)
{
    init();
}

template <class T>
LockFreeQueue<T>::LockFreeQueue(bool initializeImmediately) :
m_channel(nullptr),
m_cancelableReservations(false),
m_prepared(0),
m_distributedCounters(false),
m_maintenance(0),
m_prefixSingleBudget(0)
{
    if (initializeImmediately)
    {
        init();
    }
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
    m_channel.reset(first, m_cancelableReservations);
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
    m_prefixSingleBudget = 0;
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::configurePool(const LockFreeQueuePoolConfig& config)
{
    return m_arena.configure(config);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::configureProducerCounters(bool distributed)
{
    if (m_arena.initialized())
    {
        return LockFreeQueueInvalidState;
    }
    m_distributedCounters = distributed;
    return LockFreeQueueSuccess;
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::configureEntryOnly(bool enabled)
{
    return m_arena.configureEntryOnly(enabled);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::configureCancelableReservations(bool enabled)
{
    if (m_arena.initialized())
    {
        return LockFreeQueueInvalidState;
    }
    m_cancelableReservations = enabled;
    return LockFreeQueueSuccess;
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
LockFreeQueueResult LockFreeQueue<T>::push(const T& value)
{
    return pushImpl(value, true, nullptr);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::tryPush(const T& value)
{
    return pushImpl(value, false, nullptr);
}

template <class T>
void LockFreeQueue<T>::publicationFence()
{
#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER == 1800 && (defined(_M_X64) || defined(_M_IX86))
    // v120 x86/x64 的槽位 CAS 使用带完整硬件及编译器屏障的 Interlocked。
    // 发布之后未执行任何需要再次排序的队列写入，不再重复发出 MFENCE。
    _ReadWriteBarrier();
#else
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::tryPushAndFence(const T& value)
{
    const LockFreeQueueResult result = pushImpl(value, false, nullptr);
    if (result == LockFreeQueueSuccess)
    {
        publicationFence();
    }
    return result;
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::tryAcquireProducer(LockFreeQueueProducer<T>& producer)
{
    if (producer.attached())
    {
        return LockFreeQueueInvalidState;
    }
    if (!m_arena.initialized())
    {
        return LockFreeQueueNotInitialized;
    }
    LockFreeQueueResult result = LockFreeQueueSuccess;
    Record* record = m_arena.enter(false, &result);
    if (record == nullptr)
    {
        return result;
    }
    producer.m_owner = this;
    producer.m_record = record;
    return LockFreeQueueSuccess;
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::tryPushEntry(LockFreeQueueEntry<T>& entry)
{
    return pushEntryImpl(entry, nullptr);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::tryPushEntryAndFence(LockFreeQueueEntry<T>& entry)
{
    const LockFreeQueueResult result = pushEntryImpl(entry, nullptr);
    if (result == LockFreeQueueSuccess)
    {
        publicationFence();
    }
    return result;
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::pushEntryImpl(LockFreeQueueEntry<T>& entry, Record* retained)
{
    if (!m_arena.initialized())
    {
        return LockFreeQueueNotInitialized;
    }
    LockFreeQueueResult result = LockFreeQueueSuccess;
    Record* record = retained == nullptr ? m_arena.enter(false, &result) : retained;
    LockFreeQueueArenaLease<T> lease(m_arena, record, retained == nullptr);
    if (record == nullptr)
    {
        return result;
    }
    record->m_progress.begin(false);
    record->m_progress.advance();
    Node* credit = m_arena.acquireNode(*record, result);
    if (credit == nullptr)
    {
        return result;
    }
    entry.m_cookie = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(credit));
    entry.m_owner = m_arena.creditOwner(credit);
    Node* address = reinterpret_cast<Node*>(reinterpret_cast<uintptr_t>(&entry) | uintptr_t(2));
    result = publishNode(address, credit, *record);
    if (result != LockFreeQueueSuccess)
    {
        entry.m_cookie = 0;
        entry.m_owner = nullptr;
    }
    // 成功后消费者可能已经归还外部对象，不再读取或写入 entry。
    return result;
}

template <class T>
void LockFreeQueue<T>::releaseProducer(Record& record)
{
    m_arena.releaseCache(record);
    m_arena.leave(record, true);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::pushImpl(const T& value, bool wait, Record* retained)
{
    if (m_arena.entryOnly())
    {
        return LockFreeQueueInvalidConfiguration;
    }
    if (!m_arena.initialized())
    {
        return LockFreeQueueNotInitialized;
    }
    LockFreeQueueResult result = LockFreeQueueSuccess;
    Record* record = retained == nullptr ? m_arena.enter(wait, &result) : retained;
    LockFreeQueueArenaLease<T> lease(m_arena, record, retained == nullptr);
    if (record == nullptr)
    {
        return result;
    }
    record->m_progress.begin(wait);
    record->m_progress.advance();
    Node* node = nullptr;
    for (;;)
    {
        node = m_arena.acquireNode(*record, result);
        if (node != nullptr)
        {
            break;
        }
        if (!wait || !record->m_progress.interrupted())
        {
            return result;
        }
        record->m_progress.resume();
        record->m_progress.retry();
    }
    try
    {
        node->construct(value);
    }
    catch (...)
    {
        m_arena.releaseNode(node, record);
        m_arena.releaseCache(*record);
        return LockFreeQueueConstructionFailure;
    }
    return publishNode(node, node, *record);
}

template <class T>
LockFreeQueueResult LockFreeQueue<T>::publishNode(Node* address, Node* credit, Record& record)
{
    if (m_distributedCounters)
    {
        record.m_published.store(record.m_published.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }
    else
    {
        m_prepared.fetch_add(1, std::memory_order_relaxed);
    }
    SegmentPool pool(m_arena, &record);
    while (!m_channel.publish(address, record, pool))
    {
        if (record.m_progress.interrupted() && record.m_progress.waiting())
        {
            record.m_progress.resume();
            record.m_progress.retry();
            continue;
        }
        if (m_distributedCounters)
        {
            record.m_published.store(record.m_published.load(std::memory_order_relaxed) - 1, std::memory_order_release);
        }
        else
        {
            m_prepared.fetch_sub(1, std::memory_order_relaxed);
        }
        m_arena.releaseNode(credit, &record);
        // 竞争失败只退还当前载荷；保留已领取缓存，避免 tryPush 重试反复清空批次。
        if (!record.m_progress.interrupted())
        {
            m_arena.releaseCache(record);
        }
        return record.m_progress.interrupted() ? LockFreeQueueBusy : pool.result();
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
void LockFreeQueue<T>::assignEntry(T& output, const T& source)
{
    T copy(source);
    assign(output, copy, typename std::is_nothrow_move_assignable<T>::type());
}

template <class T>
typename LockFreeQueue<T>::Node* LockFreeQueue<T>::copyFront(Node* address, T* output, Block*& externalOwner)
{
    const uintptr_t encoded = reinterpret_cast<uintptr_t>(address);
    if ((encoded & uintptr_t(2)) != 0)
    {
        LockFreeQueueEntry<T>* entry = reinterpret_cast<LockFreeQueueEntry<T>*>(encoded & ~uintptr_t(2));
        Node* credit = reinterpret_cast<Node*>(static_cast<uintptr_t>(entry->m_cookie));
        externalOwner = static_cast<Block*>(entry->m_owner);
        if (output != nullptr)
        {
            assignEntry(*output, entry->m_value);
        }
        return credit;
    }
    externalOwner = nullptr;
    if (output != nullptr)
    {
        assign(*output, *address->data(), typename std::is_nothrow_move_assignable<T>::type());
    }
    return address;
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
bool LockFreeQueue<T>::popReady(T* output)
{
    SegmentPool pool(m_arena, nullptr);
    Node* node = m_channel.front(pool);
    return node != nullptr && consumeNode(node, output);
}

template <class T>
bool LockFreeQueue<T>::popReadyBatch(T* output, size_t maximum, size_t& consumed)
{
    consumed = 0;
    if (maximum == 0)
    {
        return false;
    }
    SegmentPool pool(m_arena, nullptr);
    Node* node = m_channel.front(pool);
    return node != nullptr && consumeBatch(output, (std::min)(maximum, size_t(16)), consumed);
}

template <class T>
#ifdef _MSC_VER
__forceinline
#else
inline
#endif
bool LockFreeQueue<T>::popReadyPrefix(T* output, size_t maximum, size_t& consumed)
{
    consumed = 0;
    if (maximum == 0)
    {
        return false;
    }
    SegmentPool pool(m_arena, nullptr);
    Node* node = m_channel.front(pool);
    if (node == nullptr)
    {
        return false;
    }
    if (maximum == 1 || m_prefixSingleBudget != 0)
    {
        if (m_prefixSingleBudget != 0)
        {
            --m_prefixSingleBudget;
        }
        return consumeNode(node, output, &consumed);
    }
    const bool result = consumeBatch(output, (std::min)(maximum, size_t(64)), consumed);
    if (consumed < 4)
    {
        m_prefixSingleBudget = 7;
    }
    return result;
}

template <class T>
#ifdef _MSC_VER
__forceinline
#else
inline
#endif
bool LockFreeQueue<T>::popReadyEntryPrefix(LockFreeQueueEntry<T>** output, size_t maximum, size_t& consumed)
{
    consumed = 0;
    if (maximum == 0)
    {
        return false;
    }
    SegmentPool pool(m_arena, nullptr);
    Node* node = m_channel.front(pool);
    if (node == nullptr || !m_arena.entryOnly())
    {
        return false;
    }
    if (maximum == 1 || m_prefixSingleBudget != 0)
    {
        if (m_prefixSingleBudget != 0)
        {
            --m_prefixSingleBudget;
        }
        // 在额度归还和调度交接前发出读预取；此时 Entry 仍在本次消费借用范围内。
        const void* address = reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(node) & ~uintptr_t(2));
#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86) && _M_IX86_FP >= 1))
        _mm_prefetch(static_cast<const char*>(address), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
        __builtin_prefetch(address, 0, 3);
#else
        (void)address;
#endif
        Block* fixed = m_arena.fixedEntryBlock();
        LockFreeQueueEntry<T>* entry = reinterpret_cast<LockFreeQueueEntry<T>*>(reinterpret_cast<uintptr_t>(node) & ~uintptr_t(2));
        Block* owner = fixed != nullptr ? fixed : static_cast<Block*>(entry->m_owner);
        if (output != nullptr)
        {
            output[0] = entry;
        }
        m_channel.consume();
        consumed = 1;
        m_arena.releaseEntryCredits(*owner, 1);
        if (m_arena.automaticMaintenanceNeeded() && ++m_maintenance == 64)
        {
            m_maintenance = 0;
            maintain(8);
        }
        return true;
    }
    return consumeEntryPrefix(node, output, maximum, consumed);
}

template <class T>
bool LockFreeQueue<T>::consumeEntryPrefix(Node* node, LockFreeQueueEntry<T>** output, size_t maximum, size_t& consumed)
{
    // 在额度归还和调度交接前发出读预取；此时 Entry 仍在本次消费借用范围内。
    const void* address = reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(node) & ~uintptr_t(2));
#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86) && _M_IX86_FP >= 1))
    _mm_prefetch(static_cast<const char*>(address), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_prefetch(address, 0, 3);
#else
    (void)address;
#endif
    Block* fixed = m_arena.fixedEntryBlock();
    Node* entries[64];
    Node* credits[64];
    consumed = m_channel.readyPrefix(entries, (std::min)(maximum, size_t(64)));
    for (size_t i = 0; i < consumed; ++i)
    {
        LockFreeQueueEntry<T>* entry = reinterpret_cast<LockFreeQueueEntry<T>*>(reinterpret_cast<uintptr_t>(entries[i]) & ~uintptr_t(2));
        if (fixed == nullptr)
        {
            credits[i] = reinterpret_cast<Node*>(entry->m_owner);
        }
        if (output != nullptr)
        {
            output[i] = entry;
        }
    }
    m_channel.consume(consumed);
    if (fixed != nullptr)
    {
        releaseBatch(*fixed, consumed);
    }
    else
    {
        releaseBatch(credits, nullptr, consumed);
    }
    if (consumed < 4)
    {
        m_prefixSingleBudget = 7;
    }
    return consumed != 0;
}

template <class T>
inline bool LockFreeQueue<T>::possiblyReady() const
{
    return m_channel.possiblyReady();
}

template <class T>
bool LockFreeQueue<T>::consumeBatch(T* output, size_t maximum, size_t& consumed)
{
    Node* nodes[64];
    Block* externalOwners[64];
    try
    {
        Node* ready[64];
        const size_t available = m_channel.readyPrefix(ready, maximum);
        // 只消费本次观察到的同段前缀；后续发布留给下次调用，不追赶正在前进的尾部。
        // 先并行预取；输出赋值异常仍只提交此前成功的元素。
        for (size_t i = 0; i < available; ++i)
        {
            const void* address = reinterpret_cast<const void*>(reinterpret_cast<uintptr_t>(ready[i]) & ~uintptr_t(2));
#if defined(_MSC_VER) && (defined(_M_X64) || (defined(_M_IX86) && _M_IX86_FP >= 1))
            // 外部借用载荷只读；不提前申请写独占权，使发布者的后续只读访问失效。
            _mm_prefetch(static_cast<const char*>(address), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
            __builtin_prefetch(address, 0, 3);
#endif
        }
        for (size_t i = 0; i < available; ++i)
        {
            Node* credit = copyFront(ready[i], output != nullptr ? &output[consumed] : nullptr, externalOwners[consumed]);
            nodes[consumed++] = credit;
        }
    }
    catch (...)
    {
        m_channel.consume(consumed);
        releaseBatch(nodes, externalOwners, consumed);
        throw;
    }
    m_channel.consume(consumed);
    releaseBatch(nodes, externalOwners, consumed);
    return consumed != 0;
}

template <class T>
void LockFreeQueue<T>::releaseBatch(Node* const* nodes, Block* const* externalOwners, size_t count)
{
    if (count != 0)
    {
        m_arena.releaseNodes(nodes, count, externalOwners);
        if (m_arena.automaticMaintenanceNeeded())
        {
            m_maintenance += count;
            if (m_maintenance >= 64)
            {
                m_maintenance %= 64;
                maintain(8);
            }
        }
    }
}

template <class T>
void LockFreeQueue<T>::releaseBatch(Block& owner, size_t count)
{
    if (count != 0)
    {
        m_arena.releaseEntryCredits(owner, count);
        if (m_arena.automaticMaintenanceNeeded())
        {
            m_maintenance += count;
            if (m_maintenance >= 64)
            {
                m_maintenance %= 64;
                maintain(8);
            }
        }
    }
}

template <class T>
bool LockFreeQueue<T>::consumeNode(Node* node, T* output, size_t* consumed)
{
    Block* externalOwner = nullptr;
    Node* credit = copyFront(node, output, externalOwner);
    m_channel.consume();
    if (consumed != nullptr)
    {
        *consumed = 1;
    }
    m_arena.releaseNode(credit, nullptr, externalOwner);
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
    return static_cast<size_t>((m_distributedCounters ? m_arena.published() : m_prepared.load(std::memory_order_relaxed)) - consumed);
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