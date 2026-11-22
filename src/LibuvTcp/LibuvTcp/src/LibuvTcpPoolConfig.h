#pragma once
#include "LibuvTcpMacro.h"
#ifdef _WIN32
// 基础池会引入 Windows.h，须先选定与 libuv 相同的 Winsock 头文件。
#include <winsock2.h>
#endif
#include "LockFreeHashMap/LockFreeHashMapAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include <stdint.h>

/** 并发队列的物理节点池与生产者登记上限，不等于可排队消息数量
*/
struct LibuvTcpAPI LibuvQueuePoolConfig
{
public:
    // 初始物理节点数，2至16777216且不超过m_maximumCapacity
    uint32_t m_initialCapacity;
    // 物理节点硬上限，包含哨兵及内部预约，2至16777216，-1取消节点上限
    int64_t m_maximumCapacity;
    // 剩余空闲节点达到此值时评估增长，0表示用尽后再增长
    uint32_t m_growThreshold;
    // 同时借用的生产者登记数，1至65536；缓存的生产者也占用登记
    uint32_t m_maximumProducers;

public:
    /** 填写默认配置，不分配内存池或创建网络资源
    */
    LibuvQueuePoolConfig();

    /** 检查数值范围，不分配资源或改变配置
    @return true参数有效，跨配置约束在初始化时另行检查
    */
    bool valid() const;
};

/** 一条实际提交队列的节点池、发布和维护策略。在初始化前按队列分别设置。
*/
struct LibuvTcpAPI LibuvQueueSettings : public LockFreeQueuePoolConfig
{
    // 将队列生产者统计分摊到各登记；长度帧发送默认开启
    bool m_distributed;
    // 允许回收尚未完成的预约；长度帧发送默认开启
    bool m_cancelable;
    // 触发一次维护的消费次数，大于零
    uint32_t m_maintenanceInterval;
    // 单次维护工作预算，大于零
    size_t m_maintenanceBudget;
    // 容量不足时进入定时休眠之前的重试次数
    uint32_t m_capacityRetries;
    // 达到重试次数后每次休眠的毫秒数，大于零
    uint32_t m_sleepMilliseconds;
    // 消费者协作让步间隔，1至65536
    uint32_t m_consumerInterval;
    // 空策略使用基础库默认增长策略
    LockFreeQueueGrowPolicy m_growPolicy;
    // 空策略使用基础库默认回收策略
    LockFreeQueueShrinkPolicy m_shrinkPolicy;

    /** 更新基础节点池参数，保留本通路的发布和维护策略
    @param [in] pool 基础库配置
    @return 当前配置
    */
    LibuvQueueSettings& operator=(const LockFreeQueuePoolConfig& pool);

    /** 设置节点池和队列策略的默认值
    @param [in] sending 是否启用长度帧发送队列的生产者统计和可取消预约
    */
    explicit LibuvQueueSettings(bool sending = false);
};

/** 跨线程连接索引的节点池、目录与保护记录配置；运行时不改变
*/
struct LibuvTcpAPI LibuvConnectionPoolConfig
{
public:
    // 初始物理节点数，1至16777216且不超过m_maximumCapacity
    uint32_t m_initialCapacity;
    // 物理节点硬上限，包含候选和退役节点，1至16777216，-1取消节点上限，实际仍受目录约束
    int64_t m_maximumCapacity;
    // 剩余空闲槽达到此值时评估增长，0表示按需增长
    uint32_t m_growFreeThreshold;
    // 固定块目录数，1至65536；每块最多65535槽
    uint32_t m_directoryCapacity;
    // 同时操作和回调重入的保护记录数量，1至65536
    uint32_t m_protectionRecordCount;
    // 整数副本缓存格数，0关闭，否则为不超过16777216的2次幂
    uint32_t m_lookupCacheCapacity;
    // 连接哈希桶数，1至16777216且为2的幂
    uint32_t m_bucketCount;

public:
    /** 填写默认配置，不分配内存池或创建网络资源
    */
    LibuvConnectionPoolConfig();

    /** 检查数值范围，不分配资源或改变配置
    @return true参数有效，跨配置约束在初始化时另行检查
    */
    bool valid() const;
};

/** 本库发送预算、首线程通道、生产者缓存与写请求池，全部按实例计
*/
struct LibuvTcpAPI LibuvSendPoolConfig
{
public:
    // 已接纳数组和写请求的合计预算，1至INT32_MAX字节或-1取消预算；固定数组和物理内存仍约束容量
    int64_t m_maximumPendingBytes;
    // 主通道及每个附加通道的容量上限；附加槽总量不超过发送队列上限；0关闭，否则为不超过1048576的2次幂
    uint32_t m_laneCapacity;
    // 生产者槽数，平均分配附加通道；通道不足时缓存队列登记，0关闭；小于队列m_maximumProducers且最多65536
    uint32_t m_producerCacheCapacity;
    // 同时交给libuv的写请求上限，1至65536，-1取消窗口限制
    int64_t m_maximumActiveWrites;
    // 空闲写请求缓存上限，0不缓存，1至65536，-1保留所有已分配请求
    int64_t m_maximumCachedWrites;

public:
    /** 填写默认配置，不分配内存池或创建网络资源
    */
    LibuvSendPoolConfig();

    /** 检查数值范围，不分配资源或改变配置
    @return true参数有效，跨配置约束在初始化时另行检查
    */
    bool valid() const;
};

/** 长度帧接口的循环专用读缓冲池，容量只约束空闲缓存
*/
struct LibuvTcpAPI LibuvReadPoolConfig
{
public:
    // 每块读缓冲大小，1至16777216字节
    uint32_t m_bufferBytes;
    // 空闲读块缓存上限，-1按需保留，0不缓存；有限值最多65536，配置字节总量最多INT32_MAX
    int64_t m_maximumCachedBuffers;

public:
    /** 填写默认配置，不分配内存池或创建网络资源
    */
    LibuvReadPoolConfig();

    /** 检查数值范围，不分配资源或改变配置
    @return true参数有效，跨配置约束在初始化时另行检查
    */
    bool valid() const;
};

/** 原始字节运输的实际写请求池；所有连接共享存储，不另设每连接条数或在途字节门槛
*/
struct LibuvTcpAPI LibuvChannelWritePoolConfig
{
public:
    // 单次提交的最大正文长度，1至INT32_MAX；大正文按实际长度分配，不预建最大正文
    uint32_t m_maximumWriteBytes;
    // 预建独占生产者通道数上限，默认16，0关闭；实际数还受生产者登记和可用记录约束
    uint32_t m_maximumLanes;
    // 单条独占通道的预建槽上限，默认1048576，须为正的2次幂且不超过1048576
    uint32_t m_maximumLaneSlots;
    // 实际写请求槽池；初始至少64槽时最多一半预留生产者通道，其余供合批及在途写入
    LockFreeHashMapPoolConfig m_records;
    // 跨线程提交描述符的实际队列池，包含生产者登记和回收余量
    LibuvQueueSettings m_submission;
    // 已连接编号的并发只读快照池；TCP对象仍只由loop访问
    LockFreeHashMapPoolConfig m_connections;
    // 连接快照的哈希桶请求，0使用基础库默认值
    size_t m_connectionBuckets;

public:
    /** 默认单条32KiB，初始2槽、最大256槽；调用方可直接配置基础池增长及上限
    */
    LibuvChannelWritePoolConfig();

    /** 检查单次正文及独占通道槽数的范围；实际池配置在configureWritePool中由基础组件验证
    @return 两项范围是否均有效
    */
    bool valid() const;
};