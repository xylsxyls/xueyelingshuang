#pragma once
#include "NetWork/NetWorkAPI.h"
#include <map>
#include <set>
#include <vector>

// 由测试拥有的完整帧快照，不保存借用指针。
class NetInterfaceTransportMessage
{
public:
    // 线字段，payload指针始终清空
    NetInterfaceFrame m_frame;
    // 自有二进制消息体
    std::vector<uint8_t> m_payload;
};

// 实际TLS帧运输的观察者，可受控注入背压和回调异常
class NetInterfaceTransportProbe : public NetTransportListener
{
public:
    /** 初始化受控测试事件接收者
    */
    NetInterfaceTransportProbe();

    /** 记录TLS就绪连接号
    @param [in] peer 连接号
    */
    virtual void onReady(uint64_t peer);

    /** 可控背压/异常/关闭，接纳时保存完整快照并可回显
    @param [in] peer 连接号
    @param [in] frame 借用帧
    @return 是否接纳，false时不记录消息或发送
    */
    virtual bool onFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 记录终态次数和原因
    @param [in] peer 连接号
    @param [in] reason 原因
    @param [in] detail TLS诊断
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);

public:
    // 回调中使用的借用传输，实例由测试外层拥有
    NetTransport* m_transport;
    // 回显、暂停、抛出和在回调内停止开关
    bool m_echo;
    // 受控拒收业务帧，验证上游背压和恢复
    bool m_hold;
    // 受控回调异常开关，只供异常隔离用例
    bool m_throw;
    // 在帧回调中请求停止，用于验证不递归销毁对象
    bool m_stopOnFrame;
    // 包含被拒绝尝试的次数
    uint32_t m_attempts;
    // 完整帧和终态证据
    std::vector<NetInterfaceTransportMessage> m_received;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    std::set<uint64_t> m_ready;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    std::map<uint64_t, uint32_t> m_closed;
    // 各运输代次实际终态原因
    std::map<uint64_t, int32_t> m_reasons;
};

// TLS运输和回调异常隔离测试集合，逐用例独立建立预期并检查实际结果
class NetInterfaceTransportTestCases
{
public:
    /** 实际NetWork DLL组合TCP/TLS/帧，1MiB二进制及背压恢复
    */
    static void nativeTransport();

    /** 错误域名、真实握手截止时间、未认证断流
    */
    static void transportFailures();

    /** 回调异常、过时连接号和回调内关闭的唯一终态
    */
    static void transportLifetime();
};