#pragma once

inline uint64_t CTaskReadDomain::current() const
{
    return m_epoch.load(std::memory_order_acquire);
}

inline size_t CTaskReadDomain::indexOf(const CTaskReadPosition& position) const
{
    return static_cast<size_t>(&position - m_positions);
}

inline void CTaskReadDomain::leave(CTaskReadPosition& position)
{
    position.m_epoch.store(0, std::memory_order_release);
}

inline bool CTaskReadDomain::closed() const
{
    return m_closed.load(std::memory_order_acquire);
}

inline size_t CTaskReadDomain::capacity() const
{
    return m_count;
}

inline CTaskReadPosition* CTaskReadDomain::tryEnter(size_t index, bool* contended)
{
    if (index >= m_count || m_closed.load(std::memory_order_seq_cst))
    {
        return nullptr;
    }
    CTaskReadPosition& position = m_positions[index];
    if (position.m_epoch.load(std::memory_order_relaxed) != 0)
    {
        return nullptr;
    }
    uint64_t expected = 0;
    uint64_t epoch = m_epoch.load(std::memory_order_seq_cst);
    if (!position.m_epoch.compare_exchange_strong(expected, epoch, std::memory_order_seq_cst, std::memory_order_relaxed))
    {
        if (contended != nullptr)
        {
            *contended = true;
        }
        return nullptr;
    }
    // 公布后只验证一次，再开始读取受保护地址。维护者若先推进 epoch，
    // 此次 SC 读取会观察到该推进；若后推进，扫描会看到本位置的非零公告。
    // 公告可以保守地留在旧代，不必追赶持续推进的全局 epoch；更新时也不清零。
    const uint64_t observed = m_epoch.load(std::memory_order_seq_cst);
    if (observed != epoch)
    {
        position.m_epoch.store(observed, std::memory_order_seq_cst);
    }
    if (m_closed.load(std::memory_order_seq_cst))
    {
        position.m_epoch.store(0, std::memory_order_release);
        return nullptr;
    }
    return &position;
}

#ifdef _MSC_VER
__forceinline
#else
inline
#endif
CTaskReadPosition* CTaskReadDomain::enter(bool* contended)
{
    if (contended != nullptr)
    {
        *contended = false;
    }
#if defined(_MSC_VER) && _MSC_VER == 1800
    static __declspec(thread) size_t s_hint = 0;
#else
    static thread_local size_t s_hint = 0;
#endif
    const size_t preferred = s_hint < m_count ? s_hint : 0;
    // 常见调用继续使用自己的空闲位置；tryEnter 已完成关闭及 epoch 的完整核对。
    // 命中时不进入轮转扫描，也不重复写入未改变的线程本地提示。
    CTaskReadPosition* preferredPosition = tryEnter(preferred, contended);
    if (preferredPosition != nullptr)
    {
        if (s_hint != preferred)
        {
            s_hint = preferred;
        }
        return preferredPosition;
    }
    return enterFallback(preferred, s_hint, contended);
}