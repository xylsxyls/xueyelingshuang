#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

// Windows scheduling probe. Queue code does not set thread priorities or affinity.
class LockFreeQueueScheduling
{
public:
    /** Run one producer count and API under the runner's inherited affinity. */
    static int run(size_t producers, uint32_t milliseconds, int mode, bool cancelable, size_t capacity = 65536);
private:
    struct Lane
    {
        std::atomic<uint64_t> m_progress, m_completed;
        uint64_t m_count, m_attempts, m_cycles, m_cpu100ns, m_empty, m_capacityFailures;
        uint64_t m_sampledGapNs;
        int m_priority;
        char m_padding[64];
        /** Initialize private results and the single-writer sampled counter. */
        Lane();
    };
    /** Monotonic high resolution timestamp in nanoseconds. */
    static uint64_t now();
    /** Read current thread kernel plus user time, in units of 100 ns. */
    static uint64_t cpu();
    /** Read cycles charged to this thread. */
    static uint64_t cycles();
};