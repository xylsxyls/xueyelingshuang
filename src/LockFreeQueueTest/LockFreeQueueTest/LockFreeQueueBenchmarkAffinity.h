#pragma once

#include <cstddef>

// Optional benchmark-only placement; never included by production queue headers.
class LockFreeQueueBenchmarkAffinity
{
public:
    /** Apply the environment's comma-separated logical CPU plan before timing
    @param [in] participant Zero for consumer/main; producer lane plus one otherwise
    @param [in] announce Print the full plan once before workers start
    @return False for an invalid/unsupported plan or an OS placement failure
    */
    static bool pin(size_t participant, bool announce = false);
};

#include "LockFreeQueueBenchmarkAffinity.inl"