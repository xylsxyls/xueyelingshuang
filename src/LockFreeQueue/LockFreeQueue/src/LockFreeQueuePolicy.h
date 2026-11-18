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
    // Node, producer-record or publication-metadata capacity is exhausted (including reservations).
    LockFreeQueueCapacityLimit,
    // System allocation failed; reservations were returned.
    LockFreeQueueAllocationFailure,
    // Copy construction failed; the source remains caller-owned.
    LockFreeQueueConstructionFailure,
    // destroy was called and initialization has not succeeded.
    LockFreeQueueNotInitialized,
    // Only try APIs: a concurrent claim/publication invalidated this attempt.
    LockFreeQueueBusy,
    // Source/numeric compatibility alias; no longer denotes resource exhaustion.
    LockFreeQueueTemporaryUnavailable = LockFreeQueueBusy,
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
    LockFreeQueueSequenceExhausted,
    // Storage is not safely reusable, or this host cannot perform the requested capacity wait.
    LockFreeQueueResourceUnavailable,
    // Configuration after initialization, or a producer context already attached.
    LockFreeQueueInvalidState
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
    // 成功发布新增块后通知实际新增槽数及成功计数快照，不含初始化；可并发，不可重入本队列，异常忽略
    std::function<void(size_t, uint64_t)> m_onGrowth;

    /** Construct defaults: 64 nodes, threshold 0, unlimited, 1024 producers
    */
    LockFreeQueuePoolConfig();
};

// 扩容策略的弱一致输入，允许多个生产者并发评估
struct LockFreeQueueGrowContext
{
    // 配置的初始物理容量，包含保留额度
    size_t initialCapacity;
    // 已成功发布增长块的累计次数，不包含初始申请
    uint64_t successfulGrowCount;
    // 已发布且尚未物理释放的节点容量，不包含在途预留
    size_t currentCapacity;
};

// 唯一消费者的缩容输入，不能作为生产者领取额度的依据
struct LockFreeQueueShrinkContext
{
    // 缩容不能突破的初始物理容量
    size_t initialCapacity;
    // 已成功发布增长块的累计次数
    uint64_t successfulGrowCount;
    // 已发布且尚未物理释放的节点容量
    size_t currentCapacity;
    // 当前可复用空闲额度的弱一致快照
    size_t reusableFreeCapacity;
    // 是否存在有效的最近成功扩容时刻
    bool hasLastSuccessfulGrow;
    // 最近成功扩容后的单调毫秒数；仅hasLastSuccessfulGrow为true时有意义
    uint64_t millisecondsSinceLastSuccessfulGrow;
};

// 本轮缩容建议，受完整块、空闲时间、保护及维护预算限制
struct LockFreeQueueShrinkDecision
{
    // 本轮允许新增退役的节点容量上限，0表示不新增退役块
    size_t shrinkCount;
    // Zero disables automatic shrinking, rather than requesting immediate release.
    uint64_t idleMilliseconds;
};

// 返回新增容量而非目标总容量；0拒绝增长，异常转换为PolicyFailure
typedef std::function<size_t(const LockFreeQueueGrowContext&)> LockFreeQueueGrowPolicy;
// 唯一消费者调用的缩容策略；建议值不代表当次已物理释放
typedef std::function<LockFreeQueueShrinkDecision(const LockFreeQueueShrinkContext&)> LockFreeQueueShrinkPolicy;

// Weakly consistent observations, never an admission reservation.
struct LockFreeQueuePoolStatistics
{
    // 已发布且尚未物理释放的节点容量，包含保留额度
    size_t currentCapacity;
    // 尚可用于后续申请的空闲额度弱快照
    size_t reusableFreeCapacity;
    // 已占用额度，包含保留额度、生产者私有缓存和未消费项
    size_t usedNodes;
    // Payload nodes return immediately; this field is zero. Segment retirement is metadata.
    size_t retiredNodes;
    // 已摘除但尚未物理释放的块容量
    size_t retiringBlockCapacity;
    // 正在增长但尚未发布或回滚的预留容量
    size_t reservedCapacity;
    // 已发布容量与在途预留之和的历史峰值
    size_t peakReservedCapacity;
    // Requested pool storage + object/record/block metadata. Excludes allocator
    // overhead, T's own allocations and user callable captures. An unpublished
    // allocation is charged to reservedCapacity before its bytes enter this snapshot.
    size_t retainedBytes;
    // 生产者登记及遍历保护的保留字节；辅助段登记表另计入retainedBytes
    size_t protectionBytes;
    // 尚未物理释放的块数量，包含初始块和退役块
    size_t blockCount;
    // 正被独占的登记槽数，包含长期绑定的生产者上下文
    size_t activeProducers;
    // 成功发布增长块的累计次数，饱和后不回绕
    uint64_t successfulGrowCount;
    // 向系统申请池存储的累计尝试次数，包含失败申请
    uint64_t allocationCalls;
    // Publication CAS losses and tail-protection retries; excludes other pool/allocator CAS operations.
    uint64_t casRetries;
};

// 单次维护结果，错误不撤销此前已经完成的安全回收
struct LockFreeQueueMaintenanceResult
{
    // Success或本轮时钟、策略错误
    LockFreeQueueResult result;
    // 从空闲登记归还的私有缓存额度数，不是物理释放量
    size_t reclaimedNodes;
    // 本轮实际物理释放的节点容量
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
    @return 构造时保存的错误码，不申请存储
    */
    LockFreeQueueResult status() const;

    /** Return a static message whose lifetime exceeds the exception
    @return 非空静态错误文本，不转移所有权，不抛异常
    */
    virtual const char* what() const throw();

private:
    // Original failure, without a heap-allocated diagnostic string.
    LockFreeQueueResult m_result;
};

#include "LockFreeQueuePolicy.inl"