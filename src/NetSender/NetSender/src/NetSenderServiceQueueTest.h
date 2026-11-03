#pragma once
#include "NetSender/NetSenderService.h"
#include <vector>

class NetSenderServiceManaged;

// 只模拟业务回调是否接纳，排队／预算／超时及恢复使用真实服务实现。
class NetSenderServiceQueueTest : public NetSenderServiceListener
{
public:
    /** 初始让来源41背压，来源42始终接纳；同一执行者访问全部观察状态。
    */
    NetSenderServiceQueueTest();

    /** 本用例不建立网关，记录非预期就绪回调
    @param [in] endpoint 非预期代次
    */
    virtual void onServiceReady(uint64_t endpoint);

    /** 记录非预期关闭回调
    @param [in] reason 非预期关闭原因
    */
    virtual void onServiceClosed(int32_t reason);

    /** 背压不产生业务副作用；接纳时记录实际收到的正文序号
    @param [in] frame 实际服务队列交出的帧
    @param [in] origin 本条源端点
    @return 是否接纳
    */
    virtual bool onServiceFrame(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 同来源100条消息超过32条单次预算，另一来源仍须前进，恢复后严格FIFO。
    */
    static void run();

    /** 在真实回调内填满队列、关闭及抛异常，借用必须继续占预算且正文有效
    */
    static void reentrant();

    /** 真实路由与两个本地索引一致登记；失败后能释放和重新登记，关闭不分配
    */
    static void registration();
private:
    // 0执行FIFO，1条数重入，2字节重入，3回调关闭，4回调异常
    uint32_t m_mode;
    // 仅测试调用期间借用，观察者先于受测服务销毁
    NetSenderServiceManaged* m_service;
    // 回调内实际接纳数量
    uint32_t m_admitted;
    // 业务终态回调次数
    uint32_t m_closedCount;
    // 本轮由run在调用之间切换，不与回调并发。
    bool m_allow;
    // 健康来源的实际接纳数量。
    uint32_t m_healthy;
    // 受控来源的实际接纳顺序，不用提交日志代替到达记录。
    std::vector<uint8_t> m_seen;
    // 格式、身份或非预期生命周期回调。
    bool m_bad;
};