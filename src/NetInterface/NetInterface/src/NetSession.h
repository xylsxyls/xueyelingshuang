#pragma once
#include "NetInterfaceTransport.h"
#include "NetHeartbeatConfig.h"
#include "NetInterfaceConnection.h"
#include "NetInterfaceHandshake.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"

class NetSessionState;

// 所有回调属于poll/关闭的执行者；输入只借用到返回，禁止等待或销毁/重入poll。
class NetInterfaceAPI NetSessionListener
{
public:
    /** 不拥有会话
    */
    virtual ~NetSessionListener();

    /** TLS和应用握手均已通过，可发起业务
    @param [in] generation 本对象连接代次，递增不复用
    @param [in] maxRequests 协商在途上限
    @param [in] maxChunk 协商块上限
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk) = 0;

    /** 已接纳请求的唯一终态，包括尚未send的预留请求
    @param [in] generation 发起时的连接代次
    @param [in] requestId 原64位请求号
    @param [in] product 请求产品
    @param [in] type 请求命令
    @param [in] status NetInterfaceRpcStatus；正常本地关闭为CANCELLED，远端失联为UNAVAILABLE，非SUCCESS数据为空
    @param [in] data 借用业务体
    @param [in] size 字节数
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type, uint32_t status,
                                 const uint8_t* data, uint32_t size) = 0;

    /** 数据块及服务推送交给已登记的上层处理，不解释业务权限
    @param [in] generation 当前代次
    @param [in] frame EVENT/DATA/DATA_ACK帧，调用时借用
    @return false无副作用背压；true接纳，上层须核对流拥有者和消息类型
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame) = 0;

    /** 所有请求先回报终态，再关闭本代次
    @param [in] generation 此次连接代次，握手失败也非零
    @param [in] reason NetInterface/TLS/libuv错误或0主动关闭
    @param [in] detail TLS诊断或0
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail) = 0;
};

// 无物理传输的客户端协议会话；桌面IPC及各端进程内门面共用请求/取消/终态状态机。
class NetInterfaceAPI NetSession : private NetTransportListener
{
public:
    /** 创建未连接对象
    */
    NetSession();

    /** 停止并释放；监听者必须仍然存活，不得在回调内销毁
    */
    ~NetSession();

    /** 首次连接前设置本会话独立的PING间隔、PONG和总无输入期限
    @param [in] config 三个单调毫秒期限，复制数值，失败保留原配置
    @return NI_OK、NI_INVALID_ARGUMENT或NI_INVALID_STATE
    */
    int32_t configureHeartbeat(const NetHeartbeatConfig& config);

    /** 设置每轮poll最多结算的到期请求数，只能在首次连接前调用
    @param [in] count 至少1，默认128
    @return NI_OK、NI_INVALID_ARGUMENT或NI_INVALID_STATE
    */
    int32_t configureExpiryBudget(uint32_t count);

    /** 首次连接前设置每次请求编码后允许保留的暂存容量
    @param [in] bytes 字节数，默认4096，0立即释放，-1保留增长后的容量
    @return NI_OK、NI_INVALID_ARGUMENT或NI_INVALID_STATE，失败保留原配置
    */
    int32_t configurePayloadRetention(int64_t bytes);

    /** 在所属执行者上查询RPC编码缓冲的实际保留容量
    @return 容量字节数，shutdown后为0
    */
    uint64_t payloadCapacity() const;

    /** 首次连接前配置协议请求记录池，未完成应答不占普通DATA记录
    @param [in] config 基础池初值、上限和增长策略；仅静止生命周期owner调用
    @return NI_OK或状态/参数/资源错误
    */
    int32_t configureRequestPool(const LockFreeHashMapPoolConfig& config);

    /** 显式绑定借用通道；null只解除绑定，不创建物理传输
    @param [in] channel 借用至shutdown/更换，必须实现相同TLS及回调契约；不可在回调/活跃连接中更换
    @return OK或INVALID_STATE；不拥有、不销毁通道
    */
    int32_t bindChannel(NetClientChannel* channel);

    /** 异步发起新连接；通道接纳后分配新代次，同步拒绝时可沿用绑定重试
    @param [in] address 数值IPv4/IPv6
    @param [in] port 1至65535
    @param [in] tls 客户端配置，根/域名及强随机回调必需，调用时复制
    @param [in] handshake 客户端能力和持久device；nonce由本方法生成，传入值忽略
    @param [in] listener 借用至本代次关闭，不可空
    @return OK启动，其余负数未启动；未绑定通道NOT_SUPPORTED；通道抛出分配异常为 NI_NO_MEMORY，其他异常为 NI_INVALID_STATE。READY仅由回调报告
    */
    int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls, const NetInterfaceHandshakeOptions& handshake,
                    NetSessionListener* listener);

    /** 验证直连参数和 HELLO 后，将可选池配置交给已绑定的通道，再启动连接
    @param [in] address 由已绑定的通道解释的非空地址
    @param [in] port 1至65535
    @param [in] tls 客户端 TLS 配置，随机回调及上下文在连接期间有效
    @param [in] handshake 客户端能力和持久设备标识，nonce 在本次调用生成
    @param [in] listener 借用至本代关闭，不可空
    @param [in] pools 独立的连接池参数，借用至返回，为空时保留通道自身配置
    @return NI_OK 已接纳。配置失败保留原代次并返回通道错误。配置回调抛出分配异常返回 NI_NO_MEMORY，其他异常返回 NI_INVALID_STATE
    */
    int32_t connectWithPools(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls,
        const NetInterfaceHandshakeOptions& handshake, NetSessionListener* listener, const NetConnectionPoolConfig* pools);

    /** 非阻塞驱动；运输耗尽且未报告关闭时用本地原因或 NI_TRUNCATED 结算。推进抛异常时停止通道并结算在途请求。单执行者调用，不重入
    @return 仍有正在连接/关闭的资源
    */
    bool poll();

    /** 预留请求，先取得编号再用业务SDK构造载荷，不在业务维护计数
    @param [in] product 非零产品号
    @param [in] type 命令号
    @param [in] timeoutMs 总期限1至60000毫秒，从本次预留开始且不因重试延长
    @param [out] requestId 成功非零，否则0；容量拒绝可能消耗编号
    @return OK预留，其余负数拒绝且无终态回调
    */
    int32_t beginRequest(uint32_t product, uint32_t type, uint32_t timeoutMs, uint64_t& requestId);

    /** 接收统一提交层的原号与原始期限，仅协议执行者调用
    @param [in] requestId 本来源严格递增的非零原号
    @param [in] product 非零产品
    @param [in] type 业务命令
    @param [in] startedMs 原始单调开始时间，不能晚于当前时间
    @param [in] timeoutMs 总期限1至60000，排队不延长
    @return OK、TIMEOUT或错误；不再生成第二个原号
    */
    int32_t beginRequestAt(uint64_t requestId, uint32_t product, uint32_t type, uint64_t startedMs, uint32_t timeoutMs);

    /** 发送预留请求，body由下层复制
    @param [in] requestId 当前未发送请求号
    @param [in] body 业务二进制，非空size时不可空
    @param [in] size 不超过1048560
    @return OK已接纳且只能发送一次；LIMIT_EXCEEDED未接纳可重试，其他错误须cancel
    */
    int32_t sendRequest(uint64_t requestId, const uint8_t* body, uint32_t size);

    /** 取消当前请求并报告唯一CANCELLED终态；已发出的取消控制帧失败则关闭代次
    @param [in] requestId 已预留的号
    @return OK取消，NOT_FOUND已结束
    */
    int32_t cancel(uint64_t requestId);

    /** 发送上层已授权的数据帧，不接受RPC或握手帧
    @param [in] frame DATA/DATA_ACK或FILE_DATA_TYPE的无体CANCEL，产品/流号非零，需协商数据块能力
    @return OK接纳或负数；背压不重写或丢弃输入
    */
    int32_t sendData(const NetInterfaceFrame& frame);

    /** 请求关闭，回调中可调用。首次请求后停止握手和业务交付，重复请求沿用首次原因
    运输 close 抛异常时，下一次 poll 或 shutdown 在运输回调返回后停止通道并结算已接纳的请求
    @param [in] reason 默认0；运输未报告终态时由 poll 或 shutdown 用此原因结算
    */
    void close(int32_t reason = 0);

    /** 非回调调用者立即收敛并解除通道绑定；回调中仅请求关闭，执行者随后完成收敛
    */
    void shutdown();

    /** 当前是否允许新业务
    @return 握手通过且未开始关闭
    */
    bool ready() const;

    /** 最近获得通道接纳的连接代次
    @return 首次接纳前为0，每次接纳后递增；同步错误保留原代次
    */
    uint64_t generation() const;

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    int32_t outputState() const;

private:
    /** 停止本次尝试的通道一次；允许其同步报告关闭，隔离宿主异常和生命周期重入
    */
    void stopChannel();

    /** 隔离就绪回调并还原嵌套回调标志，异常只关闭本代
    @param [in] requests 实际协商请求上限
    @param [in] chunk 实际协商块长
    */
    void notifyReady(uint32_t requests, uint32_t chunk);

    /** 隔离单个请求终态，异常不得吃掉其他已接纳请求的终态
    @param [in] request 已从等待表取出的请求
    @param [in] status 本次业务终态
    @param [in] bytes 借用正文，size非零时有效
    @param [in] size 正文字节数
    */
    void notifyResult(const NetInterfaceRequest& request, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 隔离借用帧回调；异常关闭本代并消费出错帧，不能重复交付
    @param [in] frame 仅回调期间有效的完整帧
    @return 正常回调的接纳结果；异常为true并关闭本代
    */
    bool notifyFrame(const NetInterfaceFrame& frame);

    /** 隔离唯一关闭通知，连接已失效，回调异常也必须恢复可回收状态
    @param [in] reason 原关闭结果
    @param [in] detail 原运输诊断
    */
    void notifyClosed(int32_t reason, int32_t detail);

    /** 隔离适配器发送异常；send不得同步回调或保留借用帧指针
    @param [in] frame 完整借用帧
    @return 下层接纳结果，宿主异常为INVALID_STATE，不自动重放
    */
    int32_t sendFrame(const NetInterfaceFrame& frame);

    /** 共用唯一请求预留及回滚实现
    @param [in] original 0使用原自动编号，否则采用提交层原号
    @param [in] product 产品
    @param [in] type 命令
    @param [in] startedMs 原始单调起点
    @param [in] timeoutMs 原期限
    @param [out] requestId 成功非零，失败0
    @return OK或负数
    */
    int32_t reserveRequest(uint64_t original, uint32_t product, uint32_t type, uint64_t startedMs, uint32_t timeoutMs, uint64_t& requestId);

    /** TLS就绪后生成同源应用握手
    @param [in] peer 真实传输连接号
    */
    virtual void onReady(uint64_t peer);

    /** 校验控制/RPC及在途对应关系
    @param [in] peer 真实连接
    @param [in] frame 借用完整帧
    @return 接纳或无副作用背压
    */
    virtual bool onFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 结算全部请求和唯一连接终态
    @param [in] peer 关闭连接
    @param [in] reason 错误
    @param [in] detail TLS诊断
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);

    /** 发送待握手并只在下层接纳后消费
    @param [in] nowMs 单调毫秒
    */
    void progressHandshake(uint64_t nowMs);

    /** 发送请求对应的取消；调用者先交付本请求终态，再按错误关闭代次
    @param [in] request 原请求元数据
    @return OK或负数，不抛出适配器异常
    */
    int32_t sendCancel(const NetInterfaceRequest& request);

    /** 禁止复制独占会话
    @param [in] other 原对象
    */
    NetSession(const NetSession& other);

    /** 禁止赋值
    @param [in] other 原对象
    @return 不可调用
    */
    NetSession& operator=(const NetSession& other);

private:
    // 单一请求拥有表与协议状态，不暴露跨模块STL布局。
    NetSessionState* m_state;
public:
    /** 启动前配置无秘密公开RPC类型，握手不支持时保持原帧
    @param [in] types count非零时不可空，各类型非零；只借用至返回
    @param [in] count 非负项数，0清空
    @return OK或参数/状态/内存错误，失败保留旧配置
    */
    int32_t configureCompression(const uint32_t* types, uint32_t count);
};