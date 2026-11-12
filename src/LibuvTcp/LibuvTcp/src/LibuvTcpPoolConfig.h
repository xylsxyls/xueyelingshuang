#pragma once
#include "LibuvTcpMacro.h"
#include <stdint.h>

/** 并发队列的物理节点池与生产者登记上限，不等于可排队消息数量
*/
struct LibuvTcpAPI LibuvQueuePoolConfig
{
public:
    // 初始物理节点数，2至16777216且不超过m_maximumCapacity
    uint32_t m_initialCapacity;
    // 物理节点硬上限，包含哨兵及内部预约，2至16777216，不允许无限增长
    uint32_t m_maximumCapacity;
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

/** 跨线程连接索引的节点池、目录与保护记录配置；运行时不改变
*/
struct LibuvTcpAPI LibuvConnectionPoolConfig
{
public:
    // 初始物理节点数，1至16777216且不超过m_maximumCapacity
    uint32_t m_initialCapacity;
    // 物理节点硬上限，包含候选和退役节点，1至16777216，不等于同时连接数
    uint32_t m_maximumCapacity;
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
    // 已接纳数组和写请求的合计预算，1至INT32_MAX字节；不是整个进程RSS
    uint32_t m_maximumPendingBytes;
    // 主通道及每个附加通道的容量上限；附加槽总量不超过发送队列上限；0关闭，否则为不超过1048576的2次幂
    uint32_t m_laneCapacity;
    // 生产者槽数，平均分配附加通道；通道不足时缓存队列登记，0关闭；小于队列m_maximumProducers且最多65536
    uint32_t m_producerCacheCapacity;
    // 同时交给libuv的写请求上限，1至65536
    uint32_t m_maximumActiveWrites;
    // 空闲写请求缓存上限，0不缓存，不超过65536
    uint32_t m_maximumCachedWrites;

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
    // 空闲缓冲缓存上限，0不缓存，不超过65536块；数量乘块大小不超过INT32_MAX
    uint32_t m_maximumCachedBuffers;

public:
    /** 填写默认配置，不分配内存池或创建网络资源
    */
    LibuvReadPoolConfig();

    /** 检查数值范围，不分配资源或改变配置
    @return true参数有效，跨配置约束在初始化时另行检查
    */
    bool valid() const;
};

/** 原始字节运输的每连接写窗口与实例空闲缓存，全部由同一执行者使用
*/
struct LibuvTcpAPI LibuvChannelWritePoolConfig
{
public:
    // 单次写入上限，1至INT32_MAX字节且不超过每连接预算
    uint32_t m_maximumWriteBytes;
    // 每连接在途字节上限，1至INT32_MAX
    uint32_t m_maximumPendingBytesPerPeer;
    // 每连接在途请求上限，1至65536
    uint32_t m_maximumWritesPerPeer;
    // 实例空闲写请求上限，0不缓存，不超过65536
    uint32_t m_maximumCachedWrites;
    // 实例空闲正文capacity合计上限，0不缓存，不超过INT32_MAX
    uint32_t m_maximumCachedBytes;

public:
    /** 填写默认配置，不分配内存池或创建网络资源
    */
    LibuvChannelWritePoolConfig();

    /** 检查数值范围，不分配资源或改变配置
    @return true参数有效，跨配置约束在初始化时另行检查
    */
    bool valid() const;
};