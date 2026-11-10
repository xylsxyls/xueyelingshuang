#pragma once
#include "CTaskThreadManagerMacro.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

/** 本次调用状态；成功接纳不等于任务已经执行完成
*/
enum CTaskThreadStatus
{
    // 操作成功
    CTaskThreadOk = 0,
    // 参数非法
    CTaskThreadInvalid,
    // 已关闭接纳
    CTaskThreadClosed,
    // 达到配置的容量上限
    CTaskThreadCapacity,
    // 内部单次尝试暂不可用；普通公开接口自行重试，不向调用方暴露竞争
    CTaskThreadRetry,
    // 内存申请失败
    CTaskThreadNoMemory,
    // 策略拒绝或抛异常
    CTaskThreadPolicyFailure,
    // 业务回调抛异常
    CTaskThreadCallbackFailure,
    // 系统调用失败，保留原生错误码
    CTaskThreadSystemFailure,
    // 目标平台不支持所需能力
    CTaskThreadUnsupported,
    // 对象不存在
    CTaskThreadNotFound,
    // 当前线程不能等待自身或自己的回调退出
    CTaskThreadSelfWait,
    // 仅 try 接口报告尚未提交的竞争失败，不表示容量不足
    CTaskThreadBusy,
    // 单调编号已耗尽
    CTaskThreadSequenceExhausted,
    // 容量尚未用满，但资源暂时不能安全复用；不是 CAS 竞争
    CTaskThreadResourceUnavailable
};

/** 系统线程优先级，与 Task 的整数优先级独立
*/
enum CTaskSystemPriority
{
    // 保持操作系统创建线程时的默认值
    CTaskPriorityInherited = 0,
    // 空闲级，平台不支持时明确返回 Unsupported
    CTaskPriorityIdle,
    // 低于普通线程
    CTaskPriorityLow,
    // 普通线程
    CTaskPriorityNormal,
    // 高于普通线程，POSIX 可能要求实时调度权限
    CTaskPriorityHigh,
    // 更高等级，仍不映射 Windows 的 TIME_CRITICAL
    CTaskPriorityHighest
};

/** 无分配的逐调用结果
*/
struct CTaskThreadManagerAPI CTaskThreadResult
{
public:
    // 本次状态
    CTaskThreadStatus m_status;
    // 已经接纳或完成修改；为 true 时不能重复投递
    bool m_committed;
    // 原生系统错误码
    int32_t m_nativeError;

public:
    /** 构造结果
    @param [in] status 状态
    @param [in] committed 是否提交
    @param [in] nativeError 原生错误码
    */
    CTaskThreadResult(CTaskThreadStatus status = CTaskThreadOk, bool committed = false, int32_t nativeError = 0);

};

/** 每条工作线程的初始化参数；初始化复制，此后不允许并发修改策略捕获对象
*/
struct CTaskThreadManagerAPI CTaskThreadConfig
{
public:
    // 优先级桶有序索引的节点和值池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeMapConfig m_priorityMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 稳定任务记录池配置；V3 一条记录占一槽，含排队、执行、重做和延迟回收
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeHashMapPoolConfig m_taskMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 每个优先级 FIFO 的节点池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueuePoolConfig m_queue;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // FIFO 扩容算法，返回增量，上限由基础库截断
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueGrowPolicy m_queueGrow;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // FIFO 缩容算法，包含动态空闲时间
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueShrinkPolicy m_queueShrink;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 兼容保留的非零参数；V3 稳定槽位池不分配哈希桶
    size_t m_taskBuckets;
    // 已接纳任务加在途预留上限，包含正在执行任务；-1 不设业务上限
    int64_t m_maximumTasks;
    // 空闲补查间隔，防止已提交任务因系统通知失败而永久滞留
    uint32_t m_idleCheckMilliseconds;
    // 初始系统优先级；设置失败使初始化失败
    CTaskSystemPriority m_systemPriority;

public:
    /** 使用兼容默认值
    */
    CTaskThreadConfig();

};

/** 管理器配置只在没有线程及没有并发调用的静止阶段设置
*/
struct CTaskThreadManagerAPI CTaskThreadManagerConfig
{
public:
    // 线程注册表节点池
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeMapConfig m_threadMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
    /** 使用默认容量和扩缩策略
    */
    CTaskThreadManagerConfig();

};

/** 弱一致负载观察，不能用来预留容量或授权销毁
*/
struct CTaskThreadManagerAPI CTaskThreadStatistics
{
public:
    // 尚未完成任务及在途投递预留
    uint64_t m_outstanding;
    // 排队数，不含正在执行任务
    uint64_t m_queued;
    // 完成的执行次数，重做另计
    uint64_t m_executed;
    // 接纳失败次数
    uint64_t m_rejected;
    // 回调或资源处理错误次数
    uint64_t m_errors;
    // 当前执行优先级，0 为空闲
    int32_t m_currentLevel;
    // 是否仍接纳任务
    bool m_accepting;
    // 工作线程是否已经完成退出
    bool m_exited;

public:
    /** 清零观察结果
    */
    CTaskThreadStatistics();

};

/** 取得不会分配内存的中文状态说明
@param [in] status 状态值
@return 静态字符串，未知值也有说明
*/
CTaskThreadManagerAPI const char* taskThreadStatusText(CTaskThreadStatus status);