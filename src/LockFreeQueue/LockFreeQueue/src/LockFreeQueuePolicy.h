#pragma once

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>

// BASE-LF-1.0. All non-success push results mean NOT committed.
enum LockFreeQueueResult
{
    // The item (or requested configuration) was committed.
    LockFreeQueueSuccess = 0,
    // No reusable node and no remaining physical node budget.
    LockFreeQueueCapacityLimit,
    // System allocation failed; reservations were returned.
    LockFreeQueueAllocationFailure,
    // Copy construction failed; the source remains caller-owned.
    LockFreeQueueConstructionFailure,
    // destroy was called and initialization has not succeeded.
    LockFreeQueueNotInitialized,
    // Hazard records, pending growth or protected retired nodes occupy resources.
    LockFreeQueueTemporaryUnavailable,
    // The growth policy returned zero.
    LockFreeQueueGrowthDeclined,
    // A policy threw an exception.
    LockFreeQueuePolicyFailure,
    // Invalid capacity, producer count or address-space overflow.
    LockFreeQueueInvalidConfiguration,
    // A required atomic width is not lock-free on this platform.
    LockFreeQueueAtomicUnavailable,
    // Maintenance could not read its monotonic clock; no shrinking was attempted.
    LockFreeQueueClockUnavailable,
    // Publication segment generation exhausted before wrap; drain and externally quiescent init.
    LockFreeQueueSequenceExhausted
};

// Physical nodes include the sentinel and nodes awaiting safe reuse.
struct LockFreeQueuePoolConfig
{
public:
    // Minimum retained physical node capacity; at least one sentinel.
    size_t initialCapacity;
    // Zero grows only on an actual failed node acquisition.
    size_t growThreshold;
    // -1 means no configured limit; other negative values are invalid.
    int64_t maximumCapacity;
    // Fixed per-instance protection records; no TLS registration or thread handles.
    size_t maximumProducers;

    /** Construct defaults: 64 nodes, threshold 0, unlimited, 1024 producers

    */
    LockFreeQueuePoolConfig();
};

struct LockFreeQueueGrowContext
{
    size_t initialCapacity;
    uint64_t successfulGrowCount;
    size_t currentCapacity;
};

struct LockFreeQueueShrinkContext
{
    size_t initialCapacity;
    uint64_t successfulGrowCount;
    size_t currentCapacity;
    size_t reusableFreeCapacity;
    bool hasLastSuccessfulGrow;
    uint64_t millisecondsSinceLastSuccessfulGrow;
};

struct LockFreeQueueShrinkDecision
{
    size_t shrinkCount;
    // Zero disables automatic shrinking, rather than requesting immediate release.
    uint64_t idleMilliseconds;
};

typedef std::function<size_t(const LockFreeQueueGrowContext&)> LockFreeQueueGrowPolicy;
typedef std::function<LockFreeQueueShrinkDecision(const LockFreeQueueShrinkContext&)> LockFreeQueueShrinkPolicy;

// Weakly consistent observations, never an admission reservation.
struct LockFreeQueuePoolStatistics
{
    size_t currentCapacity;
    size_t reusableFreeCapacity;
    size_t usedNodes;
    // Payload nodes return immediately; this field is zero. Segment retirement is metadata.
    size_t retiredNodes;
    size_t retiringBlockCapacity;
    size_t reservedCapacity;
    size_t peakReservedCapacity;
    // Requested pool storage + object/record/block metadata. Excludes allocator
    // overhead, T's own allocations and user callable captures. An unpublished
    // allocation is charged to reservedCapacity before its bytes enter this snapshot.
    size_t retainedBytes;
    size_t protectionBytes;
    size_t blockCount;
    size_t activeProducers;
    uint64_t successfulGrowCount;
    uint64_t allocationCalls;
    // Publication CAS losses and tail-protection retries; excludes other pool/allocator CAS operations.
    uint64_t casRetries;
};

struct LockFreeQueueMaintenanceResult
{
    LockFreeQueueResult result;
    size_t reclaimedNodes;
    size_t releasedCapacity;
};

// Compatibility wrapper error. status() does not allocate memory.
class LockFreeQueueError : public std::exception
{
public:
    /** Store the uncommitted operation's result
    @param [in] result Failure reason
    */
    explicit LockFreeQueueError(LockFreeQueueResult result);

    /** Return the failure reason

    */
    LockFreeQueueResult status() const;

    /** Return a static message whose lifetime exceeds the exception

    */
    virtual const char* what() const throw();

private:
    // Original failure, without a heap-allocated diagnostic string.
    LockFreeQueueResult m_result;
};

#include "LockFreeQueuePolicy.inl"