#pragma once
#include "NetSession.h"
#include "NetGatewayClientChannel.h"
#include "NetClientDiscovery.h"

class NetClientSessionState;

// 共用恢复协调及业务会话；不拥有网络/IPC/线程/文件，只借用显式工厂与发现器
class NetInterfaceAPI NetClientSession : private NetSessionListener
{
public:
    /** 创建未连接协议状态，分配失败抛异常；不创建物理传输
    */
    NetClientSession();

    /** 同一执行者收敛；禁止在本对象回调中销毁
    */
    virtual ~NetClientSession();

    /** 绑定默认服务运输与可选发现，只允许会话停止后调用
    @param [in] factory 借用至解除绑定；空仅允许解除，不提供隐式原生运输
    @param [in] discovery 可空；非空借用至解除绑定，必须同时有factory
    @return OK或INVALID_ARGUMENT/INVALID_STATE
    */
    int32_t bindGateway(NetGatewayTransportFactory* factory, NetClientDiscovery* discovery = nullptr);

    /** 停止时配置候选及缓存提示，算法不访问路径
    @param [in] seeds 可空的数值入口列表，空采用connectService地址
    @param [in] cachePath 可空UTF8路径，最多4096字节；实际支持由发现器决定
    @return OK或负数；失败不替换既有配置
    */
    int32_t configureGateways(const std::string& seeds, const std::string& cachePath);

    /** 停止时设置公开无秘密RPC类型的压缩候选
    @param [in] types count非零时不可空
    @param [in] count 0至64，0清空
    @return OK或负数，实际使用仍需对端协商
    */
    int32_t configureCompression(const uint32_t* types, uint32_t count);

    /** 接纳逻辑服务恢复，已接纳业务只结算一次且绝不自动重放
    @param [in] product 固定逻辑目标，0的权限仍由服务角色及路由决定
    @param [in] address 最多39字节及NUL的默认数值入口，或由显式IPC运输解释的本地参数
    @param [in] port 1至65535，IPC也必须提供有效占位端口
    @param [in] tls 只采用强随机及100至60000毫秒期限，根/目标身份内置
    @param [in] handshake 客户端平台/能力/设备声明，不等于账号授权
    @param [in] listener 借用至shutdown，包含后续恢复代次
    @param [in] factory 可空用已绑定工厂；非空只借用本工厂并跳过默认发现
    @return OK接纳，结果由poll交付；无工厂NOT_SUPPORTED，无强随机明确失败
    */
    int32_t connectService(uint32_t product, const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
        const NetInterfaceHandshakeOptions& handshake, NetSessionListener* listener, NetGatewayTransportFactory* factory = nullptr);

    /** 绑定旧直连/IPC协议通道，只借用，不开启服务发现和恢复
    @param [in] channel 可空解除；非空借用至shutdown/更换
    @return OK或INVALID_STATE；活跃/回调期间拒绝
    */
    int32_t bindChannel(NetClientChannel* channel);

    /** 在显式通道上接纳旧协议连接，不分配默认网络
    @param [in] address 由通道解释的非空地址
    @param [in] port 1至65535
    @param [in] tls 客户端TLS与强随机
    @param [in] handshake 客户端能力和设备
    @param [in] listener 借用至shutdown
    @return OK接纳或负数；未绑定通道NOT_SUPPORTED
    */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
        const NetInterfaceHandshakeOptions& handshake, NetSessionListener* listener);

    /** 同一执行者推进发现、恢复和会话；禁止重入
    @return 是否仍有资源/自动恢复
    */
    bool poll();

    /** 停止恢复并请求关闭；回调中可调用，不制造重复终态
    @param [in] reason 关闭原因，默认0
    */
    void close(int32_t reason = 0);

    /** 非回调执行者请求停止；回调内只发close，外层再完成shutdown
    发现器异步回收未完成时quiescent仍为false，必须保持借用并继续poll；不得提前销毁发现器。
    */
    void shutdown();

    /** 业务TLS及能力握手是否完成
    @return 是否允许新请求
    */
    bool ready() const;

    /** 是否正在本对象poll或上层回调，供拥有方拒绝释放
    @return true时只能提交允许的请求/关闭，不能销毁或重入
    */
    bool executing() const;

    /** 是否停止全部业务、恢复及发现资源，可以由拥有方释放
    @return false时继续poll收敛；回调/poll期间始终false
    */
    bool quiescent() const;

    /** 当前/最近已接纳的逻辑尝试代次，包含发现和恢复失败
    @return 未接纳过连接为0，之后严格递增；不是底层TCP连接号
    */
    uint64_t generation() const;

    /** 是否采用逻辑服务语义
    @return true为服务模式，主动关闭不切换成旧直连
    */
    bool serviceMode() const;

    /** 预留请求，服务模式禁止跨目标产品
    @param [in] product 非零请求产品，必须匹配固定普通服务目标
    @param [in] type RPC命令
    @param [in] timeoutMs 1至60000毫秒，从本次预留开始计时
    @param [out] requestId 成功非零，失败0
    @return OK接纳或负数，未接纳没有请求回调
    */
    int32_t beginRequest(uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId);

    /** 由唯一执行者接收提交层原号，保留原始总期限
    @param [in] requestId 本地已接纳原号
    @param [in] product 固定服务产品或旧通用连接产品
    @param [in] type 业务命令
    @param [in] startedMs 原始单调开始
    @param [in] timeoutMs 原总期限
    @return OK或负数；只在现有NI请求表预留一次
    */
    int32_t beginRequestAt(uint64_t requestId, uint32_t product, uint32_t type, uint64_t startedMs, uint32_t timeoutMs);

    /** 发送已预留请求，仅下层接纳后标记已发送
    @param [in] requestId 当前代次的预留编号
    @param [in] bytes size非零时不可空，借用至返回
    @param [in] size 最多1048560字节
    @return OK接纳；LIMIT_EXCEEDED完全未接纳可重试；其他失败由调用者取消
    */
    int32_t sendRequest(uint64_t requestId, const uint8_t* bytes, uint32_t size);

    /** 取消已接纳请求，不能解释为服务端数据库回滚
    @param [in] requestId 当前请求
    @return OK或NOT_FOUND等错误，唯一终态由共用会话交付
    */
    int32_t cancel(uint64_t requestId);

    /** 提交已授权数据/确认/文件取消帧，不解释业务权限
    @param [in] frame 借用至返回，服务模式必须匹配固定目标
    @return OK、背压或错误
    */
    int32_t sendData(const NetInterfaceFrame& frame);

protected:
    /** 供进程内组合判断是否已经由调用者提供旧协议通道
    @return 是否存在借用通道，不包含内部服务TLS通道
    */
    bool hasBoundChannel() const;

    /** 供组合层识别默认工厂，避免将其宿主限制施加到显式工厂
    @return 当前借用工厂，未绑定为空；不得在返回后改变其所有权
    */
    NetGatewayTransportFactory* boundGatewayFactory() const;

    /** 派生组合析构时先停会话并解除全部借用，防止基类析构再访问已销毁成员
    仅非poll/回调的同一执行者、派生析构中调用；发现器后续收敛由其拥有方兜底。
    常规关闭/释放仍使用shutdown及quiescent，不能用本方法绕过异步资源收敛。
    */
    void detachBindingsForDestruction();

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    int32_t outputState() const;

private:
    /** 检查发现器的真实收敛状态，宿主异常视为尚未停止
    @return 没有发现器或其确认已经停止
    */
    bool discoveryStopped() const;

    /** 请求停止发现器；只在它确认收敛后解除借用，异常不向外传播
    */
    void stopDiscovery();

    /** 已解绑NetSession后，先释放通道再释放运输，不跨回调销毁
    */
    void releaseAttempt();

    /** 超过原始总期限时按唯一关闭路径结算，不能接受迟到就绪
    @param [in] nowMs 当前单调毫秒
    */
    void expireRecovery(uint64_t nowMs);

    /** 为接纳/恢复预留新代次，耗尽时拒绝且不复用
    @return 是否成功
    */
    bool reserveGeneration();

    /** 终止一次无法进入物理连接的已接纳尝试，确保也交付一次终态
    @param [in] reason 首次失败原因
    */
    void failAttempt(int32_t reason);

    /** 创建新的独占运输和TLS代次，不复用旧代资源
    */
    void attempt();

    /** 仅在原始恢复期限内转交一次业务就绪，然后结束期限
    @param [in] generation 代次
    @param [in] maxRequests 协商额度
    @param [in] maxChunk 协商块大小
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 转交共用会话的唯一请求终态
    @param [in] generation 原请求代次
    @param [in] requestId 原编号
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status RPC结果
    @param [in] bytes 借用结果
    @param [in] size 长度
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type,
        uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 转交数据/推送，拒绝必须没有副作用
    @param [in] generation 当前代次
    @param [in] frame 借用帧
    @return 是否接纳
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 保留首次终态，必要时退避并安排新代，绝不重放原请求
    @param [in] generation 关闭代次
    @param [in] reason NI结果
    @param [in] detail 宿主/TLS诊断
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 禁止复制独占会话
    @param [in] other 原对象
    */
    NetClientSession(const NetClientSession& other);

    /** 禁止复制独占会话
    @param [in] other 原对象
    @return 不可调用
    */
    NetClientSession& operator=(const NetClientSession& other);

private:
    // 协议状态由本库独占，不跨DLL暴露STL布局
    NetClientSessionState* m_state;
};