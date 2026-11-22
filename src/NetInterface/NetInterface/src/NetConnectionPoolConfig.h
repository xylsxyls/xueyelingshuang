#pragma once
#include "NetBufferPoolConfig.h"

// 单连接TLS输出资源；控制与普通数据独立保留，但控制不能截断已开始的帧
struct NetInterfaceAPI NetConnectionPoolConfig
{
public:
    // 普通帧记录的初值、上限与增长策略
    LockFreeHashMapPoolConfig m_dataRecords;
    // 控制帧记录的初值、上限与增长策略
    LockFreeHashMapPoolConfig m_controlRecords;
    // 普通帧实际编码缓冲
    NetBufferPoolConfig m_dataBytes;
    // 控制帧实际编码缓冲
    NetBufferPoolConfig m_controlBytes;

public:
    /** 默认物理记录预算各约8MiB；正文按大小类增长，无附加帧数阈值
    */
    NetConnectionPoolConfig();
};