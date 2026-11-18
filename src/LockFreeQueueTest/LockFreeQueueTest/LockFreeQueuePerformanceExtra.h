#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

// Test-owned payload with an independent FIFO identity
template<class T>
struct LockFreeQueuePerformanceMessage
{
    // Sequence checked independently of the payload representation
    uint64_t m_sequence;
    // Copied payload whose cost is measured
    T m_body;
};

// Measurement-only helpers. They do not enter the library or its ABI.
class LockFreeQueuePerformanceExtra
{
public:
    /** Execute sequential, observer, lifecycle and pool-path measurements
    @return True only if every workload oracle passes
    */
    static bool run();
private:
    /** Begin a measured phase with process CPU and allocation observations
    @param [out] cpuStart Process CPU timestamp captured before timing, nanoseconds
    @return Monotonic start timestamp in nanoseconds
    */
    static uint64_t begin(uint64_t& cpuStart);

    /** Finish and print a verified phase; duration is for the complete operation
    @param [in] operation Non-null stable metric name, borrowed for this call
    @param [in] began Monotonic start timestamp, nanoseconds
    @param [in] cpuStart Process CPU start timestamp, nanoseconds
    @param [in] count Number of items in the requested workload
    @param [in] valid Independent workload oracle result
    */
    static void finish(const char* operation, uint64_t began, uint64_t cpuStart, size_t count, bool valid);

    /** Observe an externally quiescent empty queue with a known record high-water
    @param [in] recordCount Producer-record high-water mark to establish before timing
    @return Whether every stated invariant passed
    */
    static bool observe(size_t recordCount);

    /** Explicitly measure constructor/destructor, init/destroy and clearing work
    @return Whether every stated invariant passed
    */
    static bool lifecycle();

    /** Measure nonwaiting tryPush refusal at a real physical limit
    @return Whether every stated invariant passed
    */
    static bool rejection();

    /** Measure optional record acquisition/release, outside the throughput shortcut
    @return Whether every stated invariant passed
    */
    static bool registration();

    /** Batched payload copies and output assignment, with independent FIFO IDs
    @param [in] name Non-null metric name, borrowed for this call
    @param [in] body Payload copied into each measured item
    @param [in] count Number of items in the requested workload
    @return Whether every stated invariant passed
    */
    template<class T>
    static bool payload(const char* name, const T& body, size_t count);
};