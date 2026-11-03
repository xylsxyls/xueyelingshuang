#pragma once
#include "NetPeerMacro.h"
#include "NetTransport.h"
#include "NetInterface/NetInterfaceAPI.h"

// 通用宿主传输与共享端点协议的进程内组合，不包含Libuv/ProcessWork/线程/磁盘
class NetPeerAPI NetPeerEndpointRouter : public NetEndpointRouter
{
public:
    /** 创建组合，尚不监听；附加端点模式不绑定物理传输
    @param [in] services 借用业务处理器至shutdown
    @param [in] clients 可空，非空使用附加端点模式，借用至shutdown
    */
    explicit NetPeerEndpointRouter(NetEndpointServiceSink* services, NetEndpointServiceSink* clients = nullptr);

    /** 先停止借用本传输的协议，再销毁传输资源
    */
    virtual ~NetPeerEndpointRouter();

    /** 接纳同源TLS监听，未给熵时仅使用已配置的宿主强随机
    @param [in] address 数值地址
    @param [in] port 0至65535，0供隔离测试
    @param [in] tls 服务端配置，调用时复制
    @param [in] peers 1至256
    @return OK接纳或负数，不代表业务授权成功
    */
    int32_t listen(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls, uint32_t peers = 64);

private:
    /** 禁止复制传输资源
    @param [in] other 原对象
    */
    NetPeerEndpointRouter(const NetPeerEndpointRouter& other);

    /** 禁止复制传输资源
    @param [in] other 原对象
    @return 不可调用
    */
    NetPeerEndpointRouter& operator=(const NetPeerEndpointRouter& other);

private:
    // 本组合独占的字节宿主传输，析构前显式解除协议借用
    NetTransport m_transport;
};