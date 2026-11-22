#pragma once
#include "NetSenderMacro.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

// IPC接收资源配置；数据、控制和未知OPEN隔离实际存储，不再设置每通道包数门槛
struct NetSenderAPI NetSenderLocalInboxPools
{
public:
    // 已识别通道数据的正文槽，直到TLS或透明协议消费完成才归还
    NetBufferPoolConfig m_data;
    // ACK和END独立正文槽，数据满池不会占用此池
    NetBufferPoolConfig m_control;
    // 未识别OPEN独立正文槽，洪泛不挪用已建通道的数据或控制池
    NetBufferPoolConfig m_open;
    // 回调MPSC入口节点池，出队后正文仍继续占用对应正文池
    LockFreeQueuePoolConfig m_queue;
    // 并发路由索引实际池，包含候选和退役节点
    LockFreeHashMapPoolConfig m_routes;

public:
    /** 采用可增长的默认物理池；maximumCapacity均由基础池直接执行
    */
    NetSenderLocalInboxPools();
};