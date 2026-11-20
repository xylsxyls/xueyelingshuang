#ifndef LOCK_FREE_QUEUE_PROGRESS_INL
#define LOCK_FREE_QUEUE_PROGRESS_INL

inline LockFreeQueueProgress::LockFreeQueueProgress() :
m_remaining(4096), m_interval(4096), m_state(kWaiting)
{

}

inline void LockFreeQueueProgress::configureInterval(uint16_t operations)
{
    m_remaining = operations;
    m_interval = operations;
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
            // Reuse the exception-isolated sleep path: scheduler failure must
            // not escape an enqueue or leave a partially acquired resource.
            pausePublisher();
        }
        else
#endif
        {
            std::this_thread::yield();
        }
    }
    return true;
}

inline bool LockFreeQueueProgress::waitCapacity(uint32_t& retries, uint32_t retriesBeforeSleep, uint32_t sleepMilliseconds)
{
    if (retries < retriesBeforeSleep)
    {
        ++retries;
        // 容量只能由消费或回收释放；密集重试也要给这些线程运行机会。
        if ((retries & 15) == 0)
        {
            std::this_thread::yield();
        }
        return true;
    }
#if defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__)
    (void)sleepMilliseconds;
    return false;
#else
    try
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilliseconds));
        return true;
    }
    catch (...)
    {
        return false;
    }
#endif
}

#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
inline void LockFreeQueueProgress::pausePublisher()
{
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    try
    {
        const std::chrono::microseconds quantum(1);
        std::this_thread::sleep_for(quantum);
    }
    catch (...)
    {
        std::this_thread::yield();
    }
#else
    std::this_thread::yield();
#endif
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
    if (count == 1)
    {
        // The common one-operation path is a decrement and zero test.
        // The constructor and every slow path keep the counter in 1..m_interval.
        if (--m_remaining == 0)
        {
            yieldOperations();
        }
    }
    else if (count >= m_remaining)
    {
        // Compare before narrowing: even SIZE_MAX requests exactly one yield,
        // discarding the excess just as the original accumulated budget did.
        yieldOperations();
    }
    else
    {
        m_remaining = static_cast<uint16_t>(m_remaining - count);
    }
}

#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
inline void LockFreeQueueProgress::yieldOperations()
{
    m_remaining = m_interval;
    std::this_thread::yield();
}

#endif