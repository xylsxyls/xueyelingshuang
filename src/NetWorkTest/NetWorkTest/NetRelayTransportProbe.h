#pragma once
#include "NetWork/NetWorkAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <deque>
#include <memory>
#include <string>
#include <vector>

// 端点拥有TLS；转发探针m_tls为空，只拥有两侧密文字节通道。
class NetRelayTransportProbe : public NetRelayTransportListener
{
public:
    /** 创建尚未连接的隔离宿主
    */
    NetRelayTransportProbe();

    /** 在监听状态析构前收敛运输层回调
    */
    ~NetRelayTransportProbe();

    /** 推进有界TLS输入输出，不解释业务内容
    */
    void progress();

    /** 保存本轮实际建立的透明运输连接号
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    */
    virtual void onRelayConnected(uint64_t peer);

    /** 核对外层归属并转发给伙伴，或有界缓存给本地TLS端点
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] record 回调期间借用的完整透明记录，背压时不消费
    @return true已消费；false表示伙伴未就绪或缓冲已满
    */
    virtual bool onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 记录当前透明运输关闭次数及异常原因
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] reason 本连接或服务的NI关闭原因
    */
    virtual void onRelayClosed(uint64_t peer, int32_t reason);

public:
    // 本探针拥有的实际透明TCP运输
    NetRelayTransport m_transport;
    // 借用配对转发探针，双方停止前保持有效
    NetRelayTransportProbe* m_partner;
    // 端点拥有的真实TLS状态，透明代理不解释其业务密文
    std::unique_ptr<NetInterfaceTls> m_tls;
    // 背压期间保留的独占透明输入块
    std::deque<std::vector<uint8_t>> m_input;
    // 当前首块已消费字节偏移
    uint32_t m_inputOffset;
    // 尚未转交输入的总字节预算
    uint32_t m_inputBytes;
    // 当前用例使用的真实运输代次，不以PID替代
    uint64_t m_peer;
    // 实际转交的透明字节数量，用于往返核对
    uint64_t m_forwarded;
    // 实际关闭通知次数，恢复时核对每个旧代次都已终止
    uint32_t m_closedCount;
    // 本轮诊断文本，仅记录测试失败，不保存身份秘密
    std::string m_error;
};

/** 两段真实TCP的端点TLS往返与无密钥中继
*/
void netRelayTlsTcpTest();

/** 真实Libuv宿主的有界队列、拒绝原子性、缓冲副本及FIFO回绕
*/
void netRelayQueueTcpTest();