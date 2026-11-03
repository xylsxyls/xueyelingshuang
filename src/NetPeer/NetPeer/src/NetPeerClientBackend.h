#pragma once
#include "NetPeerTypes.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <NetInterface/NetInterfaceAPI.h>

// 原生组合入口，只在执行者启动前借用本地提交端口；不是业务NetSender接口
class NetPeerAPI NetPeerClientBackend
{
public:
    /** 创建连接之前借用外层提交端口，不接管端口或协议所有权
    @param [in] handle 本提供方创建的句柄，尚未连接
    @param [in] submission 活到Peer销毁且所有生产者退出
    @return OK或明确参数／生命期错误
    */
    static int32_t bindSubmission(NetPeerClientHandle handle, NetSenderSubmission* submission);
};