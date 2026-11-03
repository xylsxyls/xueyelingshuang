#pragma once
#include "NetSenderService.h"
#include "NetSenderServiceGateway.h"
#include "NetSenderServiceOutgoingTransport.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetSenderServiceQueue.h"
#include <map>
#include <set>

class NetSenderServiceManaged;

// 路由器输出到服务组合有界队列的适配，不直接回调业务
class NetSenderServiceManagedSink : public NetEndpointServiceSink
{
public:
    /** 绑定路由器输出方向，不取得拥有者所有权
    @param [in] owner 须覆盖本接收入口生命周期
    @param [in] clients true客户端输出，false服务处理器或内部依赖
    */
    NetSenderServiceManagedSink(NetSenderServiceManaged& owner, bool clients);

    /** 将路由器输出交给所属有界队列
    @param [in] endpoint 目标路由端点
    @param [in] frame 本次借用，接纳时复制
    @param [in] origin 已认证来源
    @return NI_OK已入队、NI_LIMIT_EXCEEDED背压或负数错误
    */
    virtual int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 请求停止对应客户端或内部依赖，未知端点忽略
    @param [in] endpoint 目标路由端点
    */
    virtual void close(uint64_t endpoint);

private:
    // 借用，拥有者必须覆盖全部路由回调。
    NetSenderServiceManaged* m_owner;
    // true为客户端输出入口，false为业务处理器或内部服务入口
    bool m_clients;
};

// 一个已登记来源的本地路由信息，由服务执行者独占
class NetSenderServiceManagedPeer
{
public:
    /** 初始化空来源、通道和角色，不登记路由
    */
    NetSenderServiceManagedPeer();
    // 已认证的客户端来源快照，不信任业务负载中的同名字段
    NetInterfaceOrigin m_origin;
    // 所属网关逻辑通道；多条来源可归属同一内部服务通道
    uint64_t m_channel;
    // 网关已验证的NI_LINK_CLIENT或NI_LINK_SERVICE角色
    uint32_t m_role;
};

// v3服务端组合：端点TLS、请求树、文件、内部依赖和推送复用现有公共算法。
// 所有入口由同一poll执行者调用；业务回调通过拥有队列交付，不能重入poll。
class NetSenderServiceManaged : private NetSenderServiceGatewayListener, private NetTransportListener
{
public:
    /** 创建停用的服务组合；所有入口由同一执行者串行调用
    @param [in] inbox 借用收件箱，须覆盖本对象生命周期
    */
    explicit NetSenderServiceManaged(NetSenderLocalInbox& inbox);

    /** 收敛网关和路由器，再释放内部依赖；禁止在自身回调中析构
    */
    ~NetSenderServiceManaged();

    /** 登记本服务并连接本机网关，不以PID代替认证
    @param [in] pid 本机网关PID
    @param [in] product 产品号，0为Account
    @param [in] proof 服务身份签名回调
    @param [in] context 身份回调上下文，借用至shutdown
    @param [in] listener 业务监听者，借用至关闭通知完成
    @param [in] limit 请求限额，1至128
    @param [in] weight 服务负载权重
    @param [in] channels 独立通道容量，0使用网关默认值
    @return NI_OK启动，其余为负数且不对外发布半登记的服务
    */
    int32_t connect(int32_t pid, uint32_t product, NetInterfaceLinkServiceProof proof, void* context,
        NetSenderServiceListener* listener, uint32_t limit, uint32_t weight, uint32_t channels = 0);

    /** 有界推进网关、内部依赖和三路队列；禁止跨线程或递归推进
    @return false表示未启动或服务已关闭，true仍需继续推进
    */
    bool poll();

    /** 把业务输出交给本实例路由器；外层公共send负责后续交付边界
    @param [in] frame 借用至返回，异步输出由队列复制
    @return NI_OK接纳、NI_LIMIT_EXCEEDED背压、NI_NOT_FOUND目标失效或负数错误
    */
    int32_t send(const NetInterfaceFrame& frame);

    /** 在接纳客户端前配置允许压缩的业务类型
    @param [in] types 借用类型数组，count为0时允许为空
    @param [in] count 数组项数
    @return NI_OK或配置错误，不强行改变已运行会话
    */
    int32_t configureCompression(const uint32_t* types, uint32_t count);

    /** 停止新工作并收敛拥有的通道；监听者和借用收件箱仍须有效
    */
    void shutdown();

    /** 读取当前可用服务代次，仅所属执行者调用
    @return 非零表示就绪；未就绪、停止或已关闭返回0
    */
    uint64_t endpoint() const;

private:
    friend class NetSenderServiceManagedSink;
    // 仅NetWorkTest核对真实有界队列的跨轮背压时序，不生成运行时测试接口。
    friend class NetSenderServiceQueueTest;

    /** 生成非零服务代次并通知业务监听者，重复就绪视为协议错误
    */
    virtual void onGatewayServiceReady();

    /** 接纳已认证客户端；Account只接受服务角色并在请求到达时绑定来源
    @param [in] channel 网关逻辑通道
    @param [in] role 已认证角色
    @param [in] origin 已认证来源快照
    */
    virtual void onGatewayClientReady(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin);

    /** 验证服务包装及来源后交给本地路由器
    @param [in] channel 已认证通道
    @param [in] role 已认证角色
    @param [in] frame 回调期间借用的完整明文帧
    @param [in] origin 直接连接的已认证来源
    @return false仅用于数据、确认或事件背压；true表示已处理或已拒绝
    */
    virtual bool onGatewayClientFrame(uint64_t channel, uint32_t role, const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 移除该通道的全部来源及关联请求，通知内部依赖来源终止
    @param [in] channel 已失效通道
    @param [in] reason 关闭原因
    */
    virtual void onGatewayClientClosed(uint64_t channel, int32_t reason);

    /** 清理全部队列和路由并唯一通知服务关闭
    @param [in] reason 网关原因；主动失败时优先保留本地原因
    */
    virtual void onGatewayServiceClosed(int32_t reason);

    /** 内部依赖TLS就绪后创建独立业务握手
    @param [in] peer 内部通道只接受编号1
    */
    virtual void onReady(uint64_t peer);

    /** 验证内部依赖握手、心跳及来源包装后交付路由
    @param [in] peer 内部通道编号
    @param [in] frame 回调期间借用
    @return false表示无副作用的有界背压，true已处理或已拒绝
    */
    virtual bool onFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 记录内部依赖关闭并安排安全释放和退避重连
    @param [in] peer 内部通道编号
    @param [in] reason 关闭原因
    @param [in] detail 下层详细错误，此层不另行解释
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);

    /** 在库拥有队列中接纳路由器输出，禁止同步业务回调
    @param [in] clients true为客户端输出，false为服务处理器或内部依赖
    @param [in] endpoint 本地路由端点
    @param [in] frame 借用帧，成功接纳时复制
    @param [in] origin 已认证来源
    @return NI_OK已入队、NI_LIMIT_EXCEEDED背压或负数错误
    */
    int32_t enqueue(bool clients, uint64_t endpoint, const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 按路由输出方向关闭一个端点，不影响无关客户端
    @param [in] clients true客户端，false业务处理器或内部依赖
    @param [in] endpoint 已登记端点
    */
    void closeEndpoint(bool clients, uint64_t endpoint);

    /** 复用匹配来源或事务式登记新路由端点
    @param [in] channel 网关通道
    @param [in] role 已验证角色
    @param [in] origin 已认证来源
    @param [out] endpoint 成功为本地路由号，失败为0
    @return NI_OK或来源冲突、容量、内存等负数错误
    */
    int32_t attach(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin, uint64_t& endpoint);

    /** 完成已认证来源的本地登记；分配失败回滚索引并关闭尚未公开的路由端点
    @param [in] channel 已验证的网关通道
    @param [in] role 已验证角色
    @param [in] origin 已验证来源
    @param [in] fields 业务握手协商的四个字段，借用至返回
    @param [out] endpoint 完整登记后才赋值，失败为0
    @return OK、NO_MEMORY或路由错误；失败端点由后续poll回收
    */
    int32_t addPeer(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin,
        const uint32_t fields[4], uint64_t& endpoint);

    /** 推进内部Account依赖，失败退避重连且不重放旧请求
    @param [in] now 单调毫秒，执行中刷新以排除回调耗时
    */
    void pumpInternal(uint64_t now);

    /** 查询同来源／同目标的保序键，不改变队列
    @param [in] frame 队列拥有的消息
    @param [in] kind 0业务回调，1客户端输出，2内部依赖
    @return 来源或目标通道；已失效目标返回0，后续按原规则丢弃
    */
    uint64_t queueTarget(const NetSenderServiceManagedFrame& frame, uint32_t kind) const;

    /** 每轮最多32次实际投递，跳过本轮已背压来源，保留其在后继消息之前
    @param [in] kind 回调／客户端／内部依赖
    @param [in] now 单调毫秒，实际投递前刷新
    */
    void pumpQueue(uint32_t kind, uint64_t now);

    /** 使内部端点立即失效，丢弃其队列并延迟释放运输对象
    @param [in] reason 该依赖关闭原因
    */
    void stopInternal(int32_t reason);

    /** 保存首个主动终止原因并停止网关服务
    @param [in] reason 正常停止为0，失败为负数
    */
    void fail(int32_t reason);

    /** 查询三路队列及借用项的共享剩余额度，不修改状态
    @param [in] count 待新增项数
    @param [in] bytes 待新增正文总字节数
    @return true表示两种额度都足够
    */
    bool room(uint32_t count, uint64_t bytes) const;

    /** 复制消息到一条队列并保存入队时间
    @param [in] lane 0业务回调、1客户端、2内部依赖
    @param [in] endpoint 本地路由端点
    @param [in] frame 借用消息头和正文
    @param [in] origin 已认证来源
    @return NI_OK已复制、NI_LIMIT_EXCEEDED未接纳或负数错误
    */
    int32_t push(uint32_t lane, uint64_t endpoint,
        const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 禁止复制通道和路由所有权
    @param [in] other 不可复制实例
    */
    NetSenderServiceManaged(const NetSenderServiceManaged& other);

    /** 禁止复制通道和路由所有权
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetSenderServiceManaged& operator=(const NetSenderServiceManaged& other);
    // 路由器到本服务处理器和内部依赖的借用接收入口
    NetSenderServiceManagedSink m_services;
    // 路由器到已登记客户端的借用接收入口
    NetSenderServiceManagedSink m_clients;
    // 拥有请求关联、来源隔离、取消及文件状态的路由器
    NetEndpointRouter m_router;
    // 拥有本机网关登记及客户端端点会话，借用外部收件箱
    NetSenderServiceGateway m_gateway;
    // 内部依赖通道的独占运输适配，须晚于m_internalChannel销毁
    std::unique_ptr<NetSenderServiceOutgoingTransport> m_internalTransport;
    // 到公共Account服务的独占端点通道，重连后不重放旧请求
    std::unique_ptr<NetGatewayClientChannel> m_internalChannel;
    // 内部依赖的应用握手状态，断开时与通道一同释放
    std::unique_ptr<NetInterfaceHandshake> m_internalHandshake;
    // 借用业务监听者，从connect接纳到服务关闭期间有效
    NetSenderServiceListener* m_listener;
    // 本地路由端点到网关通道、角色和已认证来源
    std::map<uint64_t, NetSenderServiceManagedPeer> m_peers;
    // 外部来源端点到本地路由端点的反向索引，与m_peers成对维护
    std::map<uint64_t, uint64_t> m_roots;
    // 三路消息及当前回调借用共享同一个有界拥有者
    NetSenderServiceQueue m_queue;
    // 本服务在路由器中的业务处理器编号，0表示未登记
    uint64_t m_handler;
    // 已就绪内部依赖在路由器中的端点，断开时先清零
    uint64_t m_internalEndpoint;
    // 本次服务就绪的非零随机代次；停止后对外不再可用
    uint64_t m_generation;
    // 当前同步路由调用暂借的上游父请求号，调用结束即清零
    uint64_t m_foreignParent;
    // 内部依赖下次允许重连的单调毫秒
    uint64_t m_retryAt;
    // 内部依赖最后收到有效帧的单调毫秒，45秒无输入则断开
    uint64_t m_internalReceive;
    // 内部依赖最近发送心跳或收到匹配响应的单调毫秒
    uint64_t m_pingAt;
    // 内部依赖心跳递增编号，溢出时停止该连接
    uint64_t m_ping;
    // 内部依赖重连退避毫秒，250起步，最大5000
    uint32_t m_retryDelay;
    // 本服务产品号，0为公共Account服务
    uint32_t m_product;
    // 单端点协商请求限额，connect接受1至128
    uint32_t m_limit;
    // 主动失败时保存的关闭原因，正常停止为0
    int32_t m_reason;
    // 已成功登记处理器并进入服务连接流程
    bool m_started;
    // 已拒绝新工作，等待唯一服务终态
    bool m_stopping;
    // 已完成服务关闭通知，后续poll不再工作
    bool m_closed;
    // 单执行者poll重入保护，不提供多线程锁
    bool m_polling;
    // 内部依赖已标记失效，等待下一次安全推进位置释放
    bool m_internalClosing;
};