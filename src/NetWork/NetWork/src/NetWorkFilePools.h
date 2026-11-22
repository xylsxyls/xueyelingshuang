#pragma once
#include "NetWorkMacro.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

// 原生文件后端的实际存储；生命周期owner在创建文件后端前配置，文件块大小遵守传输协议
struct NetWorkAPI NetWorkFilePools
{
public:
    // 复制输入和待读结果，结果被consume前始终占有实际缓冲
    NetBufferPoolConfig m_bodies;
    // 请求对象，从接纳到结果被consume持续占有
    LockFreeHashMapPoolConfig m_entries;
    // 实际文件句柄记录，从打开到系统句柄关闭持续占有
    LockFreeHashMapPoolConfig m_files;
    // owner到文件执行者的交接队列，与结果持有的记录池独立
    LockFreeQueuePoolConfig m_queue;

public:
    /** 默认预热128条记录，记录与句柄各约8MiB，队列按基础组件配置增长
    */
    NetWorkFilePools();
};