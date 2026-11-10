#pragma once

template<class T>
class LockFreeQueueProducer;

// Measurement-only workloads; no hooks or interfaces enter production builds.
class LockFreeQueuePerformancePools
{
public:
    /** Run ID38: growth, reuse, shrink/regrow, and bounded producer pressure
    @return All independent data, capacity and recovery oracles passed
    */
    static bool run();

private:
    /** Submit through the selected public interface without changing ownership
    @param [in,out] queue Live queue, owned by the scenario
    @param [in] value Independent sequence ID
    @param [in] producer Borrowed session for context variants, otherwise null
    @return Zero on success; current library status on refusal
    */
    static int32_t submit(LockFreeQueue<uint64_t>& queue, uint64_t value,
        LockFreeQueueProducer<uint64_t>* producer);

    /** Fill/drain an initially small pool, reuse it, optionally shrink and regrow
    @param [in] name Stable scenario name
    @param [in] threshold Free-node growth threshold
    @param [in] growth 0 default doubling, 1 fixed256, 2 proportional half, 3 fixed512
    @param [in] shrink Whether to require real physical release between rounds
    @return FIFO, growth, finite-capacity and physical-release checks passed
    */
    static bool cycles(const char* name, size_t threshold, size_t growth, bool shrink);

    /** Apply genuine backpressure to a small pool with one consumer
    @param [in] threshold Zero grows only on exhaustion; one evaluates after the last reusable node
    @param [in] producers Number of simultaneous callers
    @return All successful lane sequences matched, no retained active session
    */
    static bool pressure(size_t threshold, size_t producers);

    /** Print pool state outside timing; baseline has no equivalent observer
    @param [in] queue Externally quiescent queue
    */
    static void pool(const LockFreeQueue<uint64_t>& queue);
};