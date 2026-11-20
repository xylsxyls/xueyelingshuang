#ifndef LOCK_FREE_QUEUE_BENCHMARK_PUSH_INL
#define LOCK_FREE_QUEUE_BENCHMARK_PUSH_INL

template <class Queue, class Value>
int32_t LockFreeQueueBenchmarkPush::submit(Queue& queue, const Value& value)
{
#ifdef LOCK_FREE_QUEUE_LIST_MUTEX
    queue.push(value);
    return 0;
#else
    return queue.push(value) ? 0 : -1;
#endif
}

template <class Queue, class Value>
void LockFreeQueueBenchmarkPush::checked(Queue& queue, const Value& value)
{
    const int32_t result = submit(queue, value);
    if (result != 0)
    {
        throw std::runtime_error("Queue push returned false");
    }
}

inline bool LockFreeQueueBenchmarkPush::transient(int32_t result)
{
#ifdef LOCK_FREE_QUEUE_LIST_MUTEX
    (void)result;
    return false;
#else
    return result == LockFreeQueueBusy || result == LockFreeQueueResourceUnavailable;
#endif
}

#endif