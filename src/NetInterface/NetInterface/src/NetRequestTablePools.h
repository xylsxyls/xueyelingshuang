#pragma once
#include "NetInterfaceTypes.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"

// 单执行者路由表的物理存储配置；请求、端点和提前取消互不抢占保留容量
struct NetInterfaceAPI NetRequestTablePools
{
public:
    // 完整请求元数据，存活至完成、取消或断线收敛
    LockFreeHashMapPoolConfig m_requests;
    // 可信端点及其索引入口，活到相关请求全部收敛
    LockFreeHashMapPoolConfig m_endpoints;
    // 提前到达的取消标识，活到来源高水位推进或端点移除
    LockFreeHashMapPoolConfig m_cancellations;

public:
    /** 每类默认预分配128槽，最大槽数按8MiB实际槽大小计算；每项索引另有附加存储，索引桶及期限数组随池内记录数量增长
    */
    NetRequestTablePools();
};