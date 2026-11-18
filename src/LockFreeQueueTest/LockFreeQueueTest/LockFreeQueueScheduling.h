#pragma once

#include "LockFreeQueueEntry.h"
#include <atomic>
#include <cstddef>
#include <cstdint>

// Per-thread results; monitor reads only atomics and its own gap fields until join
struct LockFreeQueueSchedulingLane
{
public:
    // Sampled successful transfers and returned calls during the active window
    std::atomic<uint64_t> m_progress, m_completed;
    // Worker-owned totals, read by the coordinator only after join
    uint64_t m_count, m_attempts, m_cycles, m_cpu100ns, m_empty, m_capacityFailures;
    // Monitor-owned maximum interval without a successful transfer, nanoseconds
    uint64_t m_sampledGapNs;
    // Monitor-owned maximum interval without CPU execution, nanoseconds
    uint64_t m_sampledCpuGapNs;
    // OS priority recorded before the worker announces readiness
    int m_priority;
    // Separate adjacent single-writer results to reduce false sharing
    char m_padding[64];
public:
    /** Initialize counters before creating participating threads
    */
    LockFreeQueueSchedulingLane();
};

// Test-owned entry ring cell; queue borrows the entry until the consumer releases it
struct LockFreeQueueSchedulingSlot
{
public:
    // Payload and publication state; storage outlives the queue
    LockFreeQueueEntry<uint64_t> m_entry;
    // Consumer release / producer acquire permits the next payload write
    std::atomic<bool> m_free;
public:
    /** Make the unused entry available before workers start
    */
    LockFreeQueueSchedulingSlot();
};

// Windows scheduling probe. Queue code does not set thread priorities or affinity.
class LockFreeQueueScheduling
{
public:
    /** Run one producer count and API under the runner's inherited affinity
    @param [in] producers Producer count, 1..16
    @param [in] milliseconds Active observation window in milliseconds, 100..60000
    @param [in] mode Public API combination, 0..7; 6 is EntrySpan and 7 ordinary push, both with interval65536
    @param [in] cancelable Reservation configuration
    @param [in] capacity Pool capacity, 256, 65536, 1048576 or 20000000
    @param [in] stalledLane Test-only negative control; -1 disabled, producers denotes consumer
    @return 0 passed, 1 invalid data, 2 invalid/unsupported configuration, 3 insufficient progress
    */
    static int run(size_t producers, uint32_t milliseconds, int mode, bool cancelable, size_t capacity = 65536, int32_t stalledLane = -1);
private:
    typedef LockFreeQueueSchedulingLane Lane;
    typedef LockFreeQueueSchedulingSlot Slot;

    /** Monotonic high resolution timestamp in nanoseconds
    @return Monotonic timestamp in the documented units
    */
    static uint64_t now();

    /** Read current thread kernel plus user time, in units of 100 ns
    @return Kernel plus user CPU time in the documented units
    */
    static uint64_t cpu();

    /** Read cycles charged to this thread
    @return Cycles charged to the calling thread
    */
    static uint64_t cycles();
};