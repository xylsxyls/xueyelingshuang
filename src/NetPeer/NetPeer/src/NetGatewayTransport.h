#pragma once
#include "NetLinkTransport.h"
// 将统一网关运输接口适配到宿主字节网络，不直接绑定具体TCP库
class NetPeerAPI NetGatewayNativeTransport : public NetGatewayTransport
{
public:
    /** 管理监听者保持有效至本对象停止
    */
    virtual ~NetGatewayNativeTransport();

    /** 在唯一协议执行者创建本轮宿主网络管理运输，管理握手完成前不报告就绪
    @param [in] address 数值目标地址；桌面IPC提供方忽略此字段
    @param [in] port TCP端口；桌面IPC提供方忽略此字段
    @param [in] options 本端角色和凭据，接纳时复制必要内容
    @param [in] listener 借用管理监听者，保持到shutdown完成
    @param [out] peer 已接纳的非零运输代次，失败为0
    @return NI_OK为开始连接，负数表示未接纳
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
        NetLinkTransportListener* listener, uint64_t& peer);

    /** 由唯一执行者推进管理握手及有界收发，不创建后台线程
    @return 是否仍有运行中的运输或待收敛状态
    */
    virtual bool poll();

    /** 将管理控制帧交给对应代次的有界输出
    @param [in] peer 当前运输代次
    @param [in] frame 调用期间借用，接纳后内部持有必要副本
    @return NI_OK为本层接纳，NI_LIMIT_EXCEEDED为背压，其他负数为失败
    */
    virtual int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 将透明业务记录交给对应代次，不解释业务密文
    @param [in] peer 当前运输代次
    @param [in] record 调用期间借用的完整记录
    @return NI_OK为本层接纳，NI_LIMIT_EXCEEDED为背压，其他负数为失败
    */
    virtual int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 由唯一执行者停止本轮运输，监听者必须保持有效至收敛
    */
    virtual void shutdown();

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @param [in] peer 当前连接代次
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    virtual int32_t outputState(uint64_t peer) const;

private:
    // 本实例独占的宿主网络管理运输，不与其他代次共用协议状态
    NetLinkTransport m_transport;
};