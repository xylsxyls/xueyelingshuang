#pragma once
#include "NetClientSession.h"
#include <memory>
#include <vector>

// 恢复协调的唯一阶段；PENDING尚无物理尝试，BACKOFF的上一代已经结算。
enum NetClientSessionPhase
{
    // 尚未启动服务连接，不持有本轮物理运输
    NET_CLIENT_IDLE,
    // 等待发现或创建首轮／恢复运输，尚未发起物理尝试
    NET_CLIENT_PENDING,
    // 本轮物理运输、TLS或业务握手正在建立
    NET_CLIENT_CONNECTING,
    // 本代业务会话已完成握手，可接纳业务发送
    NET_CLIENT_READY,
    // 上一代已结算，等待带抖动的有界恢复退避
    NET_CLIENT_BACKOFF,
    // 连接尝试取消后等待下层终态及资源收敛
    NET_CLIENT_CANCELLING,
    // 已主动停止恢复，正在收敛活动会话与回调
    NET_CLIENT_STOPPING,
    // 本逻辑会话已停止，无待推进的恢复工作
    NET_CLIENT_CLOSED
};

// 所有字段仅由同一poll执行者维护，不创建网络/IPC/线程/文件资源
class NetClientSessionState
{
public:
    /** 初始化静止状态和全部标量，不请求熵或物理传输
    */
    NetClientSessionState();

    /** 是否有待尝试、活动连接、退避或待交付终态
    @return IDLE/CLOSED为false
    */
    bool active() const;

    /** 服务模式是否仍允许恢复，主动关闭后为false
    @return 是否允许推进服务恢复
    */
    bool recovering() const;

    /** 检查固定总期限，不在重试或迟到ready时续期
    @param [in] nowMs 当前单调毫秒
    @return 总期限非零且已到达
    */
    bool expired(uint64_t nowMs) const;

public:
    // 本轮尝试拥有的运输，由借用工厂创建
    std::unique_ptr<NetGatewayTransport> m_transport;
    // 本轮业务TLS协议，借用上面的运输
    std::unique_ptr<NetGatewayClientChannel> m_channel;
    // 必须先析构协议会话，再释放它借用的通道/运输
    NetSession m_session;
    // 旧协议借用通道，停止/更换后清空，不拥有
    NetClientChannel* m_external;
    // 已绑定的默认运输工厂，借用至解除绑定
    NetGatewayTransportFactory* m_providerFactory;
    // 已绑定的默认发现器，借用至解除绑定
    NetClientDiscovery* m_providerDiscovery;
    // 当前尝试实际采用的工厂
    NetGatewayTransportFactory* m_factory;
    // 当前逻辑服务实际采用的发现器，可空
    NetClientDiscovery* m_discovery;
    // 借用至shutdown的业务观察者
    NetSessionListener* m_listener;
    // 只保存强随机上下文及期限，不接受外部根或动态服务身份
    NetInterfaceTlsOptions m_tls;
    // 本逻辑会话固定的公开能力/设备声明
    NetInterfaceHandshakeOptions m_handshake;
    // 已验证的启动地址提示
    std::vector<NetGatewayAddress> m_seeds;
    // 可选持久化提示，本层不访问
    std::string m_cachePath;
    // 当前候选，地址不作为身份
    NetClientDiscoveryPoint m_destination;
    // 逻辑尝试代次，包括发现失败；不依赖物理适配器是否成功分配
    uint64_t m_generation;
    // 下次允许尝试的单调毫秒
    uint64_t m_retryAt;
    // 恢复总期限，0表示已就绪
    uint64_t m_recoveryDeadline;
    // 退避上限250至5000毫秒
    uint32_t m_delay;
    // 逻辑目标，0的权限仍由服务角色判断
    uint32_t m_product;
    // 唯一协调阶段，避免运行/尝试/已关闭布尔值互相矛盾
    NetClientSessionPhase m_phase;
    // 是否采用服务模式
    bool m_service;
    // 是否在上层回调
    bool m_callback;
    // 是否在poll中
    bool m_polling;
    // 主动关闭首个原因
    int32_t m_closeReason;
};