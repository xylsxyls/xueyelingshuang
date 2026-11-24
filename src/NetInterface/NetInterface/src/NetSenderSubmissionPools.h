#pragma once
#include "NetBufferPoolConfig.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

// 单个提交实例的索引和入口队列策略，随 configurePools 一起在 start 前生效
struct NetInterfaceAPI NetSenderSubmissionOptions
{
public:
    // 请求索引的桶数，0 使用基础库默认值
    size_t m_requestBuckets;
    // 同步回执索引的桶数，与请求索引独立
    size_t m_receiptBuckets;
    // 分散生产者计数，适用于多生产者
    bool m_distributedCounters;
    // 启用基础队列的可撤销预留实现
    bool m_cancelableReservations;
    // 基础队列自动维护触发间隔，至少1
    uint32_t m_maintenanceInterval;
    // 每次自动维护的工作预算，至少1
    size_t m_maintenanceBudget;
    // 底层队列开始容量休眠前的重试次数；0 表示直接进入休眠阶段，仅在等待容量的操作中使用
    uint32_t m_capacityRetriesBeforeSleep;
    // 底层队列的容量休眠毫秒数，至少1；网关提交在满池时直接返回背压
    uint32_t m_capacitySleepMilliseconds;
    // 基础队列消费者的让步间隔，1至65536
    uint32_t m_consumerYieldInterval;
    // owner一次从入口领取的最多批数，默认8，至少1
    uint32_t m_inputBatchBudget;
    // 每批最多领取的完整项数，范围1至32，默认32
    uint32_t m_inputBatchSize;
    // owner 每轮分别交给入口队列、请求索引和回执索引的维护预算，均至少1
    size_t m_queuePollBudget;
    // 请求索引回收与查找维护的单轮预算
    size_t m_requestPollBudget;
    // 回执索引回收与查找维护的单轮预算
    size_t m_receiptPollBudget;
    // 同步 DATA 在交付前的本地期限毫秒，至少1，独立于业务请求的应答期限
    uint32_t m_deliveryTimeoutMs;
    // 入口节点增长策略，空表示使用基础队列默认策略
    LockFreeQueueGrowPolicy m_growPolicy;
    // 入口节点回收策略，空表示使用基础队列默认策略
    LockFreeQueueShrinkPolicy m_shrinkPolicy;

public:
    /** 使用两个128桶索引、分散计数和基础队列的默认维护策略
    */
    NetSenderSubmissionOptions();
};

// 提交层全部物理存储配置；初始化前按实际基础池契约配置，不另设消息条数阈值
struct NetInterfaceAPI NetSenderSubmissionPools
{
public:
    // 请求身份和生命周期记录
    LockFreeHashMapPoolConfig m_requests;
    // 普通长DATA/同步流记录；异步短DATA直接由m_queue节点存储，不占本池
    LockFreeHashMapPoolConfig m_frames;
    // ACK和取消记录，与普通数据隔离
    LockFreeHashMapPoolConfig m_controls;
    // 并发请求身份
    LockFreeHashMapPoolConfig m_fronts;
    // 同步交付回执，与业务应答隔离
    LockFreeHashMapPoolConfig m_receipts;
    // RPC/同步及控制的提交记录；异步DATA由队列内联或m_frames拥有，不重复占输入槽
    LockFreeHashMapPoolConfig m_inputs;
    // RPC/同步正文所有权记录；异步DATA由帧记录直接拥有
    LockFreeHashMapPoolConfig m_payloads;
    // 控制正文所有权记录
    LockFreeHashMapPoolConfig m_controlPayloads;
    // 请求索引含退役节点
    LockFreeHashMapPoolConfig m_requestIndex;
    // 回执索引含退役节点
    LockFreeHashMapPoolConfig m_receiptIndex;
    // MPSC入口实际节点和生产者记录；异步DATA的至多64字节正文内联在节点中
    // owner持有32个入口项和对应的32个命令工作位，上一批完整释放后复用
    LockFreeQueuePoolConfig m_queue;
    // 普通正文的五级物理池；异步DATA的64字节内联正文不占此池
    NetBufferPoolConfig m_bodies;
    // 控制正文独立保留，避免普通正文耗尽后无法取消
    NetBufferPoolConfig m_controlBodies;

public:
    /** 默认元数据池各约8MiB物理槽预算，正文按大小类独立配置；允许调用者更换初值、上限和增长策略
    */
    NetSenderSubmissionPools();
};