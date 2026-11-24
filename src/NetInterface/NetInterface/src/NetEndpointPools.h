#pragma once
#include "NetInterfaceTypes.h"
#include "NetRequestTablePools.h"
#include "NetHeartbeatConfig.h"

// 端点路由器实际存储配置；每个索引节点对应已拥有的池记录，不另设端点/文件条数配额
struct NetInterfaceAPI NetEndpointPools
{
public:
    // 此路由器直接监听的客户端连接使用的心跳及无输入期限
    NetHeartbeatConfig m_heartbeat;
    // 每次转发后保留的暂存容量上限，默认4096字节，0立即释放，-1保留已分配容量
    int64_t m_payloadRetentionBytes;
    // 单轮poll最多结算的超时请求数，默认64，至少1
    uint32_t m_expiredRequestBudget;
    // 单轮poll最多执行的端点关闭步骤数，默认64，至少1
    uint32_t m_closingEndpointBudget;
    // 单轮poll最多回收的到期文件流数，默认64，至少1
    uint32_t m_expiredFileBudget;
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