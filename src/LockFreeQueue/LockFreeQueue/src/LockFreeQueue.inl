#ifndef _LOCK_FREE_QUEUE_H__
#define _LOCK_FREE_QUEUE_H__
#include "LockFreeQueue.h"

template<class QElmType>
qnode<QElmType>::qnode():
m_next(nullptr)
{

}

template<class QElmType>
LockFreeQueue<QElmType>::LockFreeQueue()
{
	init();
}

template<class QElmType>
LockFreeQueue<QElmType>::~LockFreeQueue()
{
	destroy();
}

template<class QElmType>
bool LockFreeQueue<QElmType>::init()
{
	m_front = m_rear = new qnode < QElmType > ;
	if (!m_front)
	{
		return false;
	}

	m_front->m_next = nullptr;
	return true;
}

template<class QElmType>
void LockFreeQueue<QElmType>::destroy()
{
	while (m_front)
	{
		m_rear = m_front->m_next;
		delete m_front;
		m_front = m_rear;
	}
}

template<class QElmType>
bool LockFreeQueue<QElmType>::push(const QElmType& e)
{
	struct qnode<QElmType> *p = new qnode < QElmType > ;
	if (!p)
	{
		return false;
	}
	p->m_next = nullptr;
	m_rear->m_next = p;
	m_rear->m_data = e;
	m_rear = p;
	return true;
}

template<class QElmType>
bool LockFreeQueue<QElmType>::pop(QElmType *e)
{
	if (m_front == m_rear)
	{
		return false;
	}

	struct qnode<QElmType> *p = m_front;
	*e = p->m_data;
	m_front = p->m_next;
	delete p;
	return true;
}

#endif