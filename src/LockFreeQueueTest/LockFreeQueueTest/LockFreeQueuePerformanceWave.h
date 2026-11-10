#pragma once

// ID40: finite growth with normal bounded maintenance and an independent FIFO oracle.
class LockFreeQueuePerformanceWave
{
public:
    /** Run 750 initial nodes, 1000-item peaks and 500-item troughs with one consumer
    @return Every sequence, phase boundary and capacity invariant passed
    */
    static bool run();

private:
    /** Repeat rise, concurrent high traffic and concurrent low traffic
    @param [in] producers Producer thread count
    @param [in] threshold Free-node proactive-growth threshold
    @return Complete FIFO and physical-capacity checks passed
    */
    static bool scenario(size_t producers, size_t threshold);
};