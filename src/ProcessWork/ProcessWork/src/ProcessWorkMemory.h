#pragma once
#include "ProcessWorkMacro.h"
#include "ProcessWorkMemoryPolicy.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <stdint.h>
#include <memory>

/** 本地指针队列的调度参数。在初始化前通过内存配置设置，池容量和扩缩策略另行配置。
*/
struct ProcessWorkAPI ProcessWorkQueueOptions
{
public:
    // -1使用所在位置的默认值（Post分散计数，接收集中计数），0集中，1分散。
    int32_t m_distributedCounters;
    // 自动维护间隔和每轮维护工作量，均须为正。
    uint32_t m_maintenanceInterval;
    // 每次自动维护最多处理的工作量。
    size_t m_maintenanceBudget;
    // 满载转入休眠前的快速尝试次数，允许0。
    uint32_t m_capacityRetries;
    // 原生队列每次满载休眠的毫秒数，必须为正。
    uint32_t m_capacitySleepMilliseconds;
    // 单消费者协作退让间隔，范围1至65536。
    uint32_t m_consumerYieldInterval;
    // 是否使用可撤销的原生发布预留，默认关闭。
    bool m_cancelableReservations;

    /** 使用基础队列的调度默认值。
    */
    ProcessWorkQueueOptions();

    /** 选择集中或按生产者分散计数。
    @param [in] distributed true启用分散计数
    @return LockFreeQueueSuccess
    */
    LockFreeQueueResult configureProducerCounters(bool distributed);

    /** 选择是否使用可撤销的发布预留。
    @param [in] enabled true启用
    @return LockFreeQueueSuccess
    */
    LockFreeQueueResult configureCancelableReservations(bool enabled);

    /** 检查值队列对Entry专用模式的支持。ProcessWork交付指针值，须保留值节点。
    @param [in] enabled 是否只允许侵入式Entry
    @return enabled为false时返回LockFreeQueueSuccess，为true时返回LockFreeQueueInvalidConfiguration
    */
    LockFreeQueueResult configureEntryOnly(bool enabled);

    /** 设置周期维护参数。
    @param [in] interval 两次维护之间的操作数，须为正
    @param [in] workBudget 每轮工作量，须为正
    @return 成功或参数无效，失败保留原设置
    */
    LockFreeQueueResult configureAutomaticMaintenance(uint32_t interval, size_t workBudget);

    /** 设置满载时快速重试和随后每次休眠的时间。
    @param [in] retriesBeforeSleep 快速重试次数
    @param [in] sleepMilliseconds 休眠毫秒数，须为正
    @return 成功或参数无效，失败保留原设置
    */
    LockFreeQueueResult configureCapacityWait(uint32_t retriesBeforeSleep, uint32_t sleepMilliseconds);

    /** 设置消费端协作退让间隔。
    @param [in] operations 两次退让之间的操作数，范围1至65536
    @return 成功或参数无效，失败保留原设置
    */
    LockFreeQueueResult configureConsumerYieldInterval(uint32_t operations);

    /** 检查计数模式、维护预算和退让参数的范围。
    @return true表示适用于本地指针队列
    */
    bool valid() const;

    /** 将本实例的设置传给尚未初始化的值队列。
    @param [in,out] queue 已处于静止配置期的指针队列
    @param [in] postQueue 未显式指定计数模式时是否启用分散计数
    @return true表示全部配置成功；false时调用方不会启用此队列
    */
    template<class T>
    bool apply(LockFreeQueue<T>& queue, bool postQueue) const;
};

/** 内存压力保护，独立于业务配置的共享缓冲块数上限
*/
struct ProcessWorkAPI ProcessWorkMemoryConfig
{
public:
    // 就绪队列独立的调度和退让设置。
    ProcessWorkQueueOptions m_queueOptions;
    // 接收备用队列独立的调度和退让设置。
    ProcessWorkQueueOptions m_freeQueueOptions;
    // 本地消费者通知的原生等待策略及事件池、计数池配置。
    SemaphoreConfig m_notification;
    // 给系统保留的可用物理/提交内存字节，默认256MiB。
    uint64_t m_reserveBytes;
    // 同时保留的物理内存比例，默认10；取两种保留量的较大值，范围0..90。
    uint32_t m_reservePercent;
    // 单轮扩容字节预算，默认4MiB；单块更大时一轮只创建一块。
    uint64_t m_growthBytes;
    // 申请失败后最短重试间隔毫秒，默认100，必须大于0。
    uint32_t m_retryMilliseconds;
    // 本地积压固定字节上限，正数不增长；默认-1不限制且跳过逐消息字节计费，0和小于-1非法
    int64_t m_localQueueBytes;
    // 本地队列初始节点数，默认64；静止初始化时准备，不代表消息正文预算
    uint32_t m_localQueueInitialCapacity;
    // 接收端初始化时预分配的标准批次数，默认0按需申请；空闲备用块不占用消息积压额度
    uint32_t m_receiveBatchReserve;
    // 标准接收批次的记录区字节数，默认65568，至少17以容纳空包。
    uint32_t m_receiveBatchBytes;
    // 接收批次对象和记录区的总字节上限，包含使用中、预留和已回收批次；-1不设业务上限。
    int64_t m_receiveBatchMaximumBytes;
    // 已归还标准批次的缓存上限，至少保留初始预留数的复用能力；默认8，-1不限，零预留下0关闭复用。
    int64_t m_receiveBatchCacheLimit;
    // Post小包备用槽数，默认0；每槽字节数由m_postBufferSlotBytes指定
    uint32_t m_postBufferReserve;
    // 每槽64或128字节，默认128；扣除4字节所有者及消息内部头
    uint32_t m_postBufferSlotBytes;
    // Post小包缓存池最大槽数，-1无配置上限，0关闭缓存池；大包及缓存未命中仍受本地字节预算约束
    int64_t m_postBufferMaximum;
    // 本地就绪队列完整配置；旧initial参数非64时覆盖这里的初始容量，冲突配置拒绝
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueuePoolConfig m_queuePool;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 接收空闲批次队列的独立配置，Post端不使用
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueuePoolConfig m_freeQueuePool;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 就绪队列的增长算法，空函数保留基础库默认算法
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueGrowPolicy m_queueGrow;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 就绪队列的缩容算法，空函数保留基础库默认策略
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueShrinkPolicy m_queueShrink;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 接收空闲队列的独立增长算法
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueGrowPolicy m_freeQueueGrow;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 接收空闲队列的独立缩容算法
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    LockFreeQueueShrinkPolicy m_freeQueueShrink;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 实际正文池和共享块增长机制；不调整积压额度，空指针使用默认算法，初始化复制强引用
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif
    std::shared_ptr<ProcessWorkMemoryPolicy> m_policy;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
    /** 设置默认保护，不设置业务容量上限
    */
    ProcessWorkMemoryConfig();

    /** 初始化前按MB设置固定积压额度，1MB=1024*1024字节；不会调整运行中实例
    @param [in] megabytes 正整数MB，-1关闭额度限制；0、其他负数及换算溢出均非法
    @return true已更新本配置的m_localQueueBytes，false保持原配置不变
    */
    bool setLocalQueueLimitMB(int64_t megabytes);

    /** 一次设置接收批次的实际存储策略，初始化前调用
    @param [in] batchBytes 标准记录区字节数，17至INT32_MAX，较大的单条记录可用一次性批次
    @param [in] maximumBytes 批次对象和记录区总字节数，-1不限；有限时须能容纳一个标准批次和初始预留
    @param [in] cacheLimit 已归还标准批次的缓存数，-1不限，否则取该值与初始预留数的较大值
    @return true设置已保存，false表示参数无效，原配置保留
    */
    bool setReceiveBatchPool(uint32_t batchBytes, int64_t maximumBytes, int64_t cacheLimit);
};

/** 查询系统资源后再创建共享映射；查询失败时拒绝扩容，不循环盲试
*/
class ProcessWorkAPI ProcessWorkMemory
{
public:
    /** 检查策略参数，不分配共享内存
    @param [in] config 内存保护参数
    @return true参数有效
    */
    static bool valid(const ProcessWorkMemoryConfig& config);

    /** 检查本次新增字节是否仍留有系统余量；不是跨进程资源预留
    @param [in] config 内存保护参数
    @param [in] bytes 本次最坏新增量，包含映射及元数据
    @return true允许尝试，false查询失败或压力过高
    */
    static bool allows(const ProcessWorkMemoryConfig& config, uint64_t bytes);

    /** 合并旧初始容量参数与新的完整队列配置，检查冲突
    @param [in] config 初始化配置
    @param [in] freeQueue true选择接收空闲批次队列
    @param [out] pool 成功时为合并后的配置，失败时输出不可用
    @return true复制完成，false初始容量冲突或配置中的回调复制失败
    */
    static bool queueConfig(const ProcessWorkMemoryConfig& config, bool freeQueue, LockFreeQueuePoolConfig& pool);
};

#include "ProcessWorkMemory.inl"