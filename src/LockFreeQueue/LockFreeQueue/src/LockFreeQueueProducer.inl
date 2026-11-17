#ifndef LOCK_FREE_QUEUE_PRODUCER_INL
#define LOCK_FREE_QUEUE_PRODUCER_INL
#include "LockFreeQueue.h"

template <class T>
LockFreeQueueProducer<T>::LockFreeQueueProducer() : m_owner(nullptr), m_record(nullptr)
{

}

template <class T>
LockFreeQueueProducer<T>::~LockFreeQueueProducer()
{
    release();
}

template <class T>
bool LockFreeQueueProducer<T>::attached() const
{
    return m_owner != nullptr;
}

template <class T>
void LockFreeQueueProducer<T>::release()
{
    if (m_owner != nullptr)
    {
        m_owner->releaseProducer(*m_record);
    }
    m_owner = nullptr;
    m_record = nullptr;
}

template <class T>
bool LockFreeQueueProducer<T>::push(const T& value)
{
    return tryPush(value, true, true) == LockFreeQueueSuccess;
}

template <class T>
LockFreeQueueResult LockFreeQueueProducer<T>::tryPush(const T& value, bool retryContention, bool retryCapacity)
{
    return m_owner == nullptr ? LockFreeQueueNotInitialized : m_owner->pushRetry(value, retryContention, retryCapacity, m_record);
}

template <class T>
LockFreeQueueResult LockFreeQueueProducer<T>::tryPushAndFence(const T& value, bool retryContention, bool retryCapacity)
{
    const LockFreeQueueResult result = tryPush(value, retryContention, retryCapacity);
    if (result == LockFreeQueueSuccess)
    {
        m_owner->publicationFence();
    }
    return result;
}

template <class T>
LockFreeQueueResult LockFreeQueueProducer<T>::tryPushEntry(LockFreeQueueEntry<T>& entry, bool retryContention, bool retryCapacity)
{
    return m_owner == nullptr ? LockFreeQueueNotInitialized : m_owner->pushEntryRetry(entry, retryContention, retryCapacity, m_record);
}

template <class T>
LockFreeQueueResult LockFreeQueueProducer<T>::tryPushEntryAndFence(LockFreeQueueEntry<T>& entry, bool retryContention, bool retryCapacity)
{
    const LockFreeQueueResult result = tryPushEntry(entry, retryContention, retryCapacity);
    if (result == LockFreeQueueSuccess)
    {
        m_owner->publicationFence();
    }
    return result;
}

#endif