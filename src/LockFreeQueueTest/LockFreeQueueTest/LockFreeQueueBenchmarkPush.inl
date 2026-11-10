#ifndef LOCK_FREE_QUEUE_BENCHMARK_PUSH_INL
#define LOCK_FREE_QUEUE_BENCHMARK_PUSH_INL

template <class Queue, class Value>
int32_t LockFreeQueueBenchmarkPush::submit(Queue& queue, const Value& value)
{
    return execute(queue, value, typename std::is_void<decltype(queue.push(value))>::type());
}

template <class Queue, class Value>
int32_t LockFreeQueueBenchmarkPush::execute(Queue& queue, const Value& value, std::true_type)
{
#ifndef LOCK_FREE_QUEUE_BASELINE
    try
    {
#endif
        queue.push(value);
        return 0;
#ifndef LOCK_FREE_QUEUE_BASELINE
    }
    catch (const LockFreeQueueError& error)
    {
        return error.status();
    }
#endif
}

template <class Queue, class Value>
int32_t LockFreeQueueBenchmarkPush::execute(Queue& queue, const Value& value, std::false_type)
{
    return static_cast<int32_t>(queue.push(value));
}

template <class Queue, class Value>
void LockFreeQueueBenchmarkPush::checked(Queue& queue, const Value& value)
{
    const int32_t result = submit(queue, value);
    if (result != 0)
    {
#ifdef LOCK_FREE_QUEUE_BASELINE
        throw std::runtime_error("Baseline push failed");
#else
        throw LockFreeQueueError(static_cast<LockFreeQueueResult>(result));
#endif
    }
}

template <class Result>
auto LockFreeQueueBenchmarkPush::resourceUnavailable(int32_t result, int)
    -> decltype(Result::LockFreeQueueResourceUnavailable, bool())
{
    return result == Result::LockFreeQueueResourceUnavailable;
}

template <class Result>
bool LockFreeQueueBenchmarkPush::resourceUnavailable(int32_t, long)
{
    return false;
}

inline bool LockFreeQueueBenchmarkPush::transient(int32_t result)
{
#ifdef LOCK_FREE_QUEUE_BASELINE
    (void)result;
    return false;
#else
    return result == LockFreeQueueTemporaryUnavailable || resourceUnavailable<LockFreeQueueResult>(result, 0);
#endif
}

#endif