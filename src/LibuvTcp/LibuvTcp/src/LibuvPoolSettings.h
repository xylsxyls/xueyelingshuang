#pragma once
#include "LibuvTcpPoolConfig.h"

/** 长度帧实例的待初始化配置副本，设置时不分配任何队列、索引或缓存
*/
struct LibuvPoolSettings
{
public:
    // 跨线程发送队列
    LibuvQueueSettings m_sendQueue;
    // 独立关闭队列，不占发送窗口
    LibuvQueueSettings m_closeQueue;
    // 连接代次索引
    LockFreeHashMapConfig m_connections;
    // 本库发送预算及缓存
    LibuvSendPoolConfig m_send;
    // 本库读缓冲及缓存
    LibuvReadPoolConfig m_read;

public:
    /** 设置所有默认值，关闭队列默认32个初始节点、256个最大节点
    */
    LibuvPoolSettings();

    /** 检查组合约束，至少为未缓存生产者保留一个队列登记
    @return 是否可进行最终初始化
    */
    bool valid() const;

    /** 将一条队列的参数设置到尚未初始化的值队列，保留基础库的首个错误
    @param [in,out] queue 目标队列，必须尚未初始化
    @param [in] config 对应通路的独立设置
    @return 基础队列配置结果
    */
    template<class T>
    static LockFreeQueueResult configureQueue(LockFreeQueue<T>& queue, const LibuvQueueSettings& config);
};

#include "LibuvPoolSettings.inl"