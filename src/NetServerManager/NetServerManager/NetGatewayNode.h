#pragma once
#include "NetWork/NetWorkAPI.h"
#include "NetSender/NetSenderAPI.h"
#include "NetGatewayNodeState.h"
#include <map>
#include <string>

// 一个独立网关节点的管理/密文入口。业务身份和通道由后续注册/路由表决定。
class NetGatewayNode : private NetLinkTransportListener
{
    friend class NetGatewayNodeLocalListener;
    friend class NetGatewayNodeConnection;
    friend class NetGatewayNodeLoadTest;
public:
    /** 复制本机节点身份，不在构造启动网络
    @param [in] name 唯一节点证书名
    @param [in] certificate 内置根签发的公开叶证书
    @param [in] privateKey 节点独立私钥，不是CA私钥
    */
    NetGatewayNode(const std::string& name, const std::string& certificate, const std::string& privateKey);

    /** 在监听状态析构前收敛回调并擦除私钥副本
    */
    ~NetGatewayNode();

    /** 启动NTW3公开管理入口；不允许公共服务注册
    @param [in] address 数值监听地址
    @param [in] port 0用于隔离测试
    @param [in] advertise 数值可达地址，不能发布0.0.0.0/::
    @param [in] capacity 1至256个连接
    @return NI_OK或错误
    */
    int32_t listen(const char* address, uint32_t port, const std::string& advertise, uint32_t capacity);

    /** 有界驱动，不创建线程
    @return 是否仍运行
    */
    bool poll();

    /** 停止全部所属连接
    */
    void shutdown();

    /** 实际入口端口
    @return 未监听时0
    */
    uint32_t localPort() const;

    /** 登记启动候选地址，不授予节点身份，不在调用中进行阻塞连接
    @param [in] address 数值IP
    @param [in] port 1至65535
    @return NI_OK或错误；重复地址不重复登记
    */
    int32_t addSeed(const std::string& address, uint32_t port);

    /** 启动阶段加载公开候选并准备异步原子缓存，不信任缓存中的身份
    @param [in] path identity目录中的nodes.cache完整UTF8路径
    @return NI_OK或错误；损坏/过期缓存丢弃并从种子重建
    */
    int32_t configureCache(const std::string& path);

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

    /** 回复一次管理请求，完整请求号保持原值
    @param [in] peer 管理代次
    @param [in] original 已验证请求
    @param [in] status 管理结果
    @param [in] bytes protobuf或空
    @return NI_OK、NI_LIMIT_EXCEEDED或错误
    */
    int32_t reply(uint64_t peer, const NetInterfaceFrame& original, int32_t status, const std::string& bytes);

    /** 本轮管理输出是否有独立剩余额度；先检查再建立路由
    @param [in] peer 管理代次
    @param [in] bytes 预计正文
    @return 是否能完整接纳
    */
    bool controlSpace(uint64_t peer, uint32_t bytes) const;

    /** 将完整控制帧复制进有界节点队列
    @param [in] peer 代次
    @param [in] frame 管理头
    @param [in] bytes protobuf正文
    @return NI_OK或错误
    */
    int32_t queueControl(uint64_t peer, const NetInterfaceFrame& frame, const std::string& bytes);

    /** 唯一执行者将已拥有的管理输出交给正确的TCP/IPC适配
    */
    void flushControls();

    /** Fairly drain bounded per-route ciphertext queues; retire only a stalled route.
    */
    void flushRecords();

    /** 按活跃及未反映接纳数挑选同产品健康实例，不改变任何计数
    @param [in] peer 请求方运输代次
    @param [in] product 目标产品
    @param [in] instance 非零时仅本机该精确实例，零时按负载选择
    @param [in] now 单调毫秒
    @param [out] output 成功时复制选择值；失败保持不变
    @return 是否存在尚有硬容量的健康实例
    */
    bool select(uint64_t peer, uint32_t product, uint64_t instance, uint64_t now,
        NetGatewayNodeSelection& output) const;

    /** 为逻辑通道挑选就绪实例并固定到结束
    @param [in] peer 请求方
    @param [in] frame OPEN管理请求
    @param [in] mesh 是否由节点请求精确本机实例
    @return NI_OK或错误
    */
    int32_t open(uint64_t peer, const NetInterfaceFrame& frame, bool mesh = false);

    /** 单执行者推进对等节点连接、快照、发现、迟到请求回收及候选退避
    */
    void pollMesh();

    /** 当前成员过期后回收其动态候选，不改变任何活跃业务路由
    @param [in] now 本轮固定单调毫秒
    */
    void expireMembers(uint64_t now);

    /** 仅在成员变化后裁去无关联的动态地址，显式种子始终保留
    */
    void pruneCandidates();

    /** 生成本节点本次启动的本机服务快照，不转抄其他节点服务声明
    @return NI_OK或错误
    */
    int32_t buildSnapshot();

    /** 只采纳已认证NODE自己的服务快照
    @param [in] peer 当前NODE代次
    @param [in] frame 有界管理事件
    @return 接纳或无副作用背压
    */
    bool snapshot(uint64_t peer, const NetInterfaceFrame& frame);

    /** 处理本节点曾发起的发现/精确实例OPEN回执
    @param [in] peer 已认证NODE代次
    @param [in] frame 完整管理回应
    @return 接纳或无副作用背压
    */
    bool meshResponse(uint64_t peer, const NetInterfaceFrame& frame);

    /** 处理邻居通知的既有逻辑通道终止，不终止该邻居其他路由
    @param [in] peer 已认证NODE代次
    @param [in] frame CHANNEL_CLOSE事件
    @return 接纳
    */
    bool meshClose(uint64_t peer, const NetInterfaceFrame& frame);

    /** 生成完整节点候选表，输出不截断
    @param [out] output protobuf
    @return NI_OK或错误
    */
    int32_t directory(std::string& output) const;

    /** 将验证完的地址提示加入有界成员/连接候选；提示不等于节点认证
    @param [in] point 合法完整条目
    @param [in] verified 是否来自该节点自己的认证连接
    @return NI_OK或错误
    */
    int32_t remember(const NetInterfaceDirectoryPoint& point, bool verified);

    /** 返回本节点的公开地址及本次启动代次
    @return 完整条目
    */
    NetInterfaceDirectoryPoint self() const;

    /** 向带作用域的TCP/IPC/主动节点运输排队管理帧
    @param [in] peer 网关代次
    @param [in] frame 借用管理帧
    @return NI_OK或错误
    */
    int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 按真实接收方回传额度，不能凭空扩大或改变通道归属
    @param [in] peer 已认证代次
    @param [in] frame CREDIT事件
    @return true接纳，false无副作用背压
    */
    bool credit(uint64_t peer, const NetInterfaceFrame& frame);

    /** 释放路由、反向索引及实例使用数，通知仍活跃端点
    @param [in] route 节点路由号
    @param [in] sender 已关闭或主动结束的一侧，0两侧均通知
    @param [in] reason 终止原因
    */
    void retire(uint64_t route, uint64_t sender, int32_t reason);

    /** 按带作用域的代次发送业务密文
    @param [in] peer 代次
    @param [in] record 借用密文
    @return NI_OK或错误
    */
    int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 标记关闭，不在回调中销毁运输适配
    @param [in] peer 带作用域代次
    @param [in] reason 原因
    */
    void close(uint64_t peer, int32_t reason);

    /** 查询非活跃编号是否已越过本物理代次的不可复用水位。
    @param [in] peer 带作用域的物理代次
    @param [in] channel 已经确认不在活跃路由索引中的通道
    @return 旧编号或不可再分配的旧间隙；未知未来编号仍拒绝
    */
    bool retired(uint64_t peer, uint64_t channel) const;

    /** 禁止复制节点身份、路由和运输所有权
    @param [in] other 不可复制节点
    */
    NetGatewayNode(const NetGatewayNode& other);

    /** 禁止复制节点身份、路由和运输所有权
    @param [in] other 不可复制节点
    @return 不可调用
    */
    NetGatewayNode& operator=(const NetGatewayNode& other);

private:
    // 拥有本机IPC收件箱，停止接收回调后才能析构
    NetSenderLocalInbox m_inbox;
    // 本机IPC代次作用域转换器，借用本节点
    NetGatewayNodeLocalListener m_localListener;
    // 本机服务登记及管理连接，先于收件箱销毁
    NetSenderLink m_local;
    // 公开TCP监听及被动NODE连接，不接受公共SERVICE首次注册
    NetLinkTransport m_transport;
    // 本节点已签证书中的唯一名称副本
    std::string m_name;
    // 内置信任根签发的节点公开证书副本
    std::string m_certificate;
    // 本节点独立私钥，析构前擦除，不包含CA私钥
    std::string m_privateKey;
    // 对其他节点公布的数值可达地址，不得为通配监听地址
    std::string m_advertise;
    // 最近序列化的本机服务快照，变更后重新生成
    std::string m_snapshot;
    // 本次节点启动的非零随机标识
    uint64_t m_boot;
    // 本机快照单调版本，溢出前停止生成
    uint64_t m_snapshotRevision;
    // 最近刷新本机快照的单调毫秒
    uint64_t m_snapshotAt;
    // 最近分配的主动节点连接号，按作用域保持唯一
    uint64_t m_lastOutbound;
    // 本机服务或占用变化后需更新快照
    bool m_meshDirty;
    // 公开TCP最大连接数，listen接纳1至256
    uint32_t m_capacity;
    // 最近分配的逻辑路由号，当前节点生命周期内不复用
    uint64_t m_lastRoute;
    // 全部路由排队密文的总字节数，用于全局内存上限
    uint32_t m_recordBytes;
    // 本节点拥有ProcessWork接收器，shutdown负责收敛
    bool m_receiving;
    // 已禁止新接入并进入统一停止过程
    bool m_stopping;
    // 带作用域的TCP、IPC和主动NODE连接状态表
    std::map<uint64_t, NetGatewayNodePeer> m_peers;
    // 节点路由号到固定服务实例的双向信用及队列状态
    std::map<uint64_t, NetGatewayNodeRoute> m_routes;
    // 物理代次与逻辑通道号到本机路由的双向索引
    std::map<std::pair<uint64_t, uint64_t>, uint64_t> m_channels;
    // 独占主动NODE连接，终态后才删除
    std::map<uint64_t, std::unique_ptr<NetGatewayNodeConnection>> m_outbound;
    // 节点名称到有界发现记录，未认证信息只作连接提示
    std::map<std::string, NetGatewayNodeMember> m_members;
    // 数值地址和端口到候选退避状态，显式种子不随过期删除
    std::map<std::pair<std::string, uint32_t>, NetGatewayNodeCandidate> m_candidates;
    // 远端NODE代次和OPEN请求号到等待确认的路由
    std::map<std::pair<uint64_t, uint64_t>, uint64_t> m_pendingOpens;
    // 公开节点候选的异步原子存储，不含认证私钥
    NetGatewayDirectoryStore m_cache;
    // 已成功打开候选缓存，允许后续异步读写
    bool m_cacheEnabled;
    // 候选信息变化后尚待提交缓存
    bool m_cacheDirty;
    // 最近提交缓存的单调毫秒，用于合并连续更新
    uint64_t m_cacheQueued;
};