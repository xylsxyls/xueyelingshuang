#pragma once

template <class T>
LockFreeQueueEntry<T>::LockFreeQueueEntry(const T& value) :
m_value(value), m_cookie(0), m_owner(nullptr)
{

}