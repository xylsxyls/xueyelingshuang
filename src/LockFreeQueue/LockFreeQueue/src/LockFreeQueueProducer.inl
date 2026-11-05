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
void LockFreeQueueProducer<T>::push(const T& value)
{
    const LockFreeQueueResult result =
        m_owner == nullptr ? LockFreeQueueNotInitialized : m_owner->pushImpl(value, true, m_record);
    if (result != LockFreeQueueSuccess)
    {
        throw LockFreeQueueError(result);
    }
}

template <class T>
LockFreeQueueResult LockFreeQueueProducer<T>::tryPush(const T& value)
{
    return m_owner == nullptr ? LockFreeQueueNotInitialized : m_owner->pushImpl(value, false, m_record);
}

#endif