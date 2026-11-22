#pragma once
#include "NetInterfaceTypes.h"
#include "NetRequestTablePools.h"

// 端点路由器实际存储配置；每个索引节点对应已拥有的池记录，不另设端点/文件条数配额
struct NetInterfaceAPI NetEndpointPools
{
public:
    // RPC等待、端点索引及取消实际存储
    NetRequestTablePools m_requests;
    // 端点对象及其会话元数据；存活至关闭和文件清理完成
    LockFreeHashMapPoolConfig m_endpoints;
    // 文件流及令牌；存活至过期、关闭或完成，索引和期限数组随实际记录增长
    LockFreeHashMapPoolConfig m_files;

public:
    /** 默认预热128槽，各对象池最大槽数按8MiB实际槽大小计算
    */
    NetEndpointPools();
};