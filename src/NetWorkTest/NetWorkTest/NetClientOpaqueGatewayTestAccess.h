#pragma once
#include "../../NetClientManager/NetClientManager/NetClientOpaqueGateway.h"

// 测试侧读取目录生命周期，不给生产网关增加测试字段或入口。
struct NetOpaqueTestDiscovery
{
public:
    // 原始成员指针类型，保持生产类访问属性及ABI。
    typedef NetGatewayDiscovery NetClientOpaqueGateway::*Type;
};

/** 取得显式实例化的目录成员指针
@param [in] tag 成员类型选择器
@return 原始成员指针
*/
NetOpaqueTestDiscovery::Type netOpaqueTestMember(NetOpaqueTestDiscovery tag);

#include "NetClientOpaqueGatewayTestAccess.inl"