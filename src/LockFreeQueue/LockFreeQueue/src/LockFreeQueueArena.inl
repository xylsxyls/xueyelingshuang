#ifndef LOCK_FREE_QUEUE_ARENA_INL
#define LOCK_FREE_QUEUE_ARENA_INL

template <class T>
LockFreeQueueArena<T>::LockFreeQueueArena() :
m_records(nullptr), m_traversal(nullptr), m_recordStorage(nullptr), m_recordBytes(0), m_auxiliary(nullptr),
m_auxiliaryCount(0), m_auxiliaryBytes(0), m_auxiliaryScan(0), m_high(0), m_blocks(nullptr), m_initial(nullptr),
m_capacity(0), m_availableBase(0), m_returned(0), m_budget(0), m_peak(0), m_retiringCapacity(0), m_bytes(0),
m_blockCount(0), m_grows(0), m_allocations(0), m_lastGrow(0), m_segmentSequence(0), m_limit(0),
m_retiredSegments(nullptr), m_retiredSegmentTail(nullptr), m_retiredBlocks(nullptr), m_retiredBlockTail(nullptr),
m_scan(nullptr), m_previous(nullptr), m_recordScan(0), m_maintenanceTurn(0)
{
    static_assert(sizeof(Record) == 64, "Record stride must isolate producer cache lines");
    static_assert(std::alignment_of<Segment>::value >= 2, "Auxiliary owner bit requires aligned segments");
}

template <class T>
LockFreeQueueArena<T>::~LockFreeQueueArena()
{
    destroy();
}

template <class T>
bool LockFreeQueueArena<T>::validate(const LockFreeQueuePoolConfig& config, size_t& limit)
{
    const size_t maximum = (std::numeric_limits<size_t>::max)();
    if (config.initialCapacity == 0 || config.maximumProducers == 0 || config.maximumCapacity < -1 ||
        config.maximumProducers > (maximum - sizeof(LockFreeQueueArena) - 4096 - 4 * sizeof(Auxiliary)) /
                                      (sizeof(Record) + sizeof(LockFreeQueueAtomic<Block*>) + 4 * sizeof(Auxiliary)))
    {
        return false;
    }
    const size_t fixed =
        config.maximumProducers * (sizeof(Record) + sizeof(LockFreeQueueAtomic<Block*>) + 4 * sizeof(Auxiliary)) +
        sizeof(LockFreeQueueArena) + 4096 + 4 * sizeof(Auxiliary);
    limit = (maximum - fixed) / (sizeof(Node) + 2 * sizeof(uintptr_t) + sizeof(uint32_t));
    if (config.maximumCapacity != -1)
    {
        if (static_cast<uint64_t>(config.maximumCapacity) > static_cast<uint64_t>(limit))
        {
            return false;
        }
        limit = static_cast<size_t>(config.maximumCapacity);
    }
    return config.initialCapacity <= limit;
}

template <class T>
LockFreeQueueResult LockFreeQueueArena<T>::configure(const LockFreeQueuePoolConfig& config)
{
    if (initialized())
    {
        return LockFreeQueueTemporaryUnavailable;
    }
    size_t limit = 0;
    if (!validate(config, limit))
    {
        return LockFreeQueueInvalidConfiguration;
    }
    m_config = config;
    m_limit = limit;
    return LockFreeQueueSuccess;
}

template <class T>
LockFreeQueueResult LockFreeQueueArena<T>::setGrowPolicy(const LockFreeQueueGrowPolicy& policy)
{
    try
    {
        LockFreeQueueGrowPolicy candidate(policy);
        m_growPolicy.swap(candidate);
        return LockFreeQueueSuccess;
    }
    catch (...)
    {
        return LockFreeQueueAllocationFailure;
    }
}

template <class T>
LockFreeQueueResult LockFreeQueueArena<T>::setShrinkPolicy(const LockFreeQueueShrinkPolicy& policy)
{
    try
    {
        LockFreeQueueShrinkPolicy candidate(policy);
        m_shrinkPolicy.swap(candidate);
        return LockFreeQueueSuccess;
    }
    catch (...)
    {
        return LockFreeQueueAllocationFailure;
    }
}

template <class T>
typename LockFreeQueueArena<T>::Block* LockFreeQueueArena<T>::createBlock(size_t amount, bool initial)
{
    const size_t maximum = (std::numeric_limits<size_t>::max)();
    const size_t nodeWords = (amount + 31) / 32;
    const size_t segments = (amount + Segment::kCellCount - 1) / Segment::kCellCount + 2;
    const size_t segmentWords = (segments + 31) / 32;
    size_t bytes = sizeof(Block) + 4 * 63;
    const size_t counts[] = {amount, segments, nodeWords, segmentWords};
    const size_t widths[] = {
        sizeof(Node), sizeof(Segment), sizeof(std::atomic<uint32_t>), sizeof(std::atomic<uint32_t>)};
    for (size_t i = 0; i < 4; ++i)
    {
        if (counts[i] > (maximum - bytes) / widths[i])
        {
            return nullptr;
        }
        bytes += counts[i] * widths[i];
    }
    increment(m_allocations);
    unsigned char* storage = nullptr;
    try
    {
        storage = new unsigned char[bytes];
    }
    catch (...)
    {
        return nullptr;
    }
    uintptr_t cursor = (reinterpret_cast<uintptr_t>(storage) + 63) & ~uintptr_t(63);
    Block* block = new (reinterpret_cast<void*>(cursor)) Block(amount, initial);
    block->m_storage = storage;
    block->m_bytes = bytes;
    cursor = (cursor + sizeof(Block) + 63) & ~uintptr_t(63);
    block->m_nodes = reinterpret_cast<Node*>(cursor);
    cursor = (cursor + amount * sizeof(Node) + 63) & ~uintptr_t(63);
    block->m_segments = reinterpret_cast<Segment*>(cursor);
    // Reserve storage and initialize allocation maps only. Each exclusive claim
    // starts its objects' lifetimes before publication. A grower need not touch
    // every payload/segment page while other producers wait for usable capacity.
    cursor = (cursor + segments * sizeof(Segment) + 63) & ~uintptr_t(63);
    block->m_nodeBits = reinterpret_cast<std::atomic<uint32_t>*>(cursor);
    for (size_t i = 0; i < nodeWords; ++i)
    {
        const size_t left = amount - i * 32;
        uint32_t mask = left >= 32 ? UINT32_MAX : (uint32_t(1) << left) - 1;
        if (initial && i == 0)
        {
            mask &= ~uint32_t(1);
        }
        new (&block->m_nodeBits[i]) std::atomic<uint32_t>(mask);
    }
    cursor += nodeWords * sizeof(std::atomic<uint32_t>);
    block->m_segmentBits = reinterpret_cast<std::atomic<uint32_t>*>(cursor);
    for (size_t i = 0; i < segmentWords; ++i)
    {
        const size_t left = segments - i * 32;
        const uint32_t mask = left >= 32 ? UINT32_MAX : (uint32_t(1) << left) - 1;
        new (&block->m_segmentBits[i]) std::atomic<uint32_t>(mask);
    }
    return block;
}

template <class T>
void LockFreeQueueArena<T>::deleteBlock(Block* block, bool empty)
{
    // Maintenance proved there are no live payloads before releasing storage.
    // Ending those empty node lifetimes needs no per-node destructor loop.
    // Under external quiescence, every claimed payload bit names a constructed
    // Node (including unused private caches). Free bits may be unconstructed;
    // their former payload, if any, was already destroyed before bit publication.
    if (!empty)
    {
        for (size_t word = 0; word < block->m_nodeWords; ++word)
        {
            uint32_t claimed = ~block->m_nodeBits[word].load(std::memory_order_relaxed);
            if (block->m_initial && word == 0)
            {
                claimed &= ~uint32_t(1);
            }
            while (claimed != 0)
            {
                const uint32_t bit = claimed & (uint32_t(0) - claimed);
                const size_t index = word * 32 + lowBit(bit);
                if (index < block->m_capacity)
                {
                    block->m_nodes[index].~Node();
                }
                claimed ^= bit;
            }
        }
    }
    unsigned char* storage = block->m_storage;
    block->~Block();
    delete[] storage;
}

template <class T>
LockFreeQueueResult LockFreeQueueArena<T>::initialize()
{
    destroy();
    if (!validate(m_config, m_limit))
    {
        return LockFreeQueueInvalidConfiguration;
    }
    m_recordBytes = m_config.maximumProducers * (sizeof(Record) + sizeof(LockFreeQueueAtomic<Block*>)) + 70;
    m_auxiliaryCount = m_config.maximumProducers * 4 + 4;
    m_auxiliaryBytes = m_auxiliaryCount * sizeof(Auxiliary);
    increment(m_allocations);
    try
    {
        m_recordStorage = new unsigned char[m_recordBytes + m_auxiliaryBytes];
    }
    catch (...)
    {
        m_recordBytes = 0;
        return LockFreeQueueAllocationFailure;
    }
    m_records = reinterpret_cast<Record*>((reinterpret_cast<uintptr_t>(m_recordStorage) + 63) & ~uintptr_t(63));
    for (size_t i = 0; i < m_config.maximumProducers; ++i)
    {
        new (&m_records[i]) Record();
    }
    m_traversal = reinterpret_cast<LockFreeQueueAtomic<Block*>*>(m_records + m_config.maximumProducers);
    for (size_t i = 0; i < m_config.maximumProducers; ++i)
    {
        new (&m_traversal[i]) LockFreeQueueAtomic<Block*>(nullptr);
    }
    m_auxiliary = reinterpret_cast<Auxiliary*>(
        (reinterpret_cast<uintptr_t>(m_traversal + m_config.maximumProducers) + 7) & ~uintptr_t(7));
    for (size_t i = 0; i < m_auxiliaryCount; ++i)
    {
        new (&m_auxiliary[i]) Auxiliary();
    }
    m_initial = createBlock(m_config.initialCapacity, true);
    if (m_initial == nullptr)
    {
        destroy();
        return LockFreeQueueAllocationFailure;
    }
    m_capacity.store(m_config.initialCapacity);
    m_availableBase.store(m_config.initialCapacity - 1);
    m_budget.store(m_config.initialCapacity);
    m_peak.store(m_config.initialCapacity);
    m_bytes.store(m_recordBytes + m_auxiliaryBytes + m_initial->m_bytes);
    m_blockCount.store(1);
    m_blocks.store(m_initial);
    m_initial->m_state.store(1);
    if (!atomicsLockFree())
    {
        destroy();
        return LockFreeQueueAtomicUnavailable;
    }
    return LockFreeQueueSuccess;
}

template <class T>
void LockFreeQueueArena<T>::destroy()
{
    Block* block = m_blocks.load(std::memory_order_relaxed);
    while (block != nullptr)
    {
        Block* next = block->m_next.load(std::memory_order_relaxed);
        deleteBlock(block);
        block = next;
    }
    block = m_retiredBlocks;
    while (block != nullptr)
    {
        Block* next = block->m_retiredNext;
        deleteBlock(block);
        block = next;
    }
    if (m_records != nullptr)
    {
        for (size_t i = 0; i < m_config.maximumProducers; ++i)
        {
            m_records[i].~Record();
        }
    }
    if (m_auxiliary != nullptr)
    {
        for (size_t i = 0; i < m_auxiliaryCount; ++i)
        {
            const uintptr_t bits = m_auxiliary[i].m_bits.load(std::memory_order_relaxed);
            delete reinterpret_cast<Segment*>(bits & ~uintptr_t(1));
            m_auxiliary[i].~Auxiliary();
        }
    }
    delete[] m_recordStorage;
    m_records = nullptr;
    m_traversal = nullptr;
    m_recordStorage = nullptr;
    m_recordBytes = 0;
    m_auxiliary = nullptr;
    m_auxiliaryCount = 0;
    m_auxiliaryBytes = 0;
    m_auxiliaryScan = 0;
    m_blocks.store(nullptr);
    m_initial = nullptr;
    m_retiredSegments = nullptr;
    m_retiredSegmentTail = nullptr;
    m_retiredBlocks = nullptr;
    m_retiredBlockTail = nullptr;
    m_scan = nullptr;
    m_previous = nullptr;
    m_recordScan = 0;
    m_maintenanceTurn = 0;
    m_availableBase.store(0);
    m_returned.store(0);
    m_high.store(0);
    m_capacity.store(0);
    m_budget.store(0);
    m_peak.store(0);
    m_retiringCapacity.store(0);
    m_bytes.store(0);
    m_blockCount.store(0);
    m_grows.store(0);
    m_allocations.store(0);
    m_lastGrow.store(0);
    m_segmentSequence.store(0);
}

template <class T>
typename LockFreeQueueArena<T>::Record* LockFreeQueueArena<T>::enter()
{
    if (!initialized())
    {
        return nullptr;
    }
#if defined(_MSC_VER) && _MSC_VER == 1800
    static __declspec(thread) size_t s_hint = 0;
#else
    static thread_local size_t s_hint = 0;
#endif
    const size_t preferred = s_hint < m_high.load(std::memory_order_relaxed) ? s_hint : 0;
    for (size_t step = 0; step < m_config.maximumProducers; ++step)
    {
        const size_t i = step == 0 ? preferred : (step <= preferred ? step - 1 : step);
        uint32_t expected = 0;
        if (m_records[i].m_active.load(std::memory_order_relaxed) == 0 &&
            m_records[i].m_active.compare_exchange_strong(
                expected, 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            size_t high = m_high.load();
            while (high < i + 1 && !m_high.compare_exchange_weak(high, i + 1))
            {

            }
            s_hint = i;
            return &m_records[i];
        }
    }
    return nullptr;
}

template <class T>
void LockFreeQueueArena<T>::leave(Record& record, bool releaseLease, bool retainPublication)
{
    record.m_block.store(nullptr, std::memory_order_release);
    if (!retainPublication)
    {
        record.m_hazard.store(nullptr, std::memory_order_release);
    }
    if (releaseLease)
    {
        record.m_active.store(0, std::memory_order_release);
    }
}

template <class T>
bool LockFreeQueueArena<T>::initialized() const
{
    return m_initial != nullptr;
}

template <class T>
bool LockFreeQueueArena<T>::atomicsLockFree() const
{
    LockFreeQueueAtomic<uintptr_t> publicationBits;
    return publicationBits.is_lock_free() && m_blocks.is_lock_free() && m_returned.is_lock_free() &&
           m_capacity.is_lock_free() && m_grows.is_lock_free() &&
           (m_records == nullptr || (m_records[0].m_active.is_lock_free() && m_records[0].m_hazard.is_lock_free())) &&
           (m_initial == nullptr || (m_initial->m_nodeBits[0].is_lock_free() && m_initial->m_activity.is_lock_free()));
}

template <class T>
bool LockFreeQueueArena<T>::shrinkEnabled() const
{
    return static_cast<bool>(m_shrinkPolicy);
}

template <class T>
bool LockFreeQueueArena<T>::automaticMaintenanceNeeded() const
{
    // Node returns are immediate. Only policy work or a previously detached
    // block needs periodic full maintenance; segment retirement has its own budget.
    return static_cast<bool>(m_shrinkPolicy) || m_retiredBlocks != nullptr;
}

template <class T>
typename LockFreeQueueArena<T>::Block* LockFreeQueueArena<T>::initialBlock() const
{
    return m_initial;
}

template <class T>
size_t LockFreeQueueArena<T>::bitCount(uint32_t bits)
{
    size_t count = 0;
    while (bits != 0)
    {
        bits &= bits - 1;
        ++count;
    }
    return count;
}

template <class T>
size_t LockFreeQueueArena<T>::lowBit(uint32_t bit)
{
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    unsigned long position = 0;
    _BitScanForward(&position, bit);
    return position;
#elif defined(__GNUC__) || defined(__clang__)
    return static_cast<size_t>(__builtin_ctz(bit));
#else
    size_t position = 0;
    while ((bit & 1) == 0)
    {
        ++position;
        bit >>= 1;
    }
    return position;
#endif
}

template <class T>
void LockFreeQueueArena<T>::increment(std::atomic<uint64_t>& value)
{
    uint64_t old = value.load(std::memory_order_relaxed);
    while (old != UINT64_MAX && !value.compare_exchange_weak(old, old + 1, std::memory_order_relaxed))
    {

    }
}

#endif