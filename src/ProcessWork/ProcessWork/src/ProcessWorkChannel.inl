#pragma once
#include "ProcessWorkAtomic.h"

#if defined(_MSC_VER)
__forceinline
#else
inline
#endif
bool ProcessWorkChannel::ready() const
{
    return m_directory != nullptr && m_control != nullptr && ProcessWorkAtomic::load(&m_control->m_ready) == 1 &&
        ProcessWorkAtomic::load(&m_directory->m_session) == m_session &&
        static_cast<uint64_t>(ProcessWorkAtomic::load(reinterpret_cast<int64_t*>(&m_directory->m_identity))) == m_identity;
}

inline int32_t ProcessWorkChannel::trySend(const char* data, int32_t length, int32_t type,
    bool retryContention, bool retryCapacity, const std::atomic<bool>* stop)
{
    if (stop != nullptr && stop->load(std::memory_order_acquire))
    {
        return PROCESS_WORK_UNAVAILABLE;
    }
    uint32_t capacity = 0;
    const int32_t result = transmit(data, length, type, retryContention, retryCapacity, capacity, stop);
    if ((result == PROCESS_WORK_BUSY && retryContention) ||
        (retryCapacity && (result == PROCESS_WORK_CAPACITY || result == PROCESS_WORK_RESOURCE)))
    {
        return retrySend(data, length, type, retryContention, retryCapacity, capacity, stop, result);
    }
    return result;
}