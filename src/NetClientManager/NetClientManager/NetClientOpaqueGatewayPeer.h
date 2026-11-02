#pragma once
#include "NetWork/NetWorkAPI.h"

class NetClientOpaqueGateway;

// 每本机端点独占一个透明TCP，避免把不同业务进程的旧TLS流混接到新节点。
class NetClientOpaqueGatewayPeer : public NetRelayTransportListener
{
public:
    /** 绑定本机代次，不启用网络
    @param [in] owner 父网关
    @param [in] local 本机代次
    */
    NetClientOpaqueGatewayPeer(NetClientOpaqueGateway& owner, uint64_t local);

    /** 监听字段析构前关闭原始运输层
    */
    ~NetClientOpaqueGatewayPeer();

    /** 记录远端连接就绪，不改变业务端点TLS状态
    @param [in] peer 远端运输代次
    */
    virtual void onRelayConnected(uint64_t peer);

    /** 将远端完整记录提交给本机端点，保持逐端点FIFO
    @param [in] peer 远端运输代次
    @param [in] record 回调期间借用的透明记录
    @return false为本机有界背压；true已接纳或已关闭
    */
    virtual bool onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 通知父网关远端失效，不把旧TLS流重放给新入口
    @param [in] peer 远端运输代次
    @param [in] reason 运输关闭原因
    */
    virtual void onRelayClosed(uint64_t peer, int32_t reason);

public:
    // 借用父网关，父对象在全部peer停止并销毁后释放
    NetClientOpaqueGateway* m_owner;
    // 对应本机端点的不可复用代次
    uint64_t m_local;
    // 对应远端TCP运输代次，连接尚未接纳时为0
    uint64_t m_remote;
    // 本端点独占的透明TCP适配，析构前停止全部回调
    NetRelayTransport m_transport;
    // 本模块保存的固定值候选，不借用发现器的字符串或堆内存
    NetClientDiscoveryPoint m_address;
    // 已收到远端连接就绪，可提交本机排队记录
    bool m_connected;
    // 已确定终态，父网关在poll外层移除本实例
    bool m_closed;
};