#pragma once
#include "NetInterfaceTestAccess.h"
#include "NetClientSessionTest.h"
#include <string>

// 管理身份由测试替身提供，业务端点使用真实证书和TLS；只验证接收调度，不代替管理认证测试
class NetGatewayReceiveTest : public NetGatewayChannelTestTransport, public NetTransportListener
{
public:
    /** 创建独立服务端证书和实际TLS连接
    */
    NetGatewayReceiveTest();

    /** 清理测试私钥副本；外层通道须先析构
    */
    virtual ~NetGatewayReceiveTest();

    /** 验证单轮预算、拒收重试、关闭和异常后无迟到交付
    */
    static void run();

    /** 推进真实业务TLS并把已产生的密文交给客户端
    @return 是否仍连接
    */
    virtual bool poll();

    /** 接纳OPEN并延迟响应，CREDIT仅用于本次有界测试
    @param [in] peer 实际管理代次7
    @param [in] frame 公开管理帧
    @return NI_OK或错误
    */
    virtual int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 将客户端真实密文交给服务端TLS
    @param [in] peer 管理代次7
    @param [in] record 完整借用记录
    @return NI_OK或错误
    */
    virtual int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 记录实际TLS认证完成
    @param [in] peer 业务代次1
    */
    virtual void onReady(uint64_t peer);

    /** 核对正文和序号，按场景拒收或在回调内关闭
    @param [in] peer 业务代次1
    @param [in] frame 解密后的实际帧
    @return 是否接纳
    */
    virtual bool onFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 记录唯一终态
    @param [in] peer 业务代次1
    @param [in] reason 首个原因
    @param [in] detail 底层诊断
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);

public:
    // 当前测试的服务端私钥，仅本对象拥有
    std::string m_key;
    // 与私钥匹配的service.101证书
    std::string m_certificate;
    // 真实服务端TLS和NIF2封装
    std::unique_ptr<NetInterfaceConnection> m_server;
    // 借用外层通道，供回调关闭测试使用
    NetGatewayClientChannel* m_channel;
    // OPEN响应待交付
    bool m_open;
    // 拒收当前帧且保持借用
    bool m_reject;
    // 0正常、1先拒收、2回调关闭、3回调抛出
    uint32_t m_mode;
    // 已收到就绪次数
    uint32_t m_ready;
    // 接纳的正文帧数
    uint32_t m_received;
    // 含拒收的回调次数
    uint32_t m_callbacks;
    // 唯一关闭次数
    uint32_t m_closed;
    // 首个关闭原因
    int32_t m_reason;
};