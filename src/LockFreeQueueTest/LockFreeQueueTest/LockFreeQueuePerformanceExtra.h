#pragma once

#include <array>
#include <string>

template<class T>
struct LockFreeQueuePerformanceMessage
{
    uint64_t m_sequence;
    T m_body;
};

// Measurement-only helpers. They do not enter the library or its ABI.
class LockFreeQueuePerformanceExtra
{
public:
    /** Execute sequential, observer, lifecycle and pool-path measurements */
    static bool run();
private:
    /** Begin a measured phase with process CPU and allocation observations */
    static uint64_t begin(uint64_t& cpuStart);
    /** Finish and print a verified phase; duration is for the complete operation */
    static void finish(const char* operation, uint64_t began, uint64_t cpuStart, size_t count, bool valid);
    /** Observe an externally quiescent empty queue with a known record high-water */
    static bool observe(size_t recordCount);
    /** Explicitly measure constructor/destructor, init/destroy and clearing work */
    static bool lifecycle();
    /** Compare status and throwing failures at a real physical limit */
    static bool rejection();
    /** Measure optional record acquisition/release, outside the throughput shortcut */
    static bool registration();
    /** Batched payload copies and output assignment, with independent FIFO IDs */
    template<class T>
    static bool payload(const char* name, const T& body, size_t count);
};