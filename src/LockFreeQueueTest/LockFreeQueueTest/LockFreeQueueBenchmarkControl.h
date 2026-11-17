#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

// Benchmark-owned shared controls; isolate polling from consumer stack writes.
#if defined(_MSC_VER)
struct __declspec(align(64)) LockFreeQueueBenchmarkControl
#else
struct alignas(64) LockFreeQueueBenchmarkControl
#endif
{
public:
    // Start gate published once after all participants are ready.
    std::atomic<bool> m_start;
    // Cancellation observed by every participant; owned until all have joined.
    std::atomic<bool> m_stop;
    // Any participant may record invalid data or an unrecoverable operation.
    std::atomic<bool> m_failed;
    // Keep the polling flags away from counters and unrelated stack storage.
    char m_flagPadding[64];
    // Number of producers that reached the start gate.
    std::atomic<size_t> m_ready;
    // Number of producers that completed their loop.
    std::atomic<size_t> m_done;
    // Last producer completion timestamp in benchmark nanoseconds.
    std::atomic<uint64_t> m_ended;
    // Total rejected attempts; all retries remain inside the timed interval.
    std::atomic<uint64_t> m_refused;
    // Subset of rejected attempts caused by an actual capacity limit.
    std::atomic<uint64_t> m_capacityRefused;
    // Prevent adjacent consumer-local variables from sharing this storage line.
    char m_endPadding[64];

public:
    /** Initialize a fresh benchmark control block before creating any worker */
    LockFreeQueueBenchmarkControl();
};

#include "LockFreeQueueBenchmarkControl.inl"