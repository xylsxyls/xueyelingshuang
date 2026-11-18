#pragma once

inline void CTaskReadyIndex::fenceAfterRmw()
{
#if defined(_MSC_VER) && !defined(__clang__) && _MSC_VER == 1800 && (defined(_M_X64) || defined(_M_IX86))
    _ReadWriteBarrier();
#else
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
}

inline void CTaskReadyIndex::fence()
{
#if defined(_MSC_VER) && _MSC_VER == 1800 && (defined(_M_X64) || (defined(_M_IX86) && _M_IX86_FP >= 2))
    // v120 的 atomic_thread_fence(SC) 对进程共享的静态 Guard 执行 exchange。
    // 使用当前调用栈上的 Interlocked 全屏障，不让无关线程争用 Guard。
    _ReadWriteBarrier();
    volatile long barrier = 0;
    _InterlockedExchange(&barrier, 1);
    _ReadWriteBarrier();
#else
    std::atomic_thread_fence(std::memory_order_seq_cst);
#endif
}

inline void CTaskReadyIndex::publish(int32_t level, bool fenced)
{
    // 若观察到足够高的旧提示并省略 CAS，此 fence 仍把之前的队列发布
    // 排在消费者成功下调后的复查之前；不能仅用一个普通 acquire load。
    if (!fenced)
    {
        fence();
    }
    int32_t highest = m_highest.load(std::memory_order_seq_cst);
    while (highest < level && !m_highest.compare_exchange_strong(highest, level, std::memory_order_seq_cst, std::memory_order_relaxed))
    {

    }
}

inline int32_t CTaskReadyIndex::highest() const
{
    return m_highest.load(std::memory_order_acquire);
}

inline bool CTaskReadyIndex::lower(int32_t level)
{
    if (level <= 1 || !m_highest.compare_exchange_strong(level, level - 1, std::memory_order_seq_cst, std::memory_order_relaxed))
    {
        return false;
    }
    // 与 publish 的屏障构成顺序，成功后调用方仍必须再检查队列。
    // v120 x86/x64 的成功 CAS 已有完整硬件屏障，避免再对栈执行一次 Interlocked。
    // 其他编译目标继续保留标准 SC 屏障。
    fenceAfterRmw();
    return true;
}