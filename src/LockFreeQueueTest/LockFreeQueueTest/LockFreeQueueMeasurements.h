#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>

template<class QueueElementType>
class LockFreeQueue;

class LockFreeQueueMeasurements
{
public:
    /** Run benchmark IDs 5/33/35 or backlog ID 22; nonzero means validation failure */
    static int run(int argc, char** argv);
    /** Nanoseconds from a monotonic high resolution source */
    static uint64_t ticks();
    /** Current process user plus kernel CPU time, nanoseconds */
    static uint64_t cpu();
    /** Print process private memory, handle and thread counts */
    static void resources(const char* phase);
    /** Print sorted latency percentiles without hiding sample count */
    static void latency(const char* operation, std::vector<uint64_t>& samples);
    /** Sequential burst workload, independent FIFO verification */
    static bool burst(size_t width, size_t rounds);
    /** Concurrent workload, per-producer sequence validates all successful items */
    static bool concurrent(size_t producers, size_t count);

    /** Measure only producers; capacity includes one private batch per caller
    @param [in] producers Number of enqueue callers
    @param [in] count Values per producer; all values are verified after timing
    @return Whether every value was accepted and drained in per-producer order
    */
    static bool producersOnly(size_t producers, size_t count);
    /** Explicit producer sessions acquired before timing, with identical readiness in the baseline
    @param [in] producers Concurrent callers, each holding a distinct session
    @param [in] count Values per caller; all IDs are independently verified
    @param [in] consume Whether the caller concurrently consumes during timing
    @return Data and no-growth/allocation/rejection invariants
    */
    static bool registered(size_t producers, size_t count, bool consume);
    /** Prepare the full workload capacity and warm both versions before timing
    @param [in,out] queue Externally quiescent queue owned by this measurement
    @param [in] itemCount Maximum simultaneous payload count; space also includes a sentinel
    @param [out] allocationCalls Pool allocation baseline after preparation, zero for the old version
    @return Whether warm-up FIFO and the preallocated resource preconditions passed
    */
    static bool preparePool(LockFreeQueue<uint64_t>& queue, size_t itemCount, uint64_t& allocationCalls);

    /** Verify no pool growth or new allocation occurred in the measured window
    @param [in] queue Queue after all measurement workers have joined
    @param [in] itemCount Maximum simultaneous payload count used for preparation
    @param [in] allocationCalls Pool allocation baseline captured before timing
    @return Whether capacity, growth and allocation invariants hold
    */
    static bool verifyPool(const LockFreeQueue<uint64_t>& queue, size_t itemCount, uint64_t allocationCalls);

    /** Preloaded queue: time pop individually at several backlog sizes */
    static bool backlog(size_t count);
    /** Measure allocation, full-pool rejection and physical shrinking separately */
    static bool poolPaths();
    /** Separate trivial structural work from last-reference payload destruction */
    static bool payloadCost();
};