#pragma once
#include <atomic>
#include <stddef.h>
#include <thread>
#include <vector>

// Test-only cleanup for partial thread creation or main-thread exceptions.
class LockFreeQueueThreadGroup
{
public:
    std::vector<std::thread> m_threads;
    /** Stop/release flags outlive this group */
    explicit LockFreeQueueThreadGroup(std::atomic<bool>& stop, std::atomic<bool>* start = nullptr);
    /** Signal cancellation and join every successfully created thread */
    ~LockFreeQueueThreadGroup();
    /** Join without requesting cancellation on a normal completed run */
    void join();
    /** Cancel producers and keep the sole consumer returning capacity until every worker exits.
    @param [in,out] queue Queue with a nonthrowing discard path; caller is its only consumer
    @param [in] completed Each worker increments exactly once after its final queue operation
    */
    template <class Queue>
    void cancelAndDrain(Queue& queue, const std::atomic<size_t>& completed);
private:
    std::atomic<bool>& m_stop;
    std::atomic<bool>* m_start;
    /** One owner for joins */
    LockFreeQueueThreadGroup(const LockFreeQueueThreadGroup& other) = delete;
    /** One owner for joins */
    LockFreeQueueThreadGroup& operator=(const LockFreeQueueThreadGroup& other) = delete;
};
#include "LockFreeQueueThreadGroup.inl"