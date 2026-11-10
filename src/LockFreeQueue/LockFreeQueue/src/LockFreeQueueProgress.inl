#ifndef LOCK_FREE_QUEUE_PROGRESS_INL
#define LOCK_FREE_QUEUE_PROGRESS_INL

inline LockFreeQueueProgress::LockFreeQueueProgress() :
m_operations(0), m_state(kWaiting)
{

}

inline void LockFreeQueueProgress::begin(bool wait)
{
    m_state = wait ? kWaiting : 0;
}

inline bool LockFreeQueueProgress::retry(uint8_t attemptBudget)
{
    uint16_t failures = m_state & kFailureMask;
    if ((m_state & kWaiting) == 0 && failures >= attemptBudget)
    {
        m_state |= kInterrupted;
        return false;
    }
    failures = (failures + 1) & kFailureMask;
    m_state = static_cast<uint16_t>((m_state & ~kFailureMask) | failures);
    if ((failures & 15) == 0)
    {
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
        if (failures == 0)
        {
            // yield 是调度提示；正时长休眠使持续竞争者真正暂离就绪队列。
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
        else
#endif
        {
            std::this_thread::yield();
        }
    }
    return true;
}

inline void LockFreeQueueProgress::interrupt()
{
    m_state |= kInterrupted;
}

inline bool LockFreeQueueProgress::interrupted() const
{
    return (m_state & kInterrupted) != 0;
}

inline void LockFreeQueueProgress::resume()
{
    m_state &= static_cast<uint16_t>(~kInterrupted);
}

inline bool LockFreeQueueProgress::waiting() const
{
    return (m_state & kWaiting) != 0;
}

inline void LockFreeQueueProgress::advance(size_t count)
{
    if (count >= 4096 || m_operations >= 4096 - count)
    {
        m_operations = 0;
        std::this_thread::yield();
    }
    else
    {
        m_operations = static_cast<uint16_t>(m_operations + count);
    }
}

#endif