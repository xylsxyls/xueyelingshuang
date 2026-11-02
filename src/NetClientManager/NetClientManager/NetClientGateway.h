#pragma once
#include "NetSender/NetSenderAPI.h"
#include "NetWork/NetWorkAPI.h"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <deque>

// 兼容TLS中继路径中一个本机端点的握手、限额及请求水位
struct NetClientGatewayPeer
{
public:
    // 本机端点的独占业务握手，TLS完成后创建
    std::unique_ptr<NetInterfaceHandshake> m_handshake;
    // 提前到达取消的请求号及产品/消息类型，用于拦截迟到请求
    std::map<uint64_t, std::pair<uint32_t, uint32_t>> m_cancelled;
    // 本机端点业务握手截止的单调毫秒
    uint64_t m_deadline;
    // 本代已接纳的最大请求号，禁止复用和倒退
    uint64_t m_highest;
    // 本机端点协商后的最大在途请求数
    uint32_t m_limit;
    // 该本机端点已接纳但尚未结算的请求数
    uint32_t m_pending;
    // 协商的数据块上限，字节
    uint32_t m_chunk;
    // 当前端点已使用的产品集合，限制跨产品事件分派
    std::set<uint32_t> m_products;
    // 本机端点完成应用握手，可以发送业务
    bool m_ready;

public:
    /** 初始化固定5秒握手期限
    @param [in] now 当前单调时间
    */
    explicit NetClientGatewayPeer(uint64_t now);
};

// 兼容中继路径的一项请求映射，正文和原关联均由网关拥有
struct NetClientGatewayRequest
{
    // 原本机请求头，回报结果时恢复其关联字段
    NetInterfaceFrame m_local;
    // 所属本机运输代次，不根据请求正文改变归属
    uint64_t m_peer;
    // 待发请求自有正文，异步转发不借用输入缓冲
    std::vector<uint8_t> m_body;
    // 请求已提交远端；断开后不能当作未发送请求重放
    bool m_sent;
};

// 兼容中继路径的文件归属记录，按本机代次和产品隔离
struct NetClientGatewayFile
{
public:
    // 所属本机运输代次，不根据请求正文改变归属
    uint64_t m_peer;
    // 此文件流所属产品号
    uint32_t m_product;
    // 复制的远端文件元数据，析构擦除其中传输秘密
    NetInterfaceFileInfo m_info;

public:
    /** 初始化已验证本机拥有者
    @param [in] peer 本机代次
    @param [in] product 产品
    @param [in] info 远端已映射元数据
    */
    NetClientGatewayFile(uint64_t peer, uint32_t product, const NetInterfaceFileInfo& info);

    /** 擦除秘密
    */
    ~NetClientGatewayFile();
};

// 一个执行者拥有两侧连接、编号映射和有界队列；本机PID不作为权限凭据。
class NetClientGateway : private NetSenderLocalListener, private NetSessionListener
{
  public:
    /** 创建未启动网关
    */
    NetClientGateway();

    /** 停止所有拥有的通道，ProcessWork回调收敛后释放inbox
    */
    ~NetClientGateway();

    /** 配置本机TLS监听并连接远端网关；均不降级明文
    @param [in] localTls 本机服务证书及私钥
    @param [in] remote 远端原生客户端v1配置，platform必须为桌面1
    @return OK启动，其余负数失败并回收
    */
    int32_t init(const NetInterfaceTlsOptions& localTls, const NetWorkClientOptions& remote);

    /** 同一执行者有界驱动并重试远端连接
    @return 未停止
    */
    bool poll();

    /** 停止接纳，结算请求，收敛ProcessWork，释放网络
    */
    void shutdown();

    /** 远端已就绪时才允许本机应用握手
    @return 联网就绪
    */
    bool ready() const;

  private:
    /** 使用系统强随机
    @param [in] context 忽略
    @param [out] output 字节
    @param [in] size 长度
    @return 0成功
    */
    static int32_t Entropy(void* context, uint8_t* output, uint32_t size);

    /** 单调时间
    @return 毫秒
    */
    static uint64_t Now();

    /** 重新连接已复制的远端配置
    @return OK启动或负数
    */
    int32_t connectRemote();

    /** 回报本机RPC错误/结果，发送队列无法接纳则关闭对应通道
    @param [in] peer 本机代次
    @param [in] original 原RPC字段
    @param [in] status RPC结果
    @param [in] body 成功业务体
    @param [in] size 长度
    */
    void respond(uint64_t peer, const NetInterfaceFrame& original, uint32_t status, const uint8_t* body = nullptr, uint32_t size = 0);

    /** 在本机TLS完成后准备握手期限
    @param [in] peer 真实通道
    @param [in] pid 对端声明，不用于授权
    */
    virtual void onLocalReady(uint64_t peer, int32_t pid);

    /** 本机已解密帧，预留远端号后才入有界映射
    @param [in] peer 来源通道
    @param [in] frame 完整帧
    @return 接纳状态
    */
    virtual bool onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 取消该通道的所有关联请求
    @param [in] peer 代次
    @param [in] reason 原因
    */
    virtual void onLocalClosed(uint64_t peer, int32_t reason);

    /** 记录真实远端就绪
    @param [in] generation 代次
    @param [in] maxRequests 限额
    @param [in] maxChunk 块大小
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 从远端号找回原本机请求及来源
    @param [in] generation 代次
    @param [in] requestId 远端原号
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status RPC状态
    @param [in] data 借用体
    @param [in] size 长度
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
                                 const uint8_t* data, uint32_t size);

    /** 已有流适配的入口，当前未登记的推送不得任意广播给本机程序
    @param [in] generation 代次
    @param [in] frame 推送或数据帧
    @return 是否接纳
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 断开本机代次以使应用观察到离线，并安排远端重连
    @param [in] generation 代次
    @param [in] reason 原因
    @param [in] detail TLS错误
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

  private:
    // 拥有本机有界收件箱，ProcessWork停止回调后才析构
    NetSenderLocalInbox m_inbox;
    // 本机TLS适配，借用m_inbox，停止后才能释放收件箱
    NetSenderLocal m_local;
    // 先停止协议会话，再销毁它借用的物理适配
    std::unique_ptr<NetTransport> m_remoteTransport;
    // 远端协议会话，借用先声明的物理运输适配
    NetSession m_remote;
    // 本机代次到独占握手和额度状态
    std::map<uint64_t, std::shared_ptr<NetClientGatewayPeer>> m_peers;
    // 远端请求号到原本机请求及自有待发正文
    std::map<uint64_t, NetClientGatewayRequest> m_requests;
    // 远端文件流号到本机拥有者及产品映射
    std::map<uint64_t, std::shared_ptr<NetClientGatewayFile>> m_files;
    // 待发的产品号与文件流号取消队列
    std::deque<std::pair<uint32_t, uint64_t>> m_fileCancels;
    // 保存的远端配置，地址/根/域名指向本对象副本
    NetWorkClientOptions m_options;
    // 远端网关连接地址副本
    std::string m_address;
    // 远端TLS验证域名副本
    std::string m_hostname;
    // 远端TLS信任根副本
    std::vector<uint8_t> m_authority;
    // 下次允许远端重连的单调毫秒
    uint64_t m_retry;
    // 待发请求正文累计字节数，不含已经发送的正文
    uint32_t m_queuedBytes;
    // 远端会话协商的最大在途请求数
    uint32_t m_remoteLimit;
    // 远端会话协商的数据块字节上限
    uint32_t m_remoteChunk;
    // 本对象已启动并拥有ProcessWork接收器
    bool m_receiving;
    // 网关本次启动已经成功进入工作阶段
    bool m_started;
    // 已停止接纳，正在结算并收敛本地和远端连接
    bool m_stopping;
};