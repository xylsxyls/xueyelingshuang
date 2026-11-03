#pragma once
#include "NetPeerMacro.h"
#include "NetTransport.h"
#include "NetGatewayHostTransportFactory.h"
#include "NetGatewayDiscovery.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <memory>

// 进程内组合只提供宿主工厂/发现及旧直连适配，恢复/请求状态机唯一实现在NetInterface
class NetPeerAPI NetPeerClientSession : public NetClientSession
{
public:
    /** 绑定本组合的借用接口，不创建连接或线程
    */
    NetPeerClientSession();

    /** 先停止共享会话并解除借用，再释放本组合成员
    */
    virtual ~NetPeerClientSession();

    /** 显式替换旧协议通道，成功后释放旧默认传输
    @param [in] channel 可空解除；非空借用至shutdown/更换
    @return OK或INVALID_STATE
    */
    int32_t bindChannel(NetClientChannel* channel);

    /** 旧直连诊断组合，未绑定外部通道时创建通用宿主传输
    @param [in] address 数值地址或由外部通道解释的地址
    @param [in] port 1至65535
    @param [in] tls TLS配置，空熵仅采用已配置宿主强随机
    @param [in] handshake 平台1至3及公开能力/设备声明
    @param [in] listener 借用至shutdown
    @return OK接纳或负数，不做协议降级或重放
    */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
        const NetInterfaceHandshakeOptions& handshake, ::NetSessionListener* listener);

    /** 接纳普通桌面/手机/Wasm的进程内逻辑服务，跳过本机Manager但仍接远端Manager
    @param [in] product 固定逻辑目标
    @param [in] address 默认入口
    @param [in] port 1至65535
    @param [in] tls 强随机及期限，空熵只使用宿主能力
    @param [in] handshake 实际平台声明，不按选用本库改成手机
    @param [in] listener 借用至shutdown
    @param [in] factory 可空使用本组合宿主工厂/发现，非空跳过默认发现
    @return OK接纳或负数，结果由poll交付
    */
    int32_t connectService(uint32_t product, const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
        const NetInterfaceHandshakeOptions& handshake, ::NetSessionListener* listener, NetGatewayTransportFactory* factory = nullptr);

private:
    /** 禁止复制借用关系
    @param [in] other 原组合
    */
    NetPeerClientSession(const NetPeerClientSession& other);

    /** 禁止复制借用关系
    @param [in] other 原组合
    @return 不可调用
    */
    NetPeerClientSession& operator=(const NetPeerClientSession& other);

private:
    // 默认通用字节运输工厂，生命周期覆盖基类使用
    NetGatewayHostTransportFactory m_factory;
    // 发现组合，协议基类只借用其抽象接口
    NetGatewayDiscovery m_discovery;
    // 旧诊断直连拥有的适配，停止协议后才释放
    NetTransport* m_direct;
};