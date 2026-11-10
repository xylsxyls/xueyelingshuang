#include "CTaskStorage.h"
#include <algorithm>
#include <chrono>
#include <limits>
#include <new>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

CTaskStorage::CTaskStorage(CTaskReadDomain& domain) :
m_domain(&domain),
m_directory(nullptr),
m_counts(nullptr),
m_directoryCount(0),
m_cacheStorage(nullptr),
m_caches(nullptr),
m_high(0),
m_budget(0),
m_directoryTurn(0),
m_grows(0),
m_lastGrow(0),
m_retiredHead(nullptr),
m_retiredTail(nullptr),
m_retiredRead(0),
m_retiredCount(0),
m_retiredOverflow(false),
m_pendingReclaims(nullptr)
{

}

CTaskStorage::~CTaskStorage()
{
    clear();
    if (m_caches != nullptr)
    {
        for (size_t i = 0; i <= m_domain->capacity(); ++i)
        {
            m_caches[i].~CTaskStorageCache();
        }
    }
    delete[] m_cacheStorage;
    delete[] m_directory;
    delete[] m_counts;
}

CTaskThreadResult CTaskStorage::initialize(const LockFreeHashMapPoolConfig& config)
{
    if (m_directory != nullptr || m_cacheStorage != nullptr || m_domain->capacity() == 0 || m_domain->capacity() >= UINT32_MAX ||
        m_domain->capacity() + 1 > ((std::numeric_limits<size_t>::max)() - 63) / sizeof(CTaskStorageCache) || config.directoryCapacity == 0 ||
        config.directoryCapacity > (std::numeric_limits<size_t>::max)() / sizeof(std::atomic<uintptr_t>) ||
        config.maximumCapacity < -1 || (config.maximumCapacity >= 0 && config.initialCapacity > static_cast<uint64_t>(config.maximumCapacity)) ||
        config.initialCapacity / 65535 + (config.initialCapacity % 65535 != 0 ? 1 : 0) > config.directoryCapacity)
    {
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    try
    {
        m_config = config;
        const size_t cacheCount = m_domain->capacity() + 1;
        m_cacheStorage = new unsigned char[cacheCount * sizeof(CTaskStorageCache) + 63];
        CTaskStorageCache* caches = reinterpret_cast<CTaskStorageCache*>(
            (reinterpret_cast<uintptr_t>(m_cacheStorage) + 63) & ~uintptr_t(63));
        size_t constructed = 0;
        try
        {
            // VS2013 x86的非平凡数组析构cookie可能只占4字节；不用new Cache[]取得元素首地址。
            // 类型步长保持自然对齐；显式64字节对齐基址，再逐项构造，避免跨缓存行的原子读写。
            for (; constructed < cacheCount; ++constructed)
            {
                new (&caches[constructed]) CTaskStorageCache();
            }
        }
        catch (...)
        {
            while (constructed != 0)
            {
                caches[--constructed].~CTaskStorageCache();
            }
            delete[] m_cacheStorage;
            m_cacheStorage = nullptr;
            return CTaskThreadResult(CTaskThreadNoMemory);
        }
        m_caches = caches;
        for (size_t i = 0; i <= m_domain->capacity(); ++i)
        {
            m_caches[i].m_ownerIndex = static_cast<uint32_t>(i);
            for (size_t level = 0; level < 16; ++level)
            {
                // 分散到不同块，避免多生产者领取不同页时仍争写同一个块额度。
                // 小池在 scan 内按实际块数取模；已占用的页继续扫描，不等待指定页。
                m_caches[i].m_allocations[level].m_blockHint = i;
                m_caches[i].m_allocations[level].m_pageHint = i;
            }
        }
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
    const size_t start = allocation.m_blockHint % high;
    for (size_t step = 0; step < high; ++step)
    {
        const size_t index = step < high - start ? start + step : step - (high - start);
        const uintptr_t address = m_directory[index].load(std::memory_order_acquire);
        if (address <= 1 || (address & 1) != 0)
        {
            continue;
        }
        CTaskStorageBlock* block = reinterpret_cast<CTaskStorageBlock*>(address);
        // 每个读侧位置至多同时预留一次；低 63 位足以容纳全部位置的 64 槽预留。
        const size_t maximumBatch = block->m_capacity < 256 ? 1 : 64;
        const uint64_t leases = block->m_leases.fetch_add(maximumBatch, std::memory_order_acq_rel);
        if ((leases >> 63) != 0)
        {
            block->m_leases.fetch_sub(maximumBatch, std::memory_order_release);
            continue;
        }
        block->m_touched.store(true, std::memory_order_release);
        const size_t firstPage = allocation.m_pageHint % block->m_pageCount;
        for (size_t pageStep = 0; pageStep < block->m_pageCount; ++pageStep)
        {
            const size_t pageIndex = pageStep < block->m_pageCount - firstPage ? firstPage + pageStep : pageStep - (block->m_pageCount - firstPage);
            CTaskStoragePage& page = block->m_pages[pageIndex];
            uint64_t mask = 0;
            if (maximumBatch == 64)
            {
                // exchange 返回本次实际取得的位，不需要与归还者反复 CAS。
                // 空页只观察，不把整段已占用页的缓存行都改为本核独占。
                if (page.m_free.load(std::memory_order_relaxed) != 0)
                {
                    mask = page.m_free.exchange(0, std::memory_order_acq_rel);
                }
            }
            else
            {
                uint64_t candidates = page.m_free.load(std::memory_order_relaxed);
                while (candidates != 0)
                {
                    const uint64_t bit = candidates & (~candidates + 1);
                    candidates &= ~bit;
                    if ((page.m_free.fetch_and(~bit, std::memory_order_acq_rel) & bit) != 0)
                    {
                        mask = bit;
                        break;
                    }
                    // 一次只检查最初快照中的每个位，失败后不重试同一个候选。
                }
            }
            if (mask != 0)
            {
                const size_t count = bitCount(mask);
                if (count < maximumBatch)
                {
                    block->m_leases.fetch_sub(maximumBatch - count, std::memory_order_release);
                }
                allocation.m_page = &page;
                allocation.m_mask = mask;
                cache.m_cachedCount.store(cache.m_cachedCount.load(std::memory_order_relaxed) + count, std::memory_order_relaxed);
                allocation.m_blockHint = index;
                // 优先复用已经回收的热页；页为空时本次扫描继续前进，不等待该页归还。
                // 失败仍轮转扫描起点，领取过程没有针对同一候选的无限 CAS 重试。
                allocation.m_pageHint = pageIndex;
                return true;
            }
        }
        block->m_leases.fetch_sub(maximumBatch, std::memory_order_release);
    }
    allocation.m_blockHint = start + 1 == high ? 0 : start + 1;
    ++allocation.m_pageHint;
    return false;
}

bool CTaskStorage::refill(CTaskStorageCache& cache, CTaskStorageAllocation& allocation, CTaskThreadResult& result)
{
    if (allocation.m_mask == 0)
    {
        helpReclaim(cache);
    }
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
                    // 新发布的槽可能被其他生产者先领取；已到硬上限时按容量不足返回。
                    result.m_status = m_config.maximumCapacity != -1 &&
                        m_budget.load(std::memory_order_acquire) >= static_cast<uint64_t>(m_config.maximumCapacity) ?
                        CTaskThreadCapacity : CTaskThreadRetry;
                }
                return false;
            }
            result = CTaskThreadResult();
        }
    }
    return true;
}

void CTaskStorage::preGrow()
{
    size_t retained = 0, available = 0;
    observe(retained, available);
    if (available <= m_config.growFreeThreshold)
    {
        // 预扩容失败不撤销已经获得的槽位。
        evaluateGrowth();
    }
}

void CTaskStorage::abandon(CTaskStorageSlot& slot)
{
    slot.m_task.reset();
    slot.m_page->m_publications[slot.index()].store(false, std::memory_order_relaxed);
    slot.m_state->m_bits.store(0, std::memory_order_release);
    slot.m_page->m_free.fetch_or(UINT64_C(1) << slot.index(), std::memory_order_release);
    slot.m_page->m_block->m_leases.fetch_sub(1, std::memory_order_release);
}

size_t CTaskStorage::collect(size_t budget, bool delegate)
{
    if (m_caches != nullptr)
    {
        flushRequested(m_caches[m_domain->capacity()]);
    }
    size_t helped = 0;
    if (!delegate && m_caches != nullptr)
    {
        for (size_t i = 0; i <= m_domain->capacity() && helped < budget; ++i)
        {
            CTaskStorageCache& cache = m_caches[i];
            helped += helpReclaim(cache, budget - helped);
            const size_t staged = (std::min)(cache.m_stagedCount, budget - helped);
            releaseSlots(cache.m_stagedSlots + cache.m_stagedCount - staged, staged);
            cache.m_stagedCount -= staged;
            helped += staged;
        }
    }
    const uint64_t cutoff = m_domain->advance();
    const uint64_t oldest = m_domain->oldest();
    size_t count = helped;
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
                const size_t position = (m_retiredRead + readyCount) & 4095;
                const uint64_t epoch = m_retiredWindow[position].m_epoch;
                if (epoch > cutoff || epoch >= oldest)
                {
                    break;
                }
                ready[readyCount] = m_retiredWindow[position].m_slot;
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
                _mm_prefetch(reinterpret_cast<const char*>(ready[readyCount]), _MM_HINT_T0);
#elif defined(__GNUC__) || defined(__clang__)
                __builtin_prefetch(ready[readyCount], 0, 3);
#endif
            }
            m_retiredRead = (m_retiredRead + readyCount) & 4095;
            m_retiredCount -= readyCount;
        }
        else
        {
            CTaskStorageSlot* next = m_retiredHead;
            while (next != nullptr && readyCount < limit &&
                next->m_cookie <= cutoff && next->m_cookie < oldest)
            {
                ready[readyCount++] = next;
                next = next->m_value;
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
        if (!delegate)
        {
            releaseSlots(ready, readyCount);
        }
        else
        {
            for (size_t i = 0; i < readyCount; ++i)
            {
                CTaskStorageCache& owner = m_caches[ready[i]->m_ownerIndex];
                if (!owner.m_stagedListed)
                {
                    owner.m_stagedListed = true;
                    owner.m_stagedNext = m_pendingReclaims;
                    m_pendingReclaims = &owner;
                }
                owner.m_stagedSlots[owner.m_stagedCount++] = ready[i];
                if (owner.m_stagedCount == 64)
                {
                    publishReclaims(owner);
                }
            }
        }
        count += readyCount;
    }
    CTaskStorageCache** pending = &m_pendingReclaims;
    while (*pending != nullptr)
    {
        CTaskStorageCache& cache = **pending;
        if (delegate && cache.m_stagedCount >= 16)
        {
            publishReclaims(cache);
        }
        if (cache.m_stagedCount == 0)
        {
            *pending = cache.m_stagedNext;
            cache.m_stagedNext = nullptr;
            cache.m_stagedListed = false;
        }
        else
        {
            pending = &cache.m_stagedNext;
        }
    }
    return count;
}

size_t CTaskStorage::helpReclaim(CTaskStorageCache& cache, size_t budget)
{
    size_t released = 0;
    for (size_t i = 0; i < 4 && released < budget; ++i)
    {
        CTaskStorageReclaimBatch& batch = cache.m_reclaims[i];
        int32_t count = batch.m_count.load(std::memory_order_acquire);
        if (count <= 0 || !batch.m_count.compare_exchange_strong(count, -1,
            std::memory_order_acq_rel, std::memory_order_acquire))
        {
            continue;
        }
        const size_t ready = (std::min)(budget - released, static_cast<size_t>(count));
        releaseSlots(batch.m_slots + batch.m_begin, ready);
        batch.m_begin += ready;
        batch.m_count.store(count - static_cast<int32_t>(ready), std::memory_order_release);
        released += ready;
    }
    return released;
}

void CTaskStorage::publishReclaims(CTaskStorageCache& cache)
{
    const size_t count = cache.m_stagedCount;
    if (count == 0)
    {
        return;
    }
    for (size_t i = 0; i < 4; ++i)
    {
        CTaskStorageReclaimBatch& batch = cache.m_reclaims[i];
        int32_t expected = 0;
        if (batch.m_count.load(std::memory_order_relaxed) != 0 ||
            !batch.m_count.compare_exchange_strong(expected, -1, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            continue;
        }
        for (size_t j = 0; j < count; ++j)
        {
            batch.m_slots[j] = cache.m_stagedSlots[j];
        }
        batch.m_begin = 0;
        batch.m_count.store(static_cast<int32_t>(count), std::memory_order_release);
        cache.m_stagedCount = 0;
        return;
    }
    // 四批都在途时直接处理新批次；不等待指定生产者或帮助者。
    releaseSlots(cache.m_stagedSlots, count);
    cache.m_stagedCount = 0;
}

void CTaskStorage::releaseSlots(CTaskStorageSlot* const* slots, size_t count)
{
    CTaskStoragePage* pages[64];
    uint64_t masks[64];
    uint8_t positions[128] = {};
    size_t groups = 0;
    CTaskStoragePage* previousPage = nullptr;
    size_t previousGroup = 0;
    for (size_t i = 0; i < count; ++i)
    {
        CTaskStorageSlot* slot = slots[i];
        CTaskStoragePage* page = slot->m_page;
        const size_t index = slot->index();
        slot->m_task.reset();
        page->m_publications[index].store(false, std::memory_order_relaxed);
        slot->m_state->m_bits.store(0, std::memory_order_release);
        size_t group = previousGroup;
        if (page != previousPage)
        {
            size_t position = (reinterpret_cast<uintptr_t>(page) >> 6) & 127;
            while (positions[position] != 0)
            {
                group = positions[position] - 1;
                if (pages[group] == page)
                {
                    break;
                }
                position = (position + 1) & 127;
            }
            if (positions[position] == 0)
            {
                group = groups++;
                positions[position] = static_cast<uint8_t>(group + 1);
                pages[group] = page;
                masks[group] = 0;
            }
            // 同页相邻退休项直接复用本地分组；跨页仍由完整地址比较处理碰撞。
            previousPage = page;
            previousGroup = group;
        }
        masks[group] |= UINT64_C(1) << index;
    }
    // 输入至多 64 项，128 个本地索引始终有空位；不申请内存或修改共享临时链。
    for (size_t i = 0; i < groups; ++i)
    {
        pages[i]->m_free.fetch_or(masks[i], std::memory_order_release);
        pages[i]->m_block->m_leases.fetch_sub(bitCount(masks[i]), std::memory_order_release);
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
    for (size_t i = 0; i <= m_domain->capacity(); ++i)
    {
        if (m_caches[i].m_cachedCount.load(std::memory_order_relaxed) != 0)
        {
            // 活跃持有者也能看到压力；不能只尝试空闲位置后任其永久缓存。
            m_caches[i].m_flushRequested.store(true, std::memory_order_release);
            // 最后一个缓存归消费者独占，由 collect 响应，生产者不能直接访问其游标。
            if (i == m_domain->capacity())
            {
                continue;
            }
            CTaskReadPosition* position = m_domain->tryEnter(i);
            if (position != nullptr)
            {
                flushRequested(m_caches[i]);
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
            const int32_t phase = slot.m_state->m_bits.load(std::memory_order_acquire) & 3;
            if (phase != 3 && slot.m_page->m_publications[slot.index()].load(std::memory_order_acquire))
            {
                visit(slot);
            }
        }
    }
}

CTaskThreadResult CTaskStorage::grow(size_t requested, bool initial)
{
    // 实际槽位不可能超过地址空间可容纳的 Slot 数；额外留出的数值范围容纳在途申请。
    // 预算使用 64 位，32 位构建的大批并发申请也不会让临时预算回绕。
    const uint64_t physicalMaximum = (std::numeric_limits<size_t>::max)() / sizeof(CTaskStorageSlot);
    const uint64_t maximum = m_config.maximumCapacity < 0 ? physicalMaximum :
        (std::min)(physicalMaximum, static_cast<uint64_t>(m_config.maximumCapacity));
    requested = (std::min)(requested, size_t(65535));
    if (requested == 0)
    {
        return CTaskThreadResult(CTaskThreadPolicyFailure);
    }
    const uint64_t reserved = m_budget.fetch_add(requested, std::memory_order_acq_rel);
    if (reserved >= maximum)
    {
        m_budget.fetch_sub(requested, std::memory_order_release);
        return CTaskThreadResult(CTaskThreadCapacity);
    }
    const size_t count = static_cast<size_t>((std::min)(static_cast<uint64_t>(requested), maximum - reserved));
    if (count != requested)
    {
        m_budget.fetch_sub(requested - count, std::memory_order_release);
    }
    const size_t first = m_directoryTurn.fetch_add(1, std::memory_order_relaxed) % m_directoryCount;
    size_t index = m_directoryCount;
    for (size_t step = 0; step < m_directoryCount; ++step)
    {
        const size_t candidate = step < m_directoryCount - first ? first + step : step - (m_directoryCount - first);
        uintptr_t expected = 0;
        if (m_directory[candidate].compare_exchange_strong(expected, 1, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            index = candidate;
            break;
        }
    }
    if (index == m_directoryCount)
    {
        m_budget.fetch_sub(count, std::memory_order_release);
        return CTaskThreadResult(CTaskThreadCapacity);
    }
    size_t high = m_high.load(std::memory_order_relaxed);
    while (high <= index && !m_high.compare_exchange_strong(high, index + 1, std::memory_order_release, std::memory_order_relaxed))
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
        while (grows != UINT64_MAX && !m_grows.compare_exchange_strong(grows, grows + 1, std::memory_order_relaxed, std::memory_order_relaxed))
        {

        }
        const uint64_t time = now();
        uint64_t previous = m_lastGrow.load(std::memory_order_relaxed);
        while (previous < time && !m_lastGrow.compare_exchange_strong(previous, time, std::memory_order_relaxed, std::memory_order_relaxed))
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
    // 满额优先于策略拒绝或策略异常，不让算法掩盖已经确定的容量不足。
    if (m_config.maximumCapacity != -1 && m_budget.load(std::memory_order_acquire) >= static_cast<uint64_t>(m_config.maximumCapacity))
    {
        return CTaskThreadResult(CTaskThreadCapacity);
    }
    try
    {
        size_t retained = 0, available = 0;
        observe(retained, available);
        const LockFreeHashMapGrowContext context = {m_config.initialCapacity, m_grows.load(std::memory_order_relaxed), retained};
        const size_t count = m_config.growPolicy ? m_config.growPolicy(context) : (std::max)(size_t(64), retained / 2);
        if (m_config.maximumCapacity != -1 && m_budget.load(std::memory_order_acquire) >= static_cast<uint64_t>(m_config.maximumCapacity))
        {
            return CTaskThreadResult(CTaskThreadCapacity);
        }
        return grow(count, false);
    }
    catch (...)
    {
        return CTaskThreadResult(m_config.maximumCapacity != -1 &&
            m_budget.load(std::memory_order_acquire) >= static_cast<uint64_t>(m_config.maximumCapacity) ?
            CTaskThreadCapacity : CTaskThreadPolicyFailure);
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
            if (leases != 0)
            {
                block->m_idleSince = 0;
            }
            else if (block->m_touched.exchange(false, std::memory_order_acq_rel) || block->m_idleSince == 0)
            {
                block->m_idleSince = time;
            }
            else if (time >= block->m_idleSince && time - block->m_idleSince >= decision.idleMilliseconds &&
                block->m_leases.compare_exchange_strong(leases, closed, std::memory_order_acq_rel, std::memory_order_relaxed))
            {
                if (block->m_touched.exchange(false, std::memory_order_acq_rel))
                {
                    // 检查空闲与关闭之间发生了重用。保留晚到的失败登记，不能 store(0) 覆盖其归还。
                    block->m_leases.fetch_and(~closed, std::memory_order_release);
                    block->m_idleSince = time;
                    continue;
                }
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
    m_pendingReclaims = nullptr;
    if (m_caches != nullptr)
    {
        for (size_t i = 0; i <= m_domain->capacity(); ++i)
        {
            for (size_t batch = 0; batch < 4; ++batch)
            {
                m_caches[i].m_reclaims[batch].m_count.store(0, std::memory_order_relaxed);
                m_caches[i].m_reclaims[batch].m_begin = 0;
            }
            m_caches[i].m_stagedCount = 0;
            m_caches[i].m_stagedListed = false;
            m_caches[i].m_stagedNext = nullptr;
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
    const uint64_t physicalMaximum = (std::numeric_limits<size_t>::max)() / sizeof(CTaskStorageSlot);
    const uint64_t maximum = m_config.maximumCapacity < 0 ? physicalMaximum :
        (std::min)(physicalMaximum, static_cast<uint64_t>(m_config.maximumCapacity));
    // 超限申请撤销前的临时账面值不是可用容量，不向外报告超过配置的预算。
    return static_cast<size_t>((std::min)(m_budget.load(std::memory_order_acquire), maximum));
}