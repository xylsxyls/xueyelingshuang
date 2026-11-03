#pragma once
#include "NetGatewayClientChannel.h"
#include "NetInterfaceConnection.h"
#include "NetInterfaceCredit.h"
#include <memory>
#include <string>

// 单目标通道严格按认证、OPEN、业务TLS、关闭推进
enum NetGatewayClientPhase
{
    // 尚未接纳连接
    NET_GATEWAY_CLIENT_IDLE,
    // 管理身份认证中
    NET_GATEWAY_CLIENT_MANAGING,
    // 已认证，OPEN尚未被运输接纳
    NET_GATEWAY_CLIENT_OPEN_PENDING,
    // OPEN已经接纳，只等待唯一响应
    NET_GATEWAY_CLIENT_OPEN_WAIT,
    // 路由已建立，业务端点TLS握手中
    NET_GATEWAY_CLIENT_HANDSHAKING,
    // 业务TLS就绪，可以交付应用帧
    NET_GATEWAY_CLIENT_ACTIVE,
    // 首个失败已记录，等待本次poll收尾
    NET_GATEWAY_CLIENT_CLOSING,
    // 停止已调用，唯一关闭通知已取走，不允许复活
    NET_GATEWAY_CLIENT_CLOSED
};

// 同一执行者独占协议、预算和一块未消费密文；运输与监听者仅借用
class NetGatewayClientChannelState
{
public:
    /** 初始化单目标独占连接
    @param [in] product 服务号
    @param [in] external 借用或空
    */
    NetGatewayClientChannelState(uint32_t product, NetGatewayTransport* external);

    /** 记录首个关闭原因
    @param [in] reason 原因
    */
    void fail(int32_t reason);

public:
    // 借用至本通道销毁，shutdown必须收敛其全部回调
    NetGatewayTransport* m_transport;
    // 接纳后借用至唯一关闭通知；未接纳时不通知
    NetTransportListener* m_listener;
    // 独占业务TLS及帧队列
    std::unique_ptr<NetInterfaceConnection> m_connection;
    // 业务通道唯一的外层字节预算
    NetInterfaceCredit m_credit;
    // 只保存强随机上下文和期限，身份始终采用内置根
    NetInterfaceTlsOptions m_options;
    // 单条记录未被TLS消费的密文，长度不超过16KiB
    uint8_t m_input[16384];
    // 已复制密文总长度，0表示没有积压
    uint32_t m_inputSize;
    // 下次提交TLS的位置，不反复移动剩余字节
    uint32_t m_inputOffset;
    // 尚未被运输接纳的唯一OPEN或返还额度消息，背压时复用编码
    std::string m_control;
    // m_control代表的待返还额度；OPEN时为0
    uint32_t m_controlCredit;
    // 本次管理运输代次，成功接纳后非零
    uint64_t m_linkPeer;
    // 初次连接的单调时间，不因各握手阶段重置
    uint64_t m_startedAt;
    // 业务拒收首帧的时刻，0表示没有拒收
    uint64_t m_blockedSince;
    // 固定业务目标，0仍受内部服务路由权限控制
    uint32_t m_product;
    // 首个关闭原因
    int32_t m_reason;
    // 首个关闭时的TLS诊断
    int32_t m_detail;
    // 唯一权威阶段
    NetGatewayClientPhase m_phase;
    // 同一执行者重入保护，不代替互斥锁
    bool m_polling;
};