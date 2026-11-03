#pragma once
#include "NetSenderServiceGateway.h"
#include <map>
#include <memory>
#include <vector>

// 服务管理代次唯一生命周期；客户端各自的TLS就绪不代替此注册状态。
enum NetSenderServiceGatewayPhase
{
    // 尚未绑定网关和业务身份
    NET_SERVICE_IDLE,
    // 正在建立管理认证
    NET_SERVICE_CONNECTING,
    // 管理认证成功，等待提交注册
    NET_SERVICE_AUTHENTICATED,
    // 注册请求已经接纳，等待网关确认
    NET_SERVICE_REGISTERING,
    // 网关确认服务可用
    NET_SERVICE_READY,
    // 停止接纳，正在回收客户端和出站依赖
    NET_SERVICE_CLOSING,
    // 已交付唯一服务终态，不允许复活
    NET_SERVICE_CLOSED
};

// 一个已授权逻辑客户端的独占业务TLS、额度及关闭状态
class NetSenderServiceGatewayClient
{
public:
    /** 独占每客户端业务TLS，身份材料由本连接复制
    @param [in] tls 业务服务端证书及私钥
    @param [in] role 网关认证来源角色
    */
    NetSenderServiceGatewayClient(const NetInterfaceTlsOptions& tls, uint32_t role);

    // 独占本客户端的业务TLS和帧解析器
    NetInterfaceConnection m_connection;
    // 本网关分配的双向密文额度
    NetInterfaceCredit m_credit;
    // 业务能力握手；与管理服务注册独立
    std::unique_ptr<NetInterfaceHandshake> m_handshake;
    // 尚未喂入TLS的有界密文，容量受本通道额度限制
    std::vector<uint8_t> m_input;
    // 握手校验后的来源身份，仅在就绪后公开
    NetInterfaceOrigin m_origin;
    // 最近业务活动的单调毫秒
    uint64_t m_lastReceive;
    // 持续背压起点，重试不延长期限
    uint64_t m_blockedSince;
    // 首次请求关闭的单调毫秒
    uint64_t m_closeSince;
    // 管理网关已认证的来源角色
    uint32_t m_role;
    // 本代首次终止原因
    int32_t m_reason;
    // 本客户端业务握手已完成
    bool m_ready;
    // 本客户端已停止接纳，等待统一回收
    bool m_closing;
    // 回收前是否还需通知远端关闭
    bool m_notifyRemote;
};

// 单执行者服务管理状态，拥有本机管理链路及各客户端业务会话
class NetSenderServiceGatewayState
{
public:
    /** 借用宿主收件箱
    @param [in] inbox 原IPC回调收件箱
    */
    explicit NetSenderServiceGatewayState(NetSenderLocalInbox& inbox);

    /** 擦除业务身份私钥
    */
    ~NetSenderServiceGatewayState();

    /** 单执行者推进全部管理与业务通道，不创建线程
    @return 是否仍有活动代次
    */
    bool poll();

    /** 是否已经停止接纳，包含终态
    @return 当前处于关闭阶段
    */
    bool closing() const;

    /** 推进注册、关闭请求及业务通道；异常由poll统一收敛
    @param [in] now 单调毫秒
    */
    void advance(uint64_t now);

    /** 无分配地摘除失效客户端；单个业务回调异常不能阻止其他通道回收
    @param [in] now 单调毫秒
    */
    void collectClosed(uint64_t now);

    /** 推进一个逻辑业务客户端
    @param [in] id 已授权逻辑号
    @param [in,out] client 客户端独占状态
    @param [in] now 单调毫秒
    */
    void pump(uint64_t id, NetSenderServiceGatewayClient& client, uint64_t now);

    /** 保存首次关闭原因，由poll负责通知和回收
    @param [in] id 逻辑号
    @param [in] reason 原因
    @param [in] remote 是否还需告知网关
    */
    void close(uint64_t id, int32_t reason, bool remote = true);

    /** 标记本轮管理接入终态，延后交付关闭回调
    @param [in] reason 原因
    */
    void fail(int32_t reason);

public:
    // 本服务的管理IPC链接，停止后才能销毁其回调目标
    NetSenderLink m_link;
    // 借用宿主监听者，覆盖服务停止及回调生命周期
    NetSenderServiceGatewayListener* m_listener;
    // 服务管理身份名称
    std::string m_name;
    // 独占私钥；失败、停止及析构时擦除
    std::string m_key;
    // 本次申请的公开证书请求
    std::string m_request;
    // 网关授予本服务的公开业务证书
    std::string m_certificate;
    // 已认证管理节点名称
    std::string m_nodeName;
    // 已认证管理节点公开证书
    std::string m_nodeCertificate;
    // 本管理代次拥有的客户端，最多512个
    std::map<uint64_t, std::unique_ptr<NetSenderServiceGatewayClient>> m_clients;
    // 业务明确允许压缩的消息类型，最多64个
    std::vector<uint32_t> m_compressionTypes;
    // 借用唯一的服务依赖出站适配器
    NetSenderServiceOutgoingTransport* m_outgoing;
    // 当前代次已经分配的最大出站逻辑号，不复用
    uint64_t m_highestOutgoing;
    // 等待提交关闭的出站逻辑号；零表示没有
    uint64_t m_outgoingClose;
    // 出站关闭持续背压起点
    uint64_t m_outgoingCloseSince;
    // 本次管理链接代次
    uint64_t m_peer;
    // 已经接纳的最大入站逻辑号，拒绝旧号复活
    uint64_t m_highestClient;
    // 本服务所属产品；零保留给公共账号服务
    uint32_t m_product;
    // 每业务端点在途请求上限
    uint32_t m_limit;
    // 允许同时服务的客户端数量
    uint32_t m_channels;
    // 向网关注册的服务容量权重
    uint32_t m_weight;
    // 本代首次终止原因
    int32_t m_reason;
    // 唯一服务注册/停止阶段，不能同时就绪且关闭
    NetSenderServiceGatewayPhase m_phase;
    // 单执行者回调重入守卫，不是跨线程同步锁
    bool m_polling;
};