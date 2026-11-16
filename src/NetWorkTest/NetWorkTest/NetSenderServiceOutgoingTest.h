#pragma once
#include "NetSender/NetSenderServiceOutgoingTransport.h"

class NetSenderServiceGateway;

// 生命周期白盒夹具只观察真实适配器；真实认证和转发由进程集成用例覆盖。
class NetSenderServiceOutgoingTest : public NetLinkTransportListener
{
public:
    /** 借用单次测试适配器，覆盖全部回调
    @param [in,out] transport 实际服务出站适配器
    */
    explicit NetSenderServiceOutgoingTest(NetSenderServiceOutgoingTransport& transport);

    /** 核对身份借用，可按测试模式停止或抛异常
    @param [in] peer 当前出站代次
    @param [in] identity 宿主已认证管理身份
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 观察逻辑号恢复及背压语义
    @param [in] peer 出站代次
    @param [in] frame 回调内借用的控制帧
    @return 是否接纳
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 观察业务密文逻辑号恢复
    @param [in] peer 出站代次
    @param [in] record 回调内借用的密文
    @return 是否接纳
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 核对唯一终态；可按测试模式抛异常
    @param [in] peer 已结束代次
    @param [in] reason 终止原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);

    /** 建立管理已经就绪的单线程夹具，不伪造网络握手或业务成功
    @param [in,out] gateway 本轮拥有的服务状态
    */
    static void prepare(NetSenderServiceGateway& gateway);

    /** 验证发送边界、生命周期及回调；Debug隔离main还注入分配失败
    @return 是否全部通过
    */
    static bool run();

public:
    // 借用至本轮测试析构
    NetSenderServiceOutgoingTransport* m_transport;
    // 模式：0正常、1就绪抛异常、2就绪停止、3背压、4控制停止、5/6/7控制/密文/关闭抛异常
    uint32_t m_mode;
    // 已观察到的就绪次数
    uint32_t m_ready;
    // 已观察到的终态次数
    uint32_t m_closed;
    // 已观察到的控制帧次数
    uint32_t m_controls;
    // 已观察到的密文次数
    uint32_t m_records;
    // 验证失败标记
    bool m_bad;
};