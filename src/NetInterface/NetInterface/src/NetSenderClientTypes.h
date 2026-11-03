#pragma once
#include "NetInterfaceClientTypes.h"

// 两个客户端提供方共用的固定v1配置；不拥有指针指向的宿主上下文
struct NetSenderGatewayOptions
{
    // 数值入口/平台/能力/稳定device/强随机期限；选择进程内不改变实际平台
    NetWorkClientOptions m_network;
    // v1布局保留项，所有提供方均必须为0；真实PID选择属于桌面后端
    int32_t m_localGatewayPid;
};