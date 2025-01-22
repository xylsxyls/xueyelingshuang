#pragma once
#include <stdint.h>
#include <atomic>
#include "concurrentqueue.h"

template<class QueueElmentType>
class LockFreeQueue
{
public:
    LockFreeQueue()
    {
        init();
    }

    ~LockFreeQueue()
    {
        destroy();
    }

public:
    void init()
    {
        
    }

    void clear()
    {
		//QueueElmentType item;
		//while (queue.try_dequeue(item));
    }

    void destroy()
    {
        
    }

    void push(const QueueElmentType& e)
    {
        m_queue.enqueue(e);
    }

    bool pop(QueueElmentType* e)
    {
        return m_queue.try_dequeue(*e);
    }

    bool empty()
    {
		return m_queue.size_approx() == 0;
    }

    int32_t size()
    {
        return m_queue.size_approx();
    }

private:
    moodycamel::ConcurrentQueue<QueueElmentType> m_queue;
};
