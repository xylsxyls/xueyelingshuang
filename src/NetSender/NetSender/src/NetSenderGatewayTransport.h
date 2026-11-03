#pragma once
#include "NetSenderLink.h"
#include "NetInterface/NetInterfaceAPI.h"

// 桌面IPC运输工厂，每次恢复重新发现网关代次，不持有网络对象
class NetSenderAPI NetSenderGatewayTransportFactory : public NetGatewayTransportFactory
{
public:
    /** 创建借用桌面收件箱的重连工厂，不拥有收件箱
    @param [in] inbox 借用至会话停止
    @param [in] pid 固定本机PID，0按标准名称发现
    */
    NetSenderGatewayTransportFactory(NetSenderLocalInbox& inbox, int32_t pid);

    /** 创建本轮独占IPC运输，不创建网络连接
    @return 调用者拥有的新实例，分配失败抛异常由会话处理
    */
    virtual NetGatewayTransport* create();

private:
    /** 禁止复制借用关系及接入身份
    @param [in] other 原工厂
    */
    NetSenderGatewayTransportFactory(const NetSenderGatewayTransportFactory& other);

    /** 禁止复制借用关系及接入身份
    @param [in] other 原工厂
    @return 不可调用
    */
    NetSenderGatewayTransportFactory& operator=(const NetSenderGatewayTransportFactory& other);

private:
    // 借用至使用本工厂的会话停止
    NetSenderLocalInbox* m_inbox;
    // 固定PID或0发现模式
    int32_t m_pid;
};

// 本机IPC管理运输适配，通过共享内存将透明业务记录交给网关
class NetSenderAPI NetSenderGatewayTransport : public NetGatewayTransport
{
public:
    /** 绑定本机IPC目标；地址不是身份，仍由管理TLS验证
    @param [in] inbox 借用已注册收件箱
    @param [in] pid 本机网关PID
    */
    NetSenderGatewayTransport(NetSenderLocalInbox& inbox, int32_t pid);

    /** 监听者有效期间停止
    */
    virtual ~NetSenderGatewayTransport();

    /** 在唯一协议执行者创建本轮本机IPC管理运输，管理握手完成前不报告就绪
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
    // 本实例独占的本机IPC管理运输，不与其他代次共用协议状态
    NetSenderLink m_link;
    // 桌面固定网关PID；0表示按标准进程租约发现
    int32_t m_pid;
};