#include "CTaskStorage.h"
#include <algorithm>
#include <chrono>
#include <limits>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

CTaskStorage::CTaskStorage(CTaskReadDomain& domain) :
m_domain(&domain),
m_directory(nullptr),
m_counts(nullptr),
m_directoryCount(0),
m_caches(nullptr),
m_high(0),
m_budget(0),
m_grows(0),
m_lastGrow(0),
m_retiredHead(nullptr),
m_retiredTail(nullptr),
m_retiredRead(0),
m_retiredCount(0),
m_retiredOverflow(false),
m_dirtyPages(nullptr)
{

}

CTaskStorage::~CTaskStorage()
{
    clear();
    delete[] m_caches;
    delete[] m_directory;
    delete[] m_counts;
}

CTaskThreadResult CTaskStorage::initialize(const LockFreeHashMapPoolConfig& config)
{
    if (m_directory != nullptr || m_domain->capacity() == 0 || config.directoryCapacity == 0 ||
        config.directoryCapacity > (std::numeric_limits<size_t>::max)() / sizeof(std::atomic<uintptr_t>) ||
        config.maximumCapacity < -1 || (config.maximumCapacity >= 0 && config.initialCapacity > static_cast<uint64_t>(config.maximumCapacity)) ||
        config.initialCapacity / 65535 + (config.initialCapacity % 65535 != 0 ? 1 : 0) > config.directoryCapacity)
    {
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    try
    {
        m_config = config;
        m_caches = new CTaskStorageCache[m_domain->capacity() + 1];
        m_directory = new std::atomic<uintptr_t>[config.directoryCapacity];
        m_directoryCount = config.directoryCapacity;
        for (size_t i = 0; i < m_directoryCount; ++i)
        {
            m_directory[i].store(0, std::memory_order_relaxed);
        }
        m_counts = new std::atomic<size_t>[config.directoryCapacity];
        for (size_t i = 0; i < m_directoryCount; ++i)
        {
            m_counts[i].store(0, std::memory_order_relaxed);
        }
        size_t remaining = config.initialCapacity;
        while (remaining != 0)
        {
            const size_t count = (std::min)(remaining, size_t(65535));
            const CTaskThreadResult result = grow(count, true);
            if (result.m_status != CTaskThreadOk)
            {
                clear();
                return result;
            }
            remaining -= count;
        }
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    catch (...)
    {
        clear();
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
}

CTaskStorageCache& CTaskStorage::cache(size_t index)
{
    return m_caches[index];
}

uint64_t CTaskStorage::reservedCount() const
{
    uint64_t result = 0;
    if (m_caches != nullptr)
    {
        for (size_t i = 0; i <= m_domain->capacity(); ++i)
        {
            result += m_caches[i].m_reserved.load(std::memory_order_acquire);
        }
    }
    return result;
}

uint64_t CTaskStorage::publishedCount(int32_t level) const
{
    uint64_t result = 0;
    if (m_caches != nullptr)
    {
        for (size_t i = 0; i <= m_domain->capacity(); ++i)
        {
            result += level == 0 ? m_caches[i].m_published.load(std::memory_order_acquire) : m_caches[i].m_levels[level - 1].load(std::memory_order_acquire);
        }
    }
    return result;
}

size_t CTaskStorage::firstBit(uint64_t bits)
{
#if defined(_MSC_VER) && defined(_M_X64)
    unsigned long index = 0;
    _BitScanForward64(&index, bits);
    return index;
#elif defined(_MSC_VER) && defined(_M_IX86)
    unsigned long index = 0;
    if (_BitScanForward(&index, static_cast<unsigned long>(bits)))
    {
        return index;
    }
    _BitScanForward(&index, static_cast<unsigned long>(bits >> 32));
    return index + 32;
#elif defined(__GNUC__) || defined(__clang__)
    return static_cast<size_t>(__builtin_ctzll(bits));
#else
    size_t index = 0;
    while ((bits & 1) == 0)
    {
        bits >>= 1;
        ++index;
    }
    return index;
#endif
}

size_t CTaskStorage::bitCount(uint64_t bits)
{
    // 固定步数的并行位统计；无需依赖运行机器是否支持 POPCNT 指令。
    bits -= (bits >> 1) & UINT64_C(0x5555555555555555);
    bits = (bits & UINT64_C(0x3333333333333333)) + ((bits >> 2) & UINT64_C(0x3333333333333333));
    bits = (bits + (bits >> 4)) & UINT64_C(0x0f0f0f0f0f0f0f0f);
    return static_cast<size_t>((bits * UINT64_C(0x0101010101010101)) >> 56);
}

bool CTaskStorage::scan(CTaskStorageCache& cache, CTaskStorageAllocation& allocation)
{
    const size_t high = m_high.load(std::memory_order_acquire);
    if (high == 0)
    {
        return false;
    }
    const size_t start = allocation.m_blockHint < high ? allocation.m_blockHint : 0;
    for (size_t step = 0; step < high; ++step)
    {
        const size_t index = start + step < high ? start + step : start + step - high;
        const uintptr_t address = m_directory[index].load(std::memory_order_acquire);
        if (address <= 1 || (address & 1) != 0)
        {
            continue;
        }
        CTaskStorageBlock* block = reinterpret_cast<CTaskStorageBlock*>(address);
        // 每次最多预留 32 槽；小池只预留一个，避免缓存吞掉有限预算。
        const size_t maximumBatch = block->m_capacity < 256 ? 1 : 32;
        uint64_t leases = block->m_leases.load(std::memory_order_relaxed);
        for (;;)
        {
            if ((leases >> 63) != 0 || (leases & UINT64_C(0xffffffff)) > UINT64_C(0xffffffff) - maximumBatch)
            {
                break;
            }
            const uint64_t version = (leases & UINT64_C(0x7fffffff00000000)) == UINT64_C(0x7fffffff00000000) ? 0 : UINT64_C(1) << 32;
            if (block->m_leases.compare_exchange_weak(leases, leases + maximumBatch + version, std::memory_order_acq_rel, std::memory_order_relaxed))
            {
                const size_t firstPage = allocation.m_pageHint < block->m_pageCount ? allocation.m_pageHint : 0;
                for (size_t pageStep = 0; pageStep < block->m_pageCount; ++pageStep)
                {
                    const size_t pageIndex = firstPage + pageStep < block->m_pageCount ? firstPage + pageStep : firstPage + pageStep - block->m_pageCount;
                    CTaskStoragePage& page = block->m_pages[pageIndex];
                    uint64_t available = page.m_free.load(std::memory_order_relaxed);
                    while (available != 0)
                    {
                        uint64_t mask = 0;
                        uint64_t remaining = available;
                        size_t count = 0;
                        while (remaining != 0 && count < maximumBatch)
                        {
                            const uint64_t bit = remaining & (~remaining + 1);
                            mask |= bit;
                            remaining &= ~bit;
                            ++count;
                        }
                        if (page.m_free.compare_exchange_weak(available, available & ~mask, std::memory_order_acq_rel, std::memory_order_relaxed))
                        {
                            if (count < maximumBatch)
                            {
                                block->m_leases.fetch_sub(maximumBatch - count, std::memory_order_release);
                            }
                            allocation.m_page = &page;
                            allocation.m_mask = mask;
                            cache.m_cachedCount.store(cache.m_cachedCount.load(std::memory_order_relaxed) + count, std::memory_order_relaxed);
                            allocation.m_blockHint = index;
                            allocation.m_pageHint = pageIndex + 1 < block->m_pageCount ? pageIndex + 1 : 0;
                            return true;
                        }
                    }
                }
                block->m_leases.fetch_sub(maximumBatch, std::memory_order_release);
                break;
            }
        }
    }
    return false;
}

CTaskStorageSlot* CTaskStorage::acquire(CTaskStorageCache& cache, CTaskThreadResult& result, int32_t level)
{
    result = CTaskThreadResult();
    if (m_directory == nullptr)
    {
        result.m_status = CTaskThreadClosed;
        return nullptr;
    }
    CTaskStorageAllocation& allocation = cache.m_allocations[level > 1 && level <= 16 ? level - 1 : 0];
    if (allocation.m_mask == 0 && !scan(cache, allocation))
    {
        // 归还本调用的其他级别缓存，不让分级缓存造成虚假容量失败。
        flush(cache);
        flushIdleCaches();
        if (!scan(cache, allocation))
        {
            result = evaluateGrowth();
            if (!scan(cache, allocation))
            {
                if (result.m_status == CTaskThreadOk)
                {
                    result.m_status = CTaskThreadRetry;
                }
                return nullptr;
            }
            result = CTaskThreadResult();
        }
    }
    const size_t index = firstBit(allocation.m_mask);
    allocation.m_mask &= ~(UINT64_C(1) << index);
    cache.m_cachedCount.store(cache.m_cachedCount.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);
    CTaskStorageSlot* slot = &allocation.m_page->m_slots[index];
    // 发布者在入队前完整填写 entry；重做只使用独立槽，退休链也先写后用。
    // 此处不重复清零随后必被覆盖的借用字段。
    if (m_config.growFreeThreshold != 0)
    {
        size_t retained = 0, available = 0;
        observe(retained, available);
        if (available <= m_config.growFreeThreshold)
        {
            // 预扩容失败不撤销已经获得的槽位。
            evaluateGrowth();
        }
    }
    return slot;
}

void CTaskStorage::abandon(CTaskStorageSlot& slot)
{
    slot.m_task.reset();
    slot.m_page->m_publications[slot.m_index].store(false, std::memory_order_relaxed);
    slot.m_state->m_flags.store(0, std::memory_order_relaxed);
    slot.m_state->m_phase.store(1, std::memory_order_release);
    slot.m_page->m_free.fetch_or(UINT64_C(1) << slot.m_index, std::memory_order_release);
    slot.m_page->m_block->m_leases.fetch_sub(1, std::memory_order_release);
}

void CTaskStorage::retire(CTaskStorageSlot& slot)
{
    if (!m_retiredOverflow && m_retiredCount < 1024)
    {
        const size_t position = (m_retiredRead + m_retiredCount) & 1023;
        m_retiredWindow[position] = &slot;
        m_retiredEpochs[position] = m_domain->current();
        ++m_retiredCount;
        return;
    }
    m_retiredOverflow = true;
    slot.m_entry.m_cookie = m_domain->current();
    slot.m_entry.m_value = nullptr;
    if (m_retiredTail != nullptr)
    {
        m_retiredTail->m_entry.m_value = &slot;
    }
    else
    {
        m_retiredHead = &slot;
    }
    m_retiredTail = &slot;
}

size_t CTaskStorage::collect(size_t budget)
{
    const uint64_t cutoff = m_domain->advance();
    const uint64_t oldest = m_domain->oldest();
    size_t count = 0;
    while (count < budget)
    {
        CTaskStorageSlot* ready[16];
        size_t readyCount = 0;
        const size_t limit = (std::min)(size_t(16), budget - count);
        const size_t windowCount = (std::min)(limit, m_retiredCount);
        if (windowCount != 0)
        {
            for (; readyCount < windowCount; ++readyCount)
            {
                const size_t position = (m_retiredRead + readyCount) & 1023;
                const uint64_t epoch = m_retiredEpochs[position];
                if (epoch > cutoff || epoch >= oldest)
                {
                    break;
                }
                ready[readyCount] = m_retiredWindow[position];
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
                _mm_prefetch(reinterpret_cast<const char*>(ready[readyCount]), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
                __builtin_prefetch(ready[readyCount], 0, 3);
#endif
            }
            m_retiredRead = (m_retiredRead + readyCount) & 1023;
            m_retiredCount -= readyCount;
        }
        else
        {
            CTaskStorageSlot* next = m_retiredHead;
            while (next != nullptr && readyCount < limit &&
                next->m_entry.m_cookie <= cutoff && next->m_entry.m_cookie < oldest)
            {
                ready[readyCount++] = next;
                next = next->m_entry.m_value;
            }
            m_retiredHead = next;
            if (next == nullptr)
            {
                m_retiredTail = nullptr;
                m_retiredOverflow = false;
            }
        }
        if (readyCount == 0)
        {
            break;
        }
        for (size_t i = 0; i < readyCount; ++i)
        {
            CTaskStorageSlot* slot = ready[i];
            slot->m_task.reset();
            slot->m_page->m_publications[slot->m_index].store(false, std::memory_order_relaxed);
            slot->m_state->m_flags.store(0, std::memory_order_relaxed);
            slot->m_state->m_phase.store(1, std::memory_order_release);
            CTaskStoragePage* page = slot->m_page;
            page->m_pending |= UINT64_C(1) << slot->m_index;
            if (!page->m_dirty)
            {
                page->m_dirty = true;
                page->m_dirtyNext = m_dirtyPages;
                m_dirtyPages = page;
            }
        }
        count += readyCount;
    }
    flushReturns();
    return count;
}

void CTaskStorage::flushReturns()
{
    while (m_dirtyPages != nullptr)
    {
        CTaskStoragePage* page = m_dirtyPages;
        m_dirtyPages = page->m_dirtyNext;
        const uint64_t bits = page->m_pending;
        page->m_pending = 0;
        page->m_dirty = false;
        page->m_dirtyNext = nullptr;
        page->m_free.fetch_or(bits, std::memory_order_release);
        page->m_block->m_leases.fetch_sub(bitCount(bits), std::memory_order_release);
    }
}

void CTaskStorage::flush(CTaskStorageCache& cache)
{
    for (size_t i = 0; i < 16; ++i)
    {
        CTaskStorageAllocation& allocation = cache.m_allocations[i];
        const uint64_t bits = allocation.m_mask;
        if (bits != 0)
        {
            CTaskStoragePage* page = allocation.m_page;
            allocation.m_mask = 0;
            page->m_free.fetch_or(bits, std::memory_order_release);
            page->m_block->m_leases.fetch_sub(bitCount(bits), std::memory_order_release);
        }
    }
    cache.m_cachedCount.store(0, std::memory_order_relaxed);
}

void CTaskStorage::flushIdleCaches()
{
    for (size_t i = 0; i < m_domain->capacity(); ++i)
    {
        if (m_caches[i].m_cachedCount.load(std::memory_order_relaxed) != 0)
        {
            CTaskReadPosition* position = m_domain->tryEnter(i);
            if (position != nullptr)
            {
                flush(m_caches[i]);
                m_domain->leave(*position);
            }
        }
    }
}

size_t CTaskStorage::releaseProducers(CTaskStorageCache& cache)
{
    size_t count = 0;
    for (size_t i = 0; i < 16; ++i)
    {
        if (cache.m_producers[i].attached())
        {
            cache.m_producers[i].release();
            ++count;
        }
    }
    cache.m_producerCount.store(0, std::memory_order_release);
    return count;
}

size_t CTaskStorage::releaseIdleProducers()
{
    size_t count = 0;
    if (m_caches != nullptr)
    {
        for (size_t i = 0; i < m_domain->capacity(); ++i)
        {
            if (m_caches[i].m_producerCount.load(std::memory_order_relaxed) != 0)
            {
                CTaskReadPosition* position = m_domain->tryEnter(i);
                if (position != nullptr)
                {
                    count += releaseProducers(m_caches[i]);
                    m_domain->leave(*position);
                }
            }
        }
    }
    return count;
}

void CTaskStorage::forEach(const std::function<void(CTaskStorageSlot&)>& visit)
{
    const size_t high = m_high.load(std::memory_order_acquire);
    for (size_t i = 0; i < high; ++i)
    {
        const uintptr_t address = m_directory[i].load(std::memory_order_acquire);
        if (address <= 1 || (address & 1) != 0)
        {
            continue;
        }
        CTaskStorageBlock* block = reinterpret_cast<CTaskStorageBlock*>(address);
        for (size_t j = 0; j < block->m_capacity; ++j)
        {
            CTaskStorageSlot& slot = block->m_pages[j / 64].m_slots[j & 63];
            const int32_t phase = slot.m_state->m_phase.load(std::memory_order_acquire);
            if ((phase == 1 || phase == 2) && slot.m_page->m_publications[slot.m_index].load(std::memory_order_acquire))
            {
                visit(slot);
            }
        }
    }
}

CTaskThreadResult CTaskStorage::grow(size_t requested, bool initial)
{
    const size_t maximum = m_config.maximumCapacity < 0 ? (std::numeric_limits<size_t>::max)() : static_cast<size_t>((std::min)(static_cast<uint64_t>(m_config.maximumCapacity), static_cast<uint64_t>((std::numeric_limits<size_t>::max)())));
    requested = (std::min)(requested, size_t(65535));
    if (requested == 0)
    {
        return CTaskThreadResult(CTaskThreadPolicyFailure);
    }
    size_t reserved = m_budget.load(std::memory_order_acquire);
    size_t count = 0;
    do
    {
        if (reserved >= maximum)
        {
            return CTaskThreadResult(CTaskThreadCapacity);
        }
        count = (std::min)(requested, maximum - reserved);
    } while (!m_budget.compare_exchange_weak(reserved, reserved + count, std::memory_order_acq_rel, std::memory_order_acquire));
    size_t index = 0;
    for (; index < m_directoryCount; ++index)
    {
        uintptr_t expected = 0;
        if (m_directory[index].compare_exchange_strong(expected, 1, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            break;
        }
    }
    if (index == m_directoryCount)
    {
        m_budget.fetch_sub(count, std::memory_order_release);
        return CTaskThreadResult(CTaskThreadCapacity);
    }
    size_t high = m_high.load(std::memory_order_relaxed);
    while (high <= index && !m_high.compare_exchange_weak(high, index + 1, std::memory_order_release, std::memory_order_relaxed))
    {

    }
    CTaskStorageBlock* block = nullptr;
    try
    {
        block = new CTaskStorageBlock();
        if (!block->initialize(count))
        {
            delete block;
            m_directory[index].store(0, std::memory_order_release);
            m_budget.fetch_sub(count, std::memory_order_release);
            return CTaskThreadResult(CTaskThreadNoMemory);
        }
    }
    catch (...)
    {
        delete block;
        m_directory[index].store(0, std::memory_order_release);
        m_budget.fetch_sub(count, std::memory_order_release);
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    m_counts[index].store(count, std::memory_order_relaxed);
    m_directory[index].store(reinterpret_cast<uintptr_t>(block), std::memory_order_release);
    if (!initial)
    {
        uint64_t grows = m_grows.load(std::memory_order_relaxed);
        while (grows != UINT64_MAX && !m_grows.compare_exchange_weak(grows, grows + 1, std::memory_order_relaxed, std::memory_order_relaxed))
        {

        }
        const uint64_t time = now();
        uint64_t previous = m_lastGrow.load(std::memory_order_relaxed);
        while (previous < time && !m_lastGrow.compare_exchange_weak(previous, time, std::memory_order_relaxed, std::memory_order_relaxed))
        {

        }
    }
    return CTaskThreadResult();
}

void CTaskStorage::observe(size_t& retained, size_t& available) const
{
    retained = 0;
    available = 0;
    for (size_t i = 0; i <= m_domain->capacity(); ++i)
    {
        available += m_caches[i].m_cachedCount.load(std::memory_order_relaxed);
    }
    for (size_t i = 0; i < m_high.load(std::memory_order_acquire); ++i)
    {
        const uintptr_t address = m_directory[i].load(std::memory_order_acquire);
        if (address > 1)
        {
            retained += m_counts[i].load(std::memory_order_relaxed);
            if ((address & 1) == 0)
            {
                CTaskStorageBlock* block = reinterpret_cast<CTaskStorageBlock*>(address);
                for (size_t j = 0; j < block->m_pageCount; ++j)
                {
                    available += bitCount(block->m_pages[j].m_free.load(std::memory_order_relaxed));
                }
            }
        }
    }
}

CTaskThreadResult CTaskStorage::evaluateGrowth()
{
    try
    {
        size_t retained = 0, available = 0;
        observe(retained, available);
        const LockFreeHashMapGrowContext context = {m_config.initialCapacity, m_grows.load(std::memory_order_relaxed), retained};
        const size_t count = m_config.growPolicy ? m_config.growPolicy(context) : (std::max)(size_t(64), retained / 2);
        return grow(count, false);
    }
    catch (...)
    {
        return CTaskThreadResult(CTaskThreadPolicyFailure);
    }
}

uint64_t CTaskStorage::now()
{
    try
    {
        const uint64_t time = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
        return time == 0 ? 1 : time;
    }
    catch (...)
    {
        return 0;
    }
}

CTaskThreadResult CTaskStorage::maintain()
{
    if (!m_config.shrinkPolicy)
    {
        return CTaskThreadResult();
    }
    flushReturns();
    const uint64_t cutoff = m_domain->advance();
    const uint64_t oldest = m_domain->oldest();
    LockFreeHashMapShrinkDecision decision;
    size_t retained = 0, available = 0;
    observe(retained, available);
    const uint64_t time = now();
    if (m_config.shrinkPolicy && time != 0)
    {
        try
        {
            flushIdleCaches();
            flush(m_caches[m_domain->capacity()]);
            observe(retained, available);
            const uint64_t last = m_lastGrow.load(std::memory_order_relaxed);
            const LockFreeHashMapShrinkContext context = {m_config.initialCapacity, m_grows.load(std::memory_order_relaxed), retained,
                available, last != 0, last != 0 && time >= last ? time - last : 0};
            decision = m_config.shrinkPolicy(context);
        }
        catch (...)
        {
            return CTaskThreadResult(CTaskThreadPolicyFailure);
        }
    }
    const uint64_t closed = UINT64_C(1) << 63;
    // 已关闭但仍受旧读者保护的块已安排释放，不能再次计入可缩容量。
    for (size_t i = 0; i < m_high.load(std::memory_order_acquire); ++i)
    {
        const uintptr_t address = m_directory[i].load(std::memory_order_acquire);
        if (address > 1 && (address & 1) != 0)
        {
            const size_t count = m_counts[i].load(std::memory_order_relaxed);
            retained = count <= retained ? retained - count : 0;
        }
    }
    // 优先尝试较晚发布的块，避免先释放小初始块后无法整块退还扩容量。
    for (size_t remaining = m_high.load(std::memory_order_acquire); remaining != 0; --remaining)
    {
        const size_t i = remaining - 1;
        uintptr_t address = m_directory[i].load(std::memory_order_acquire);
        if (address <= 1)
        {
            continue;
        }
        CTaskStorageBlock* block = reinterpret_cast<CTaskStorageBlock*>(address & ~uintptr_t(1));
        uint64_t leases = block->m_leases.load(std::memory_order_acquire);
        if ((leases & closed) == 0 && decision.idleMilliseconds != 0 && block->m_capacity <= decision.shrinkCount &&
            retained >= m_config.initialCapacity && block->m_capacity <= retained - m_config.initialCapacity)
        {
            if ((leases & UINT64_C(0xffffffff)) != 0 || (leases & UINT64_C(0x7fffffff00000000)) == UINT64_C(0x7fffffff00000000))
            {
                block->m_idleSince = 0;
            }
            else if (block->m_idleSince == 0 || block->m_idleLease != leases)
            {
                block->m_idleSince = time;
                block->m_idleLease = leases;
            }
            else if (time >= block->m_idleSince && time - block->m_idleSince >= decision.idleMilliseconds &&
                block->m_leases.compare_exchange_strong(leases, closed, std::memory_order_acq_rel, std::memory_order_relaxed))
            {
                block->m_retiredEpoch = m_domain->current();
                m_directory[i].store(address | 1, std::memory_order_release);
                decision.shrinkCount -= block->m_capacity;
                retained -= block->m_capacity;
                leases = closed;
            }
        }
        if (leases == closed && block->m_retiredEpoch <= cutoff && block->m_retiredEpoch < oldest)
        {
            const size_t count = block->m_capacity;
            m_directory[i].store(0, std::memory_order_release);
            delete block;
            m_budget.fetch_sub(count, std::memory_order_release);
        }
    }
    return CTaskThreadResult();
}

void CTaskStorage::clear()
{
    m_retiredHead = nullptr;
    m_retiredTail = nullptr;
    m_retiredRead = 0;
    m_retiredCount = 0;
    m_retiredOverflow = false;
    m_dirtyPages = nullptr;
    if (m_caches != nullptr)
    {
        for (size_t i = 0; i <= m_domain->capacity(); ++i)
        {
            releaseProducers(m_caches[i]);
            for (size_t level = 0; level < 16; ++level)
            {
                m_caches[i].m_allocations[level].m_page = nullptr;
                m_caches[i].m_allocations[level].m_mask = 0;
            }
            m_caches[i].m_cachedCount.store(0, std::memory_order_relaxed);
        }
    }
    for (size_t i = 0; i < m_directoryCount; ++i)
    {
        const uintptr_t address = m_directory[i].load(std::memory_order_relaxed);
        if (address > 1)
        {
            delete reinterpret_cast<CTaskStorageBlock*>(address & ~uintptr_t(1));
        }
        m_directory[i].store(0, std::memory_order_relaxed);
    }
    m_high.store(0, std::memory_order_relaxed);
    m_budget.store(0, std::memory_order_relaxed);
}

size_t CTaskStorage::capacity() const
{
    return m_budget.load(std::memory_order_acquire);
}