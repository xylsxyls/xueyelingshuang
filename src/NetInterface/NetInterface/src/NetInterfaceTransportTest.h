#pragma once
#include "NetInterface.h"

// 只为同名cpp的禁用main提供最小独立消费者，不实现业务协议
class NetInterfaceTransportTestChannel : public NetClientChannel
{
public:
    /** 以明确不支持模拟没有物理传输的消费者
    @param [in] address 未使用
    @param [in] port 未使用
    @param [in] options 未使用
    @param [in] listener 未借用
    @param [out] peer 清零
    @return NOT_SUPPORTED
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint64_t& peer);

    /** 无物理资源
    @return false
    */
    virtual bool poll();

    /** 不接纳任何输出
    @param [in] peer 未使用
    @param [in] frame 未使用
    @return NOT_SUPPORTED
    */
    virtual int32_t send(uint64_t peer, const NetInterfaceFrame& frame);

    /** 没有资源，不伪造终态
    @param [in] peer 未使用
    @param [in] reason 未使用
    */
    virtual void close(uint64_t peer, int32_t reason);

    /** 没有物理资源需要回收
    */
    virtual void shutdown();
};

// 独立观察未接纳连接没有业务回调，不实现会话算法
class NetInterfaceTransportTestListener : public NetSessionListener, public NetTransportListener
{
public:
    /** 初始化零事件计数
    */
    NetInterfaceTransportTestListener();

    /** 统计ready
    @param [in] generation 代次
    @param [in] maxRequests 额度
    @param [in] maxChunk 块大小
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 统计请求终态
    @param [in] generation 代次
    @param [in] requestId 请求
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status 结果
    @param [in] data 借用数据
    @param [in] size 长度
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type,
        uint32_t status, const uint8_t* data, uint32_t size);

    /** 统计事件并拒绝，不接纳字节
    @param [in] generation 代次
    @param [in] frame 帧
    @return false
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 统计会话终态
    @param [in] generation 代次
    @param [in] reason 原因
    @param [in] detail 诊断
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 仅供单元测试的确定性输入，不用于真实TLS
    @param [in] context 测试模式uint32_t，0完成、1部分写失败、2部分写异常
    @param [out] output size有效字节
    @param [in] size 长度
    @return 0或-1，模式2抛出异常
    */
    static int32_t entropy(void* context, uint8_t* output, uint32_t size);

    /** 记录不应发生的原始就绪事件
    @param [in] peer 代次
    */
    virtual void onReady(uint64_t peer);

    /** 记录不应发生的原始消息
    @param [in] peer 代次
    @param [in] frame 消息
    @return false
    */
    virtual bool onFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 记录不应发生的原始终态
    @param [in] peer 代次
    @param [in] reason 原因
    @param [in] detail 诊断
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);

public:
    // 全部上层回调数，未接纳连接应保持0
    uint32_t m_events;
};