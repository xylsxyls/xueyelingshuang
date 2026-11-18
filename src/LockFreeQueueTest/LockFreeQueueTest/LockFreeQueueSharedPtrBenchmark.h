#pragma once

#include <cstddef>
#include <cstdint>

// Independent objects allocated before timing, with one FIFO identity field.
struct LockFreeQueueSharedPtrNode
{
    // Producer lane and lane-local sequence encoded before measurement
    uint64_t m_identity;
};

// Test 46: precreated shared_ptr payloads, sufficient fixed pool, one consumer.
// All owners survive the timed phases; no payload allocation/destruction is timed.
class LockFreeQueueSharedPtrBenchmark
{
public:
    /** Run producer-only, prefilled drain, and simultaneous MPSC measurements.
    @param [in] producers Number of concurrent producers, 1..16
    @param [in] items Distinct precreated objects; acceptance uses 20000000
    @param [in] bound Bind one producer context per thread
    @param [in] prefix Consume FIFO prefixes of at most 64 instead of ordinary pop
    @param [in] distributed Enable optional distributed producer counters
    @param [in] reserved Enable optional cancelable reservation publication
    @return Zero only after exact delivery, FIFO and unchanged allocation checks
    */
    static int run(size_t producers, size_t items, bool bound, bool prefix, bool distributed, bool reserved = false);
private:
    /** Monotonic seconds; the first call warms the platform clock before timing
    @return Monotonic timestamp in the documented units
    */
    static double now();

    /** Observe the logical processor at phase boundaries; optional test placement is checked before timing
    @return Current logical processor, or zero on non-Windows hosts
    */
    static uint32_t processor();

    /** Current thread kernel plus user CPU time in 100 ns units; zero if unavailable
    @return Current thread CPU time in 100 ns units, or zero when unavailable
    */
    static uint64_t cpuTicks();
};