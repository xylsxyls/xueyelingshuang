#pragma once
#include "NetInterface.h"
#include <vector>
#include <memory>

// 只注入已验证管理事件，检验业务通道状态/背压/异常；不替代真实TLS集成测试
class NetGatewayChannelTestTransport : public NetGatewayTransport
{
public:
    /** 清零控制项，不创建网络
    */
    NetGatewayChannelTestTransport();

    /** 保存借用监听者，按测试配置接纳或拒绝
    @param [in] address 未使用
    @param [in] port 未使用
    @param [in] options 未使用
    @param [in] listener 保存至停止
    @param [out] peer 通常7，畸形模式为0
    @return 注入的连接状态
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
        NetLinkTransportListener* listener, uint64_t& peer);

    /** 交付一次节点ready或受控异常
    @return 是否仍借用监听者
    */
    virtual bool poll();

    /** 只保存有界完整控制包，模拟背压
    @param [in] peer 本次管理代次
    @param [in] frame 借用至返回
    @return OK或LIMIT_EXCEEDED
    */
    virtual int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 只计数密文字节
    @param [in] peer 当前代次
    @param [in] record 借用至返回
    @return OK
    */
    virtual int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 先解除全部借用，再按配置抛异常或交付旧关闭事件
    */
    virtual void shutdown();
public:
    // 借用的被测监听者
    NetLinkTransportListener* m_listener;
    // 连接结果
    int32_t m_connectResult;
    // 模拟非法零代次
    bool m_zeroPeer;
    // OPEN暂不接纳
    bool m_busy;
    // 推进异常一次
    bool m_pollThrow;
    // 停止异常一次
    bool m_stopThrow;
    // 停止时交付旧事件
    bool m_closeDuringStop;
    // 是否已经回报管理就绪
    bool m_announced;
    // 停止调用数
    uint32_t m_stops;
    // 控制包调用数
    uint32_t m_controls;
    // 实际接纳密文字节数
    uint32_t m_bytes;
    // 最近完整控制包长度
    uint32_t m_controlSize;
    // 固定测试观察缓冲，避免测试本身逐次分配
    uint8_t m_control[256];
};

// 观察唯一终态和回调重入，不创建任何网络
class NetGatewayChannelTestListener : public NetTransportListener
{
public:
    /** 清零观察值
    */
    NetGatewayChannelTestListener();

    /** 记录就绪
    @param [in] peer 业务代次
    */
    virtual void onReady(uint64_t peer);

    /** 记录业务帧
    @param [in] peer 业务代次
    @param [in] frame 借用帧
    @return true
    */
    virtual bool onFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 记录首因，按配置重入停止和抛出观察者异常
    @param [in] peer 业务代次
    @param [in] reason 关闭原因
    @param [in] detail 未使用
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);
public:
    // 借用被测通道，关闭回调可以请求重复停止
    NetGatewayClientChannel* m_channel;
    // 真实关闭计数
    uint32_t m_closed;
    // 本负向测试不应发生的就绪或数据
    uint32_t m_other;
    // 最近关闭原因
    int32_t m_reason;
    // 关闭观察者抛一次异常
    bool m_throw;
};

// 独立恢复消费者的注入计数；不实现协议或创建网络
struct NetClientSessionTestCounters
{
    // 工厂创建次数
    uint32_t m_created;
    // 运输析构次数
    uint32_t m_destroyed;
    // 运输连接次数
    uint32_t m_connected;
    // 运输停止次数
    uint32_t m_stopped;
    // 0返回运输，1返回空，2抛异常
    uint32_t m_factoryMode;
    // 注入的连接返回值，不交付伪造的成功事件
    int32_t m_connectResult;
};

// 每个测试尝试独占一份运输，不实现管理握手
class NetClientSessionTestTransport : public NetGatewayTransport
{
public:
    /** 借用覆盖本运输生命期的计数
    @param [in,out] counters 测试状态
    */
    explicit NetClientSessionTestTransport(NetClientSessionTestCounters& counters);

    /** 记录实际资源回收
    */
    virtual ~NetClientSessionTestTransport();

    /** 同步拒绝连接，验证已接纳逻辑尝试仍有终态
    @param [in] address 未使用
    @param [in] port 未使用
    @param [in] options 未使用
    @param [in] listener 不保存
    @param [out] peer 清零
    @return 注入的NI错误
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
        NetLinkTransportListener* listener, uint64_t& peer);

    /** 没有物理资源
    @return false
    */
    virtual bool poll();

    /** 不接纳输出
    @param [in] peer 未使用
    @param [in] frame 未使用
    @return INVALID_STATE
    */
    virtual int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 不接纳输出
    @param [in] peer 未使用
    @param [in] record 未使用
    @return INVALID_STATE
    */
    virtual int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 记录停止，没有迟到回调
    */
    virtual void shutdown();

private:
    /** 禁止复制借用计数
    @param [in] other 原运输
    */
    NetClientSessionTestTransport(const NetClientSessionTestTransport& other);

    /** 禁止复制借用计数
    @param [in] other 原运输
    @return 不可调用
    */
    NetClientSessionTestTransport& operator=(const NetClientSessionTestTransport& other);

private:
    // 借用的本次测试计数
    NetClientSessionTestCounters* m_counters;
};

// 注入空工厂/异常/确定性拒绝，所有返回运输由被测会话拥有
class NetClientSessionTestFactory : public NetGatewayTransportFactory
{
public:
    /** 借用计数
    @param [in,out] counters 测试状态
    */
    explicit NetClientSessionTestFactory(NetClientSessionTestCounters& counters);

    /** 按模式创建运输或拒绝
    @return 新运输或空，模式2抛异常
    */
    virtual NetGatewayTransport* create();

private:
    /** 禁止复制借用计数
    @param [in] other 原工厂
    */
    NetClientSessionTestFactory(const NetClientSessionTestFactory& other);

    /** 禁止复制借用计数
    @param [in] other 原工厂
    @return 不可调用
    */
    NetClientSessionTestFactory& operator=(const NetClientSessionTestFactory& other);

private:
    // 借用的测试计数
    NetClientSessionTestCounters* m_counters;
};

// 观察逻辑代次及唯一终态，不参与会话算法
class NetClientSessionTestListener : public NetSessionListener
{
public:
    /** 清零非终态事件
    */
    NetClientSessionTestListener();

    /** 本组负向连接不应就绪
    @param [in] generation 未使用
    @param [in] maxRequests 未使用
    @param [in] maxChunk 未使用
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 未接纳业务不应出现结果
    @param [in] generation 未使用
    @param [in] requestId 未使用
    @param [in] product 未使用
    @param [in] type 未使用
    @param [in] status 未使用
    @param [in] bytes 未使用
    @param [in] size 未使用
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type,
        uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 不应出现数据帧
    @param [in] generation 未使用
    @param [in] frame 未使用
    @return false
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 按次记录逻辑代次/原因
    @param [in] generation 逻辑尝试号
    @param [in] reason NI结果
    @param [in] detail 未使用
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 确定性测试输入，禁止在真实应用作为强随机提供者
    @param [in] context 未使用
    @param [out] bytes 有效输出
    @param [in] size 长度
    @return 0
    */
    static int32_t entropy(void* context, uint8_t* bytes, uint32_t size);

public:
    // 按到达顺序的关闭代次
    std::vector<uint64_t> m_generations;
    // 与代次对应的NI原因
    std::vector<int32_t> m_reasons;
    // 禁止发生的其他事件
    uint32_t m_other;
    // 嵌套取消用例借用的已连接会话，其他用例为空
    NetClientSession* m_nestedSession;
    // 第一个结果回调中要取消的另一请求，0表示没有
    uint64_t m_nestedRequest;
    // 内层回调返回后外层执行保护是否仍有效
    bool m_nestedExecuting;
    // 嵌套取消是否真实接纳
    bool m_nestedCancelOk;
    // 关闭通知中重复停止/推进/绑定，验证外层回调保护
    bool m_reenterClosed;
    // 关闭观察者注入异常
    bool m_throwClosed;
    // 结果回调注入异常，不能影响其它请求的唯一终态
    bool m_throwResult;
    // 独立记录结果原号，不用被测请求表作为预期
    std::vector<uint64_t> m_resultIds;
    // 与原号对应的业务状态
    std::vector<uint32_t> m_resultStatus;
    // 结果和关闭的事件顺序，UINT64_MAX为关闭
    std::vector<uint64_t> m_events;
};

// 提供已验证完整帧的通道Mock，复用共享握手，仅隔离验证回调重入，不冒充TLS测试
class NetClientSessionTestChannel : public NetClientChannel
{
public:
    /** 创建独占服务端握手，不创建网络
    @param [in] options 合法服务端能力、nonce及期限
    */
    explicit NetClientSessionTestChannel(const NetInterfaceHandshakeOptions& options);

    /** 先收敛借用监听者
    */
    virtual ~NetClientSessionTestChannel();

    /** 接纳单次Mock连接，事件仅在poll/shutdown交付
    @param [in] address 未使用
    @param [in] port 未使用
    @param [in] options 已由共用会话验证的配置
    @param [in] listener 借用至shutdown
    @param [out] peer 成功为1，失败0
    @return OK或INVALID_STATE
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint64_t& peer);

    /** 交付原始ready和共享握手输出
    @return 是否仍有借用连接
    */
    virtual bool poll();

    /** 将握手帧交给已有服务端握手，不实现另一套协议
    @param [in] peer 当前1
    @param [in] frame 借用完整帧
    @return 共用握手接纳结果
    */
    virtual int32_t send(uint64_t peer, const NetInterfaceFrame& frame);

    /** 标记关闭，下一poll交付
    @param [in] peer 当前1
    @param [in] reason 首个NI原因
    */
    virtual void close(uint64_t peer, int32_t reason);

    /** 收敛一次终态并解除借用
    */
    virtual void shutdown();

    /** 注入压缩配置异常，否则接受已验证协商
    @param [in] peer 未使用
    @param [in] types 未使用
    @param [in] count 未使用
    @param [in] negotiated 未使用
    @return OK或抛受控异常
    */
    virtual int32_t configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated);

    /** 注入查询异常，否则没有待发输出
    @param [in] peer 未使用
    @return OK或抛受控异常
    */
    virtual int32_t outputState(uint64_t peer) const;

    /** 保存下一轮poll交付的完整帧，测试输入必须有效
    @param [in] frame 有界借用帧，复制正文
    */
    void queueFrame(const NetInterfaceFrame& frame);

public:
    // 请求/取消/数据/配置/查询/关闭/推进/零peer/连接异常/拒绝/背压注入位
    uint32_t m_fault;
    // send调用总数，包含真实应用握手帧
    uint32_t m_sendCount;
    // 仅供Debug分配器释放钩子观察的借用指针，业务代码不可保留此类指针
    const uint8_t* m_observed;
    // 上述暂存正文长度
    uint32_t m_observedSize;

private:
    /** 禁止复制独占握手
    @param [in] other 原通道
    */
    NetClientSessionTestChannel(const NetClientSessionTestChannel& other);

    /** 禁止复制借用
    @param [in] other 原通道
    @return 不可调用
    */
    NetClientSessionTestChannel& operator=(const NetClientSessionTestChannel& other);

private:
    // 既有服务端握手算法
    std::unique_ptr<NetInterfaceHandshake> m_handshake;
    // 借用的客户端帧观察者
    NetTransportListener* m_listener;
    // 原始就绪是否已交付
    bool m_announced;
    // 是否请求关闭
    bool m_closing;
    // 首个关闭原因
    int32_t m_reason;
    // 下一poll待交付测试帧
    bool m_queued;
    // 固定元数据，正文指向本测试对象的拥有缓冲
    NetInterfaceFrame m_queuedFrame;
    // 仅测试生产事件，不实现第二套RPC协议
    std::vector<uint8_t> m_queuedBody;
};

// 用于发现无候选及失败记录异常边界
class NetClientSessionTestDiscovery : public NetClientDiscovery
{
public:
    /** 默认允许选择，失败记录不抛异常
    */
    NetClientSessionTestDiscovery();

    /** 接纳发现，不启动IO
    @param [in] seeds 未使用
    @param [in] cachePath 未使用
    @param [in] tls 未使用
    @return OK
    */
    virtual int32_t start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath,
        const NetInterfaceTlsOptions& tls);

    /** 没有IO
    @return false
    */
    virtual bool poll();

    /** 没有资源
    */
    virtual void shutdown();

    /** 默认无资源；收敛模式要求停止后的两次poll
    @return 是否没有待收敛资源
    */
    virtual bool stopped() const;

    /** 按模式决定是否有候选
    @param [out] point 不改动已有合法地址
    @return 是否可用
    */
    virtual bool choose(NetClientDiscoveryPoint& point);

    /** 按模式抛异常以检验关闭通知不会丢失
    @param [in] point 未使用
    */
    virtual void failed(const NetClientDiscoveryPoint& point);

public:
    // 是否允许连接候选
    bool m_available;
    // 受控畸形候选：0正常，1尺寸，2版本，3无NUL，4非法端口。
    uint32_t m_invalidPoint;
    // 是否在记录失败时抛异常
    bool m_throw;
    // 是否注入停止后两次poll才能完成的资源
    bool m_drain;
    // 是否已收到本轮停止请求，重复停止不重置倒计数
    bool m_shutdownStarted;
    // 剩余收敛次数
    uint32_t m_remaining;
    // 1启动异常、2停止异常、4推进异常、8选择异常、16持续查询异常、32启动拒绝
    uint32_t m_fault;
};