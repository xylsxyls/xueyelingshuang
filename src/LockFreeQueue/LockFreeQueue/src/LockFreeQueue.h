#pragma once
#include "LockFreeQueueMacro.h"
#include <stdint.h>
#include <atomic>

template<class QueueElmentType>
struct QueueNode
{
	QueueNode();
	struct QueueNode* m_next;
	QueueElmentType m_data;
};

template<class QueueElmentType>
class LockFreeQueue
{
public:
	LockFreeQueue();
	~LockFreeQueue();

public:
	bool init();
	void destroy();
	bool push(const QueueElmentType& e);
	bool pop(QueueElmentType* e);
	bool empty();
	int32_t size();

private:
	struct QueueNode<QueueElmentType>* volatile m_front;
	struct QueueNode<QueueElmentType>* volatile m_rear;
	std::atomic<int32_t> m_count;
};

#include "LockFreeQueue.inl"