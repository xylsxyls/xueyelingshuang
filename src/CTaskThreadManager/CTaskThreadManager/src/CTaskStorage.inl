#pragma once

#ifdef _MSC_VER
__forceinline
#else
inline
#endif
void CTaskStorage::retire(CTaskStorageSlot& slot)
{
    if (!m_retiredOverflow && m_retiredCount < 4096)
    {
        const size_t position = (m_retiredRead + m_retiredCount) & 4095;
        m_retiredWindow[position].m_slot = &slot;
        m_retiredWindow[position].m_epoch = m_domain->current();
        ++m_retiredCount;
        return;
    }
    m_retiredOverflow = true;
    slot.m_cookie = m_domain->current();
    slot.m_value = nullptr;
    if (m_retiredTail != nullptr)
    {
        m_retiredTail->m_value = &slot;
    }
    else
    {
        m_retiredHead = &slot;
    }
    m_retiredTail = &slot;
}

inline CTaskStorageCache& CTaskStorage::cache(size_t index)
{
    return m_caches[index];
}

inline void CTaskStorage::flushRequested(CTaskStorageCache& cache)
{
    if (cache.m_flushRequested.load(std::memory_order_acquire) &&
        cache.m_flushRequested.exchange(false, std::memory_order_acq_rel))
    {
        flush(cache);
    }
}

inline size_t CTaskStorage::firstBit(uint64_t bits)
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

#ifdef _MSC_VER
__forceinline
#else
inline
#endif
CTaskStorageSlot* CTaskStorage::acquire(CTaskStorageCache& cache, CTaskThreadResult& result, int32_t level)
{
    // 每次领取重新填写全部输出，热路径不调用跨翻译单元的结果构造函数。
    result.m_status = CTaskThreadOk;
    result.m_committed = false;
    result.m_nativeError = 0;
    if (m_directory == nullptr)
    {
        result.m_status = CTaskThreadClosed;
        return nullptr;
    }
    flushRequested(cache);
    CTaskStorageAllocation& allocation = cache.m_allocations[level > 1 && level <= 16 ? level - 1 : 0];
    if (allocation.m_mask == 0 && !refill(cache, allocation, result))
    {
        return nullptr;
    }
    const size_t index = firstBit(allocation.m_mask);
    allocation.m_mask &= ~(UINT64_C(1) << index);
    cache.m_cachedCount.store(cache.m_cachedCount.load(std::memory_order_relaxed) - 1, std::memory_order_relaxed);
    CTaskStorageSlot* slot = &allocation.m_page->m_slots[index];
    slot->m_ownerIndex = cache.m_ownerIndex;
    // 发布者在入队前完整填写 entry；重做只使用独立槽，退休链也先写后用。
    // 此处不重复清零随后必被覆盖的借用字段。
    if (m_config.growFreeThreshold != 0)
    {
        preGrow();
    }
    return slot;
}