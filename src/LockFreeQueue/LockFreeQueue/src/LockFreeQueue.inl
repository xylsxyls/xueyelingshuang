#ifndef _LOCK_FREE_QUEUE_H__
#define _LOCK_FREE_QUEUE_H__
#include "LockFreeQueue.h"
#include "AtomicMath/AtomicMathAPI.h"

template<class QueueElmentType>
QueueNode<QueueElmentType>::QueueNode():
m_next(nullptr)
{

}

template<class QueueElmentType>
LockFreeQueue<QueueElmentType>::LockFreeQueue():
m_front(nullptr),
m_rear(nullptr),
m_count(0)
{
	init();
}

template<class QueueElmentType>
LockFreeQueue<QueueElmentType>::~LockFreeQueue()
{
	destroy();
}

template<class QueueElmentType>
bool LockFreeQueue<QueueElmentType>::init()
{
	m_front = m_rear = new QueueNode<QueueElmentType>;
	if (m_front == nullptr)
	{
		return false;
	}

	m_front->m_next = nullptr;
	return true;
}

template<class QueueElmentType>
void LockFreeQueue<QueueElmentType>::destroy()
{
	while (m_front != nullptr)
	{
		m_rear = m_front->m_next;
		delete m_front;
		m_front = m_rear;
	}
	m_count = 0;
}

template<class QueueElmentType>
bool LockFreeQueue<QueueElmentType>::push(const QueueElmentType& e)
{
	struct QueueNode<QueueElmentType>* p = new QueueNode<QueueElmentType>;
	if (p == nullptr)
	{
		return false;
	}
	while (true)
	{
#if defined _WIN64 || defined __x86_64__
		if (AtomicMath::compareAndSwap((int64_t*)&m_rear->m_next, 0, (int64_t)p))
#else
		if (AtomicMath::compareAndSwap((int32_t*)&m_rear->m_next, 0, (int32_t)p))
#endif
		{
			m_rear->m_data = e;
			m_rear = p;
			++m_count;
			break;
		}
	}
	return true;
}

template<class QueueElmentType>
bool LockFreeQueue<QueueElmentType>::pop(QueueElmentType* e)
{
	while (true)
	{
		if (m_front == m_rear)
		{
			return false;
		}
		struct QueueNode<QueueElmentType>* p = m_front;
#if defined _WIN64 || defined __x86_64__
		if (AtomicMath::compareAndSwap((int64_t*)&m_front, (int64_t)p, (int64_t)m_front->m_next))
#else
		if (AtomicMath::compareAndSwap((int32_t*)&m_front, (int32_t)p, (int32_t)m_front->m_next))
#endif
		{
			*e = p->m_data;
			delete p;
			--m_count;
			break;
		}
	}
	
	return true;
}

template<class QueueElmentType>
bool LockFreeQueue<QueueElmentType>::empty()
{
	return m_front == m_rear;
}

template<class QueueElmentType>
int32_t LockFreeQueue<QueueElmentType>::size()
{
	return m_count;
}

#endif