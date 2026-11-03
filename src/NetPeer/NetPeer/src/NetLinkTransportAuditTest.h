#pragma once
#include "NetPeer.h"
#include "NetRelayTransportTest.h"

// 单实例原始宿主替身；真实管理TLS由被测层处理
class NetLinkTransportAuditDriver : public NetRelayTransportTestDriver
{
public:
    /** 初始化暂停标志及测试随机种子
    */
    NetLinkTransportAuditDriver();

    /** 记录真实读取门控，供测试按约交付最后输入
    @param [in] instance 当前测试宿主
    @param [in] peer 当前代次
    @param [in] paused 0恢复、1暂停
    @return 原始宿主结果
    */
    static int32_t pause(void* instance, uint64_t peer, int32_t paused);

    /** 可预测测试随机，禁止用于产品或任何真实身份签发
    @param [in] context 当前测试宿主
    @param [out] output 有效缓冲
    @param [in] size 字节数
    @return 0
    */
    static int32_t random(void* context, uint8_t* output, uint32_t size);

public:
    // 当前读取门控，暂停期间不注入后续输入
    bool m_paused;
    // 仅供确定性测试的随机状态
    uint32_t m_seed;
};

// 验证管理与密文尾包、终态顺序，并注入背压/重入/异常
class NetLinkTransportAuditListener : public NetLinkTransportListener
{
public:
    /** 初始化计数和借用指针
    */
    NetLinkTransportAuditListener();

    /** 记录经过真实管理认证的就绪事件
    @param [in] peer 代次
    @param [in] identity 借用认证身份
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 检查管理尾包并按模式请求关闭
    @param [in] peer 代次
    @param [in] frame 完整认证帧
    @return 已接纳
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 检查业务密文尾包，按模式拒绝、关闭或抛异常
    @param [in] peer 代次
    @param [in] record 完整外层记录
    @return 是否接纳
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 检查资源先删除再唯一通知，回调可重入停止
    @param [in] peer 已结束代次
    @param [in] reason 最终原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);

public:
    // 已认证通知数
    uint32_t m_ready;
    // 实际接纳管理帧数
    uint32_t m_control;
    // 实际接纳业务记录数
    uint32_t m_records;
    // 唯一关闭通知数
    uint32_t m_closed;
    // 最终NI原因
    int32_t m_reason;
    // 被测实例，借用到关闭完成
    NetLinkTransport* m_client;
    // 测试场景选择，由main说明
    uint32_t m_mode;
    // 业务回调次数，包含背压重试
    uint32_t m_attempts;
    // 借用数据、身份和关闭顺序检查结果
    bool m_valid;
};