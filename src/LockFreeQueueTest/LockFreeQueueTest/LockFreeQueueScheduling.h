#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

// Windows scheduling probe. Queue code does not set thread priorities or affinity.
class LockFreeQueueScheduling
{
public:
    /** Run one producer count and API under the runner's inherited affinity
    @param [in] producers Producer count, 1..16
    @param [in] milliseconds Active observation window
    @param [in] mode Public API combination, 0..5
    @param [in] cancelable Reservation configuration
    @param [in] capacity Pool capacity, 256, 65536, 1048576 or 20000000
    @param [in] stalledLane Test-only negative control; -1 disabled, producers denotes consumer
    @return 0 passed, 1 invalid data, 3 insufficient progress
    */
    static int run(size_t producers, uint32_t milliseconds, int mode, bool cancelable, size_t capacity = 65536, int32_t stalledLane = -1);
private:
    struct Lane
    {
        std::atomic<uint64_t> m_progress, m_completed;
        uint64_t m_count, m_attempts, m_cycles, m_cpu100ns, m_empty, m_capacityFailures;
        uint64_t m_sampledGapNs;
        // Monitor-owned maximum gap without CPU execution; capacity waits are separate.
        uint64_t m_sampledCpuGapNs;
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