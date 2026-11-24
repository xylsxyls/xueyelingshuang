#pragma once
#include "NetExecution.h"
#include "NetInterfaceTransport.h"

// 逻辑会话和每次新建通道共用的期限策略，各通道独自保存正在拒收的帧期限。
struct NetGatewayClientTiming
{
    // 从管理连接到业务就绪的总毫秒数，0采用三倍有效TLS握手期限
    uint32_t m_startupMs = 0;
    // 连续拒收同一应用帧的最长毫秒数
    uint32_t m_backpressureMs = 5000;
    // 一次推进最多交付的解密业务帧数
    uint32_t m_readFrames = 32;
    // 一次推进最多交给运输的密文记录数
    uint32_t m_writeRecords = 32;

    /** 同时配置两种推进预算
    @param [in] readFrames 接收帧数，必须为正
    @param [in] writeRecords 发送记录数，必须为正
    @return NI_OK或NI_INVALID_ARGUMENT，错误时两值保持
    */
    int32_t configurePollBudgets(uint32_t readFrames, uint32_t writeRecords);

    /** 按接收预算依次交付，空闲、背压或失败应立即停止本轮
    @param [in,out] step 收到轮内序号；成功消费一条完整帧返回true
    */
    template<class Step> void readFrames(const Step& step) const;

    /** 按发送预算依次交付，成功交给运输后才计入本轮
    @param [in,out] step 收到轮内序号；成功消费一段密文返回true
    */
    template<class Step> void writeRecords(const Step& step) const;

    /** 复制两个时间预算，错误时原设置仍有效
    @param [in] startupMs 启动毫秒数，0采用自动值
    @param [in] backpressureMs 业务拒收毫秒数，必须非零
    @return NI_OK或NI_INVALID_ARGUMENT
    */
    int32_t configure(uint32_t startupMs, uint32_t backpressureMs);

    /** 对一个已解密的完整帧尝试一次交付，在首次拒收时登记固定期限
    @param [in,out] listener 当前业务观察者，回调只借用本帧
    @param [in] frame 本条逻辑通道的应用帧
    @param [in] now 单调毫秒
    @param [in,out] deadline 本通道当前帧的绝对期限，0表示首次尝试
    @return NI_OK已接纳、NI_LIMIT_EXCEEDED等待重试、NI_TIMEOUT已到期
    */
    int32_t deliver(NetTransportListener& listener, const NetInterfaceFrame& frame, uint64_t now, uint64_t& deadline) const;
};

#include "NetGatewayClientTiming.inl"