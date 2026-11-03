#pragma once
#include "NetWork/NetWorkAPI.h"
#include "NetGatewayPeerHistory.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <deque>
#include <string>
#include <vector>

class NetGatewayNode;

// 一次选择只复制路由值，不保留容器迭代器或业务对象。
struct NetGatewayNodeSelection
{
    // 已认证目标运输代次
    uint64_t m_peer;
    // 目标业务实例
    uint64_t m_instance;
    // 目标节点本次启动标识
    uint64_t m_boot;
    // 目标可接纳的信用窗口
    uint32_t m_window;
};

// 一项本机或远端服务快照，只有所属NODE验证后可用于选择
struct NetGatewayNodeService
{
public:
    // 服务实例标识；跨节点选择时与节点启动代次共同校验
    uint64_t m_instance;
    // 服务所属产品号，0表示公共Account服务
    uint32_t m_product;
    // 可接纳逻辑通道的硬容量，负载选择不得超过此值
    uint32_t m_capacity;
    // 该服务单方向信用窗口字节数
    uint32_t m_window;
    // 服务选择权重，与当前占用和容量共同参与评分
    uint32_t m_weight;
    // 当前活跃或已预留的逻辑通道数
    uint32_t m_active;

public:
    /** 初始化未登记服务，默认信用窗口65536字节且不可参与负载选择
    */
    NetGatewayNodeService();
};

// 有界发现表中的成员记录，缓存提示不自动成为认证身份
struct NetGatewayNodeMember
{
public:
    // 成员公开地址、身份提示、容量及启动代次的完整快照
    NetInterfaceDirectoryPoint m_point;
    // 最近刷新此成员的单调毫秒，过期后移除动态地址
    uint64_t m_seen;
    // 本条信息是否来自该节点自己的已认证连接
    bool m_verified;

public:
    /** 初始化未经本节点直接认证的成员提示，最后发现时间为0
    */
    NetGatewayNodeMember();
};

// 区分配置种子与动态地址；动态候选只随当前有界成员表保留。
struct NetGatewayNodeCandidate
{
public:
    // 发现身份仅作为关联提示，仍验证实际节点的证书及挑战持有证明。
    std::string m_expectedName;
    // 单调重试截止，毫秒。
    uint64_t m_retryAt;
    // 当前退避预算，毫秒。
    uint32_t m_delay;
    // 用户显式配置的种子不因成员过期或移动而自动丢弃。
    bool m_seed;

public:
    /** 初始化未固定的动态候选及首轮退避
    */
    NetGatewayNodeCandidate();
};

// 一个主动NODE连接占有独立运输代次，回调映射为网关内唯一作用域。
class NetGatewayNodeConnection : private NetLinkTransportListener
{
public:
    /** 保存主动节点连接参数，尚不发起网络连接
    @param [in] owner 借用父节点，须覆盖全部回调及析构
    @param [in] id 父节点分配的唯一带作用域编号
    @param [in] address 数值IP，复制保存
    @param [in] port 管理端口
    @param [in] expectedName 复制的身份提示，不能代替证书验证
    */
    NetGatewayNodeConnection(NetGatewayNode& owner, uint64_t id, const std::string& address,
        uint32_t port, const std::string& expectedName);

    /** 停止拥有的运输适配；父节点仍须存活供关闭通知使用
    */
    virtual ~NetGatewayNodeConnection();

    /** 发起异步NODE管理连接
    @param [in] options 本节点身份及信任配置，由下层复制必要凭据
    @return NI_OK已启动或负数错误，此时尚未认证就绪
    */
    int32_t connect(const NetInterfaceLinkOptions& options);

    /** 由父节点唯一执行者推进连接、认证和关闭
    @return true仍有活动工作，false已停止
    */
    bool poll();

    /** 向此NODE的有界管理队列提交完整帧
    @param [in] frame 借用至返回，接纳时复制
    @return NI_OK已接纳、NI_LIMIT_EXCEEDED背压或负数错误
    */
    int32_t sendControl(const NetInterfaceFrame& frame);

    /** 向此NODE转交已有路由的透明密文
    @param [in] record 借用至返回，接纳时复制
    @return NI_OK已接纳、NI_LIMIT_EXCEEDED背压或负数错误
    */
    int32_t sendRecord(const NetInterfaceTunnelRecord& record);

    /** 请求下层关闭本连接，父节点在终态回调后回收
    @param [in] reason 关闭原因
    */
    void close(int32_t reason);

    /** 收敛本连接全部回调；回调中调用时由下层延迟到安全位置
    */
    void shutdown();

    // 拥有的数值连接地址，连接期间保持稳定
    std::string m_address;
    // 发现缓存提供的名称提示，最终信任以实际证书验证为准
    std::string m_expectedName;
    // 节点管理入口端口，1至65535
    uint32_t m_port;
    // 下层已报告此连接终止，父节点随后删除连接对象
    bool m_closed;
private:
    /** 处理管理握手确认的身份，不依据传输地址授予权限
    @param [in] peer 本适配器内的运输代次
    @param [in] identity 回调期间借用的已验证身份
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 按来源代次分派完整管理帧，背压时不消费
    @param [in] peer 已认证运输代次
    @param [in] frame 回调期间借用的管理帧
    @return true已处理或拒绝，false未消费且需稍后重试
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 按已建立路由转交透明密文，不解密业务正文
    @param [in] peer 来源运输代次
    @param [in] record 回调期间借用的完整记录
    @return true已接纳或拒绝，false无副作用背压
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 回收该代次的路由及关联状态，不影响其他物理连接
    @param [in] peer 已关闭运输代次
    @param [in] reason 下层关闭原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);
    // 借用父节点，连接关闭及回调收敛前保持有效
    NetGatewayNode* m_owner;
    // 拥有的主动节点连接适配，不与其他主动连接共用代次域
    NetLinkTransport m_transport;
    // 父节点分配的带主动连接作用域的唯一编号
    uint64_t m_id;
    // 本适配器内下层连接编号，回调必须与它匹配
    uint64_t m_peer;
};

// 将本机IPC代次转换为节点作用域的监听适配，借用父节点
class NetGatewayNodeLocalListener : public NetLinkTransportListener
{
public:
    /** 为同一个节点标记本机IPC代次，避免与TCP代次冲突
    @param [in] owner 借用节点
    */
    explicit NetGatewayNodeLocalListener(NetGatewayNode& owner);

    /** 处理管理握手确认的身份，不依据传输地址授予权限
    @param [in] peer 本适配器内的运输代次
    @param [in] identity 回调期间借用的已验证身份
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 按来源代次分派完整管理帧，背压时不消费
    @param [in] peer 已认证运输代次
    @param [in] frame 回调期间借用的管理帧
    @return true已处理或拒绝，false未消费且需稍后重试
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 按已建立路由转交透明密文，不解密业务正文
    @param [in] peer 来源运输代次
    @param [in] record 回调期间借用的完整记录
    @return true已接纳或拒绝，false无副作用背压
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 回收该代次的路由及关联状态，不影响其他物理连接
    @param [in] peer 已关闭运输代次
    @param [in] reason 下层关闭原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);

private:
    // 借用父节点，本机监听停止并收敛后才释放父对象
    NetGatewayNode* m_owner;
};

// 拥有正文的待发管理帧，发送时重新绑定正文指针
struct NetGatewayNodeControl
{
public:
    // 复制的管理帧头，发送时再绑定自有正文地址
    NetInterfaceFrame m_frame;
    // 自有管理正文，不借用调用方临时缓冲
    std::string m_payload;

public:
    /** 拥有管理消息体，保存期间不借用输入帧
    @param [in] frame 管理头
    @param [in] payload 自有protobuf消息体
    */
    NetGatewayNodeControl(const NetInterfaceFrame& frame, const std::string& payload);
};

// 一个已认证物理连接的管理水位、角色、负载及输出队列
struct NetGatewayNodePeer
{
public:
    // 本物理连接已验证的NI_LINK角色
    uint32_t m_role;
    // 本物理连接已验证的身份名称副本
    std::string m_name;
    // 已处理输入管理请求号的不可回退水位
    uint64_t m_request;
    // 已接纳输入逻辑通道号的单调水位
    uint64_t m_highestChannel;
    // 本物理代次四个编号域的不可复用水位；不积累每条关闭记录。
    NetGatewayPeerHistory m_channelHistory;
    // 本节点在此连接上已发出的最大管理请求号
    uint64_t m_outRequest;
    // 本节点向对端申请的最大逻辑通道号，断开前不复用
    uint64_t m_outChannel;
    // 当前等待的节点发现请求号，0表示无在途发现
    uint64_t m_discoverRequest;
    // 最近提交发现请求的单调毫秒，用于间隔及5秒超时
    uint64_t m_discoverAt;
    // 最近采纳有效远端服务快照的单调毫秒
    uint64_t m_snapshotReceived;
    // 最近采纳的远端服务快照版本，只允许递增
    uint64_t m_snapshotRevision;
    // 最近向此连接提交的本机快照版本
    uint64_t m_snapshotSent;
    // 管理输出连续背压起点，单调毫秒；0表示未阻塞
    uint64_t m_blockedSince;
    // 对端NODE自己的公开节点信息，须匹配已认证身份
    NetInterfaceDirectoryPoint m_node;
    // 对端NODE公布的本机服务快照，不接受转抄的第三方服务
    std::vector<NetGatewayNodeService> m_remoteServices;
    // 服务所属产品号，0表示公共Account服务
    uint32_t m_product;
    // 可接纳逻辑通道的硬容量，负载选择不得超过此值
    uint32_t m_capacity;
    // 该服务单方向信用窗口字节数
    uint32_t m_window;
    // 服务选择权重，与当前占用和容量共同参与评分
    uint32_t m_weight;
    // 当前活跃或已预留的逻辑通道数
    uint32_t m_active;
    // 此物理连接作为请求方已打开的逻辑通道数
    uint32_t m_opened;
    // 待发管理正文的累计字节数，用于独立队列限额
    uint32_t m_controlBytes;
    // SERVICE角色完成注册，可以参与实例选择
    bool m_serviceReady;
    // 已请求关闭该物理连接，拒绝新的管理和路由工作
    bool m_closing;
    // 自有管理帧FIFO，与业务数据使用独立限额
    std::deque<NetGatewayNodeControl> m_controls;

public:
    /** 创建未注册服务状态
    */
    NetGatewayNodePeer();
};

// 固定到一个服务实例的逻辑路由，失效后由端点重建新代次
struct NetGatewayNodeRoute
{
public:
    // 客户端到服务端方向的信用账户，透传不能凭空增额
    NetInterfaceCredit m_clientWindow;
    // 服务端到客户端方向的独立信用账户
    NetInterfaceCredit m_serviceWindow;
    // 路由客户端一侧的带作用域物理代次
    uint64_t m_client;
    // 客户端一侧声明并已验证归属的逻辑通道号
    uint64_t m_clientChannel;
    // 服务端一侧的带作用域物理代次
    uint64_t m_service;
    // 服务端一侧映射后的逻辑通道号
    uint64_t m_serviceChannel;
    // 服务所属产品号，0表示公共Account服务
    uint32_t m_product;
    // 服务实例标识；跨节点选择时与节点启动代次共同校验
    uint64_t m_instance;
    // 等待远端节点OPEN回执时的管理请求号
    uint64_t m_remoteRequest;
    // 所选目标节点的启动代次，避免重启后复用旧实例
    uint64_t m_boot;
    // 等待远端OPEN完成的单调毫秒截止时间
    uint64_t m_deadline;
    // 远端成功接纳后，包含该连接的最早快照；此前仍计额外预留
    uint64_t m_loadRevision;
    // 发起方原OPEN帧头，回执必须保留原请求关联
    NetInterfaceFrame m_original;
    // 已预留目标容量但尚未收到远端OPEN成功回执
    bool m_pending;
    // One credit window per direction. A blocked destination must not pin the
    // shared NODE input decoder and prevent unrelated channels from progressing.
    std::deque<std::vector<uint8_t>> m_forwardQueue;
    // 服务端到客户端的有界密文FIFO，与正向队列分开计额
    std::deque<std::vector<uint8_t>> m_reverseQueue;
    // 正向当前队头首次发送受阻的单调毫秒，0表示无阻塞
    uint64_t m_forwardBlocked;
    // 反向当前队头首次发送受阻的单调毫秒，0表示无阻塞
    uint64_t m_reverseBlocked;

public:
    /** 初始化无归属且未预留容量的空路由，两方向队列为空
    */
    NetGatewayNodeRoute();
};