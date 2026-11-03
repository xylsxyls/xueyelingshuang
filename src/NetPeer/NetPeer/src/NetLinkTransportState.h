#pragma once
#include "NetLinkTransport.h"
#include "NetRelayTransport.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// 一个运输代次的管理协议状态，禁止跨执行者直接访问
class NetLinkTransportPeer
{
public:
    /** 复制独立管理状态
    @param [in] options 当前身份配置
    */
    explicit NetLinkTransportPeer(const NetInterfaceLinkOptions& options);

public:
    // 本代管理认证、控制帧与透明数据状态；只由所属poll执行者访问
    NetInterfaceLink m_link;
    // 下层已报告连接就绪，允许推进管理握手
    bool m_connected;
    // 已向监听者报告本代认证就绪，防止重复通知
    bool m_announced;
    // 已确定终态并请求下层关闭，不再接纳输出
    bool m_closing;
    // 原始运输已经收敛；保留协议状态到已接纳输入排空
    bool m_closed;
    // 上层首次拒绝输入的单调毫秒；0表示当前未背压，连续5秒后关闭
    uint64_t m_blockedSince;
    // 本代保存的关闭原因，原始运输失败优先保留
    int32_t m_reason;
};

// 管理连接集合的唯一执行者；拥有下层适配和每代协议状态
class NetLinkTransportState : public NetRelayTransportListener
{
public:
    /** 创建未启动状态
    */
    NetLinkTransportState();

    /** 擦除身份私钥副本
    */
    ~NetLinkTransportState();

    /** 复制校验配置，随机为空时使用原生系统强随机
    @param [in] options 原始配置
    @param [in] listener 借用宿主
    @return NI_OK或错误
    */
    int32_t configure(const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener);

    /** 推进单个代次，最多64条管理/业务及64条外层输出
    @param [in] id 代次
    @param [in,out] peer 独占状态
    */
    void pump(uint64_t id, NetLinkTransportPeer& peer);

    /** 标记关闭，保持首个原因
    @param [in] id 代次
    @param [in] reason 原因
    */
    void close(uint64_t id, int32_t reason);

    /** 删除排空结束且原始运输已关闭的代次，再回报唯一终态
    */
    void collectClosed();

    /** 创建或启用本代管理状态，接纳失败只关闭该连接
    @param [in] peer 已成功建立的运输代次
    */
    virtual void onRelayConnected(uint64_t peer);

    /** 将下层完整记录交给本代管理协议；不在此回调执行宿主业务
    @param [in] peer 下层代次
    @param [in] record 回调期间借用，接纳时复制
    @return false表示有界背压且未消费；true已接纳或已拒绝此代
    */
    virtual bool onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 记录原始运输终态，保留已接纳输入供后续poll排空
    @param [in] peer 原运输代次，未知或已关闭代次忽略
    @param [in] reason 下层关闭原因
    */
    virtual void onRelayClosed(uint64_t peer, int32_t reason);

public:
    // 拥有的底层透明运输适配，生命周期覆盖全部管理通道
    NetRelayTransport m_raw;
    // 借用宿主监听者，必须存活到shutdown返回
    NetLinkTransportListener* m_listener;
    // 校验后的身份配置；字符串和凭据指向本状态拥有的副本
    NetInterfaceLinkOptions m_options;
    // 本端声明名称的独占副本
    std::string m_name;
    // 本端节点证书副本；非NODE角色不保留该证书
    std::vector<uint8_t> m_certificate;
    // 本端身份私钥副本，析构前主动擦除
    std::vector<uint8_t> m_privateKey;
    // 服务角色声明的独占副本
    std::vector<uint8_t> m_claims;
    // 运输代次到独占管理状态；删除前等待原始运输关闭及输入收敛
    std::map<uint64_t, std::unique_ptr<NetLinkTransportPeer>> m_peers;
    // 已消费一次配置机会，失败后不复用半初始化状态
    bool m_started;
    // 停止接纳标志；回调中只标记，外层poll退出后收敛
    bool m_stopping;
    // 单执行者重入保护，不是跨线程同步原语
    bool m_polling;
    // 当前新连接接纳窗口起点，单调毫秒
    uint64_t m_acceptWindow;
    // 本秒新接纳连接数；超过16拒绝新连接，不影响既有连接
    uint32_t m_acceptCount;
};