#pragma once
#include "NetSender/NetSenderServiceGateway.h"

class NetSenderServiceGatewayState;

// 同一测试执行者观察真实服务状态；回调故意重入/抛异常，不模拟状态机和资源回收。
class NetSenderServiceGatewayTest : public NetSenderServiceGatewayListener
{
public:
    /** 借用覆盖测试生命周期的真实状态
    @param [in,out] state 实际服务管理状态
    */
    explicit NetSenderServiceGatewayTest(NetSenderServiceGatewayState& state);

    /** 隔离生命周期测试未联网，任何就绪通知都属于错误
    */
    virtual void onGatewayServiceReady();

    /** 本测试不伪造业务握手
    @param [in] channel 未预期连接
    @param [in] role 未预期角色
    @param [in] origin 未预期来源
    */
    virtual void onGatewayClientReady(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin);

    /** 本测试不投递业务正文
    @param [in] channel 未预期连接
    @param [in] role 未预期角色
    @param [in] frame 未预期帧
    @param [in] origin 未预期来源
    @return true消费并记录错误
    */
    virtual bool onGatewayClientFrame(uint64_t channel, uint32_t role, const NetInterfaceFrame& frame,
        const NetInterfaceOrigin& origin);

    /** 验证先删除后通知，可重入poll或故意抛出测试异常
    @param [in] channel 已回收逻辑号
    @param [in] reason 关闭原因
    */
    virtual void onGatewayClientClosed(uint64_t channel, int32_t reason);

    /** 验证所有客户端已经结束且服务状态不可复活
    @param [in] reason 首次终止原因
    */
    virtual void onGatewayServiceClosed(int32_t reason);

    /** 在不联网的真实状态中逐次注入失败，正式进程另由集成测试覆盖
    @return 是否通过
    */
    static bool run();

    /** 验证回调时间晚于本轮时间及45秒精确空闲边界
    @return 没有下溢误关闭且真实期限仍生效
    */
    static bool clockBoundary();

public:
    // 借用到run本轮结束
    NetSenderServiceGatewayState* m_state;
    // 只记录本轮实际通知数
    uint32_t m_clients;
    // 唯一服务终态计数
    uint32_t m_services;
    // 回调是否故意抛异常
    bool m_throw;
    // 是否存在顺序、重入或非预期回调
    bool m_bad;
};