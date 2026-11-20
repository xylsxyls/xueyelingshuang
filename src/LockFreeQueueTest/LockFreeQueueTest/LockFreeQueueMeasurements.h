#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>

template<class QueueElementType>
class LockFreeQueue;

// Standalone workload entrypoints; owns its queues and joins its workers
class LockFreeQueueMeasurements
{
public:
    /** Run benchmark IDs 5/33/35 or backlog ID 22; nonzero means validation failure
    @param [in] argc Argument count including the executable name
    @param [in] argv Non-null argument array borrowed until this call returns
    @return Zero on success; nonzero when arguments or workload validation fail
    */
    static int run(int argc, char** argv);

    /** Nanoseconds from a monotonic high resolution source
    @return Monotonic timestamp in nanoseconds
    */
    static uint64_t ticks();

    /** Current process user plus kernel CPU time, nanoseconds
    @return Kernel plus user CPU time in the documented units
    */
    static uint64_t cpu();

    /** Print process private memory, handle and thread counts
    @param [in] phase Non-null resource sampling label
    */
    static void resources(const char* phase);

    /** Print sorted latency percentiles without hiding sample count
    @param [in] operation Non-null stable metric name, borrowed for this call
    @param [in,out] samples Nanosecond samples, sorted in place for percentile output
    */
    static void latency(const char* operation, std::vector<uint64_t>& samples);

    /** Sequential burst workload, independent FIFO verification
    @param [in] width Items in each sequential fill/drain batch
    @param [in] rounds Number of complete fill/drain batches
    @return Whether every stated invariant passed
    */
    static bool burst(size_t width, size_t rounds);

    /** Concurrent workload, per-producer sequence validates all successful items
    @param [in] producers Number of producer threads; caller supplies a positive count
    @param [in] count Number of items in the requested workload
    @return Whether every stated invariant passed
    */
    static bool concurrent(size_t producers, size_t count);

    /** Measure only producers; capacity includes one private batch per caller
    @param [in] producers Number of enqueue callers
    @param [in] count Values per producer; all values are verified after timing
    @return Whether every value was accepted and drained in per-producer order
    */
    static bool producersOnly(size_t producers, size_t count);

    /** Explicit producer sessions acquired before timing, all producers start after registration
    @param [in] producers Concurrent callers, each holding a distinct session
    @param [in] count Values per caller; all IDs are independently verified
    @param [in] consume Whether the caller concurrently consumes during timing
    @return Data and no-growth/allocation/rejection invariants
    */
    static bool registered(size_t producers, size_t count, bool consume);

    /** Prepare the full workload capacity and warm both versions before timing
    @param [in,out] queue Externally quiescent queue owned by this measurement
    @param [in] itemCount Maximum simultaneous payload count; space also includes a sentinel
    @param [out] allocationCalls Pool allocation baseline after preparation, zero for the optional standard list comparison
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

    /** Preloaded queue: time pop individually at several backlog sizes
    @param [in] count Number of items in the requested workload
    @return Whether every stated invariant passed
    */
    static bool backlog(size_t count);

    /** Measure allocation, full-pool rejection and physical shrinking separately
    @return Whether every stated invariant passed
    */
    static bool poolPaths();

    /** Separate trivial structural work from last-reference payload destruction
    @return Whether every stated invariant passed
    */
    static bool payloadCost();
};