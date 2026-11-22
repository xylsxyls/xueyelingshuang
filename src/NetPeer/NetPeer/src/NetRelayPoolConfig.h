#pragma once
#include "NetPeerMacro.h"
#include "NetInterface/NetInterfaceAPI.h"

// 透明中继输出实际存储配置；控制和数据独立池，共享有序交付链
struct NetPeerAPI NetRelayPoolConfig
{
public:
    // 数据记录池，记录在宿主完整接纳后才归还
    LockFreeHashMapPoolConfig m_dataRecords;
    // 控制记录池，不挪用数据记录槽
    LockFreeHashMapPoolConfig m_controlRecords;
    // 实际数据密文及NTW3头存储
    NetBufferPoolConfig m_dataBytes;
    // 实际控制字节存储
    NetBufferPoolConfig m_controlBytes;

public:
    /** 默认记录池各按约8MiB实际槽大小计算，正文按大小级增长
    */
    NetRelayPoolConfig();
};