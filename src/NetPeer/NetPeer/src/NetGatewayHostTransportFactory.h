#pragma once
#include "NetPeerMacro.h"
#include "NetGatewayTransport.h"

// 每次尝试创建独占管理运输，实际字节后端由宿主配置决定
class NetPeerAPI NetGatewayHostTransportFactory : public NetGatewayTransportFactory
{
public:
    /** 创建新的通用宿主运输，不复用上一次连接状态
    @return 调用者拥有的实例；分配失败抛异常由共享恢复协调结算
    */
    virtual NetGatewayTransport* create();
};