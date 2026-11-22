#pragma once
#include "NetWorkMacro.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

// 原生宿主桥实际存储池；初始化前配置，各实例独立持有存储，配置不表示消息条数配额
struct NetWorkAPI NetWorkHostPools
{
public:
    // 已复制但尚未交付的正文，包含正在执行回调借用的缓冲
    NetBufferPoolConfig m_bodies;
    // 连接入口对象，快照和迟到回调仍持有时不提前归还
    LockFreeHashMapPoolConfig m_mail;
    // 输入记录，从生产者复制前到观察者回调退出始终占用实际槽
    LockFreeHashMapPoolConfig m_records;
    // 跨线程连接索引节点，包含基础库退役及快照开销
    LockFreeHashMapPoolConfig m_index;
    // 跨线程输入队列，owner导入后归还队列节点，记录和正文继续保留
    LockFreeQueuePoolConfig m_incoming;

public:
    /** 默认小额预热，记录及连接对象按实际槽大小设置8MiB最大存储，可由调用方覆盖
    */
    NetWorkHostPools();
};