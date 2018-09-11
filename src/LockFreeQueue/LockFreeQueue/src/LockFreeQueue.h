#pragma once
#include "LockFreeQueueMacro.h"
#include <stdint.h>

template<class QElmType>
struct qnode
{
	qnode();
	struct qnode *m_next;
	QElmType m_data;
};

template<class QElmType>
class LockFreeQueue
{
public:
	LockFreeQueue();
	~LockFreeQueue();

public:
	bool init();
	void destroy();
	bool push(const QElmType& e);
	bool pop(QElmType* e);
	bool empty();
	int32_t size();

private:
	struct qnode<QElmType>* volatile m_front;
	struct qnode<QElmType>* volatile m_rear;
};

#include "LockFreeQueue.inl"