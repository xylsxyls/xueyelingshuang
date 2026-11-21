#pragma once
#include "CTaskThreadManagerMacro.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <map>

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
    // 扩缩容策略抛异常
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

/** 单个优先级 FIFO 的参数；构造后调整字段，在线程启动前交给线程配置。
*/
struct CTaskThreadManagerAPI CTaskQueueConfig
{
public:
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
    // 自动维护每隔多少个已消费项执行一次，须为正。
    uint32_t m_maintenanceInterval;
    // 每次自动维护的工作量，须为正。
    size_t m_maintenanceBudget;
    // 消费者每处理多少项让步一次，范围 1 至 65536。
    uint32_t m_consumerYieldInterval;
    // 容量不足先快速重试多少次，0 表示立即进入休眠阶段。
    uint32_t m_capacityRetries;
    // 快速重试结束后每次容量等待的毫秒数，须为正。
    uint32_t m_capacitySleepMilliseconds;
    // 将队列的发布计数分散到生产者，适合多个生产者。
    bool m_distributedCounters;
    // 开启时消费稳定 Entry 地址，关闭时从混合队列读取任务记录指针
    bool m_entryOnly;
    // 是否先预约位置；关闭时并发投递按成功发布排序
    bool m_cancelableReservations;

public:
    /** 使用线程队列的默认配置，不分配队列。
    */
    CTaskQueueConfig();

    /** 按 FIFO 原生规则校验并复制节点池参数，不预建节点。
    @param [in] config 节点容量、生产者登记及增长通知
    @return 队列配置状态，失败保留原参数
    */
    LockFreeQueueResult configurePool(const LockFreeQueuePoolConfig& config);

    /** 设置增长算法，空函数使用基础库默认增长。
    @param [in] policy 只计算增长量的回调
    @return 成功或复制失败
    */
    LockFreeQueueResult setGrowPolicy(const LockFreeQueueGrowPolicy& policy);

    /** 设置缩容算法，空函数关闭策略缩容。
    @param [in] policy 按空闲状态决定缩容量的回调
    @return 成功或复制失败
    */
    LockFreeQueueResult setShrinkPolicy(const LockFreeQueueShrinkPolicy& policy);

    /** 选择统一或分散的发布计数。
    @param [in] distributed 多生产者通常使用分散计数
    @return 配置成功
    */
    LockFreeQueueResult configureProducerCounters(bool distributed);

    /** 选择队列的载荷模式，当前线程配置及每级覆盖独立生效
    @param [in] enabled true 采用专用 Entry 地址前缀，false 采用混合模式的值前缀
    @return 配置成功
    */
    LockFreeQueueResult configureEntryOnly(bool enabled);

    /** 设置本级 FIFO 是否允许可取消的位置预约
    @param [in] enabled true 允许先预约，false 按成功发布顺序排列并发投递
    @return 配置成功
    */
    LockFreeQueueResult configureCancelableReservations(bool enabled);

    /** 设置队列的自动维护间隔与工作预算。
    @param [in] interval 两次维护之间已消费的项数，须为正
    @param [in] workBudget 每次维护预算，须为正
    @return 成功或配置非法，失败保留原值
    */
    LockFreeQueueResult configureAutomaticMaintenance(uint32_t interval, size_t workBudget);

    /** 设置启用容量重试后的退避参数。
    @param [in] retriesBeforeSleep 休眠前的快速重试次数，允许为零
    @param [in] sleepMilliseconds 每次休眠毫秒数，须为正
    @return 成功或配置非法，失败保留原值
    */
    LockFreeQueueResult configureCapacityWait(uint32_t retriesBeforeSleep, uint32_t sleepMilliseconds);

    /** 设置本级队列的消费者让步周期。
    @param [in] operations 操作数，范围 1 至 65536
    @return 成功或配置非法，失败保留原值
    */
    LockFreeQueueResult configureConsumerYieldInterval(uint32_t operations);

};

/** 每条工作线程的初始化参数；初始化复制并冻结配置，策略捕获对象须保持有效并自行同步并发访问
*/
struct CTaskThreadManagerAPI CTaskThreadConfig : public CTaskQueueConfig
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
    // 稳定任务记录池配置；一条记录占一槽，含排队、执行、重做和延迟回收
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeHashMapPoolConfig m_taskMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 唤醒单消费者的本地通知配置。
    SemaphoreConfig m_wakeupSemaphore;
    // 停止、退出等完成通知的配置，独立于消费者唤醒。
    SemaphoreConfig m_completionSemaphore;
    // 必须非零的兼容参数；稳定槽位池不分配哈希桶，此值不影响实际容量
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

    /** 初始化前为一个正优先级指定独立 FIFO 配置，失败保留已有设置。
    @param [in] level 正任务优先级
    @param [in] config 该级的完整配置，成功时复制
    @return Ok、Invalid、NoMemory 或策略复制错误
    */
    CTaskThreadResult setPriorityQueueConfig(int32_t level, const CTaskQueueConfig& config);

    /** 取得指定级别的有效配置。
    @param [in] level 任务优先级
    @return 覆盖项或默认 FIFO 配置的借用引用，修改配置前有效
    */
    const CTaskQueueConfig& priorityQueueConfig(int32_t level) const;

    /** 判断维护时是否需要释放空闲生产者绑定，供任一队列的缩容算法使用。
    @return 默认或覆盖项有缩容策略返回 true
    */
    bool hasQueueShrinkPolicy() const;

private:
    // 仅存显式覆盖的优先级，线程启动后只读。
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::map<int32_t, std::shared_ptr<const CTaskQueueConfig>> m_priorityQueues;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

};

/** 管理器配置只在没有线程及没有并发调用的静止阶段设置
*/
struct CTaskThreadManagerAPI CTaskThreadManagerConfig
{
public:
    // 无参 init() 使用的工作线程配置；显式传入线程配置时使用调用参数。
    CTaskThreadConfig m_defaultThread;
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

#include "CTaskThreadModels.inl"