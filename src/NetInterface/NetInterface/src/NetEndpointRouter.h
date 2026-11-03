#pragma once
#include "NetInterfaceTransport.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceService.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceEvent.h"
#include <memory>
#include <vector>

class NetEndpointRouterState;

// 由网关从真实根连接附加；业务进程不能通过内部B请求替换它。
typedef NetInterfaceOrigin NetEndpointOrigin;

// 本地认证服务的投递适配；仅网关执行者调用，不可同步重入收帧或销毁网关；可请求关闭。
class NetInterfaceAPI NetEndpointServiceSink
{
public:
    /** 析构接口，不拥有网关
    */
    virtual ~NetEndpointServiceSink();

    /** 拷贝请求/回复/取消到本地有界发送队列
    @param [in] endpoint 已认证本地服务代次，不是远端自报PID
    @param [in] frame 调用时借用，退出后不得保存指针
    @param [in] origin 原始客户端上下文，控制消息可为零
    @return OK已接纳，LIMIT_EXCEEDED未接纳背压，其他负数连接错误
    */
    virtual int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin) = 0;

    /** 要求丢弃一代本地连接，旧回复不可重新登记为新端点
    @param [in] endpoint 失效代次
    */
    virtual void close(uint64_t endpoint) = 0;
};

// 真实网关的单执行者核心；线程由外层CTask提供，通用内部服务查询也是异步消息。
class NetInterfaceAPI NetEndpointRouter : private NetTransportListener
{
public:
    /** 创建本轮路由表
    @param [in] services 借用本地认证处理适配，可空表示尚无处理器
    @param [in] clients 可空，非空使用外部已认证端点运输；借用至shutdown
    */
    explicit NetEndpointRouter(NetEndpointServiceSink* services, NetEndpointServiceSink* clients = nullptr);

    /** 关闭TCP并清理本轮元数据；services须仍存活
    */
    virtual ~NetEndpointRouter();

    /** 绑定外部物理传输，附加端点模式不接受非空传输
    @param [in] transport 借用至shutdown；只允许无端点/未监听时绑定，或停止后解除绑定
    @return OK或INVALID_STATE；不分配传输或建立网络
    */
    int32_t bindTransport(NetServerChannel* transport);

    /** 启动TLS监听，不接受明文降级
    @param [in] address 显式数值地址
    @param [in] port 0测试随机端口，或1至65535
    @param [in] tls 服务器证书/私钥及宿主强随机回调
    @param [in] peers 1至256
    @return OK或负数错误
    */
    int32_t listen(const char* address, uint32_t port, const NetInterfaceTlsOptions& tls, uint32_t peers = 64);

    /** 有界驱动网络/握手/超时/关闭回收，不创建线程
    @return 仍运行为true
    */
    bool poll();

    /** 仅允许宿主在本地凭证验证成功后登记；不向公网开放此入口
    @param [in] product 普通产品非零；通用内部服务必须为0
    @param [in] internal 是否仅接内部父调用的账号服务
    @param [in] limit 每代入/出请求限额1至128
    @param [out] endpoint 新代次，失败为0
    @return OK或负数错误；重复产品拒绝，不隐式覆盖在线服务
    */
    int32_t addService(uint32_t product, bool internal, uint32_t limit, uint64_t& endpoint);

    /** 服务进程登记自身业务处理器，通用处理器0只供已经由宿主验证的内部来源
    @param [in] product 逻辑产品号，0可处理多个产品的内部请求
    @param [in] limit 入/出在途上限1至128
    @param [out] endpoint 成功为新代次，失败0
    @return NI_OK或错误；不构成网络服务注册授权
    */
    int32_t addHandler(uint32_t product, uint32_t limit, uint64_t& endpoint);

    /** 挂接已经由宿主完成业务TLS及能力握手的来源，不再次终止TLS
    @param [in] context 宿主验证后的来源号/平台/设备；来源号必须非零且当前不重复
    @param [in] negotiated 平台、能力、在途、块大小四项
    @param [out] endpoint 本路由器严格递增的内部来源号，与context来源号分开
    @return NI_OK或错误；宿主必须通过可信角色验证后才能采用内部转发上下文
    */
    int32_t addClient(const NetEndpointOrigin& context, const uint32_t negotiated[4], uint64_t& endpoint);

    /** 由挂接端点交付完整业务帧，不能自报PID/来源/目标
    @param [in] endpoint addClient返回的当前代次
    @param [in] frame 已经由端点TLS解密并完整解析的帧
    @return NI_OK接纳，文件LIMIT_EXCEEDED无副作用背压，其他错误关闭此来源
    */
    int32_t receiveClient(uint64_t endpoint, const NetInterfaceFrame& frame);

    /** 已认证本地适配交付一帧，来源身份不可从消息体采用
    @param [in] endpoint 先前登记的服务代次
    @param [in] frame 完整借用帧，普通服务只能在合法父请求内查询通用内部服务
    @return OK接纳/忽略迟到，其他负数拒绝
    */
    int32_t receiveService(uint64_t endpoint, const NetInterfaceFrame& frame);

    /** 无分配地关闭一代服务或外部连接；每轮至多64个关闭工作，未收完的端点排尾
    @param [in] endpoint 既有代次
    @param [in] reason 原因
    */
    void closeEndpoint(uint64_t endpoint, int32_t reason);

    /** 停止接受请求并关闭全部所属连接；回调中只登记停止，由下一次poll完成回收
    */
    void shutdown();

    /** 实际监听端口
    @return 未启动为0
    */
    uint32_t localPort() const;

    /** 全部外部A及内部B的在途总数
    @return 不超过4096
    */
    uint32_t pendingCount() const;

private:
    /** 登记服务表项的统一实现，区分内部依赖和通用本机处理器
    @param [in] product 产品号
    @param [in] internal 内部依赖，必须产品0
    @param [in] generic 通用自身处理器，必须产品0
    @param [in] limit 在途额度
    @param [out] endpoint 新代次
    @return NI_OK或错误
    */
    int32_t addPort(uint32_t product, bool internal, bool generic, uint32_t limit, uint64_t& endpoint);

    /** TLS完成后分配独立应用握手
    @param [in] peer TCP本轮连接号
    */
    virtual void onReady(uint64_t peer);

    /** 完成应用握手后交给路由，不在回调等待本地进程
    @param [in] peer TCP连接号
    @param [in] frame 完整借用帧
    @return 文件/事件背压为false且不提交副作用；其他帧消费，协议错误关闭相应连接
    */
    virtual bool onFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** TCP唯一终态触发请求回收
    @param [in] peer TCP连接号
    @param [in] reason 传输原因
    @param [in] detail TLS诊断
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);

    /** 在公共poll保护内推进一轮，不重入
    @return 是否仍运行
    */
    bool advance();

    /** 隔离物理关闭异常，端点回收由本地关闭队列负责
    @param [in] peer 物理代次
    @param [in] reason 首次关闭原因
    */
    void closePeer(uint64_t peer, int32_t reason);

    /** 已在dispatch保护内处理完整帧，不把控制元数据当作来源身份
    @param [in] endpoint 真实来源代次
    @param [in] frame 借用至当前dispatch返回
    @return 接纳/背压/协议错误
    */
    int32_t routeFrame(uint64_t endpoint, const NetInterfaceFrame& frame);

    /** 分派经过入口鉴别的请求/响应/取消
    @param [in] endpoint 来源代次
    @param [in] frame 完整帧
    @return OK或负数错误
    */
    int32_t dispatch(uint64_t endpoint, const NetInterfaceFrame& frame);

    /** 向可信端点投递一次并隔离宿主异常；调用方根据结果决定背压或关闭
    @param [in] endpoint 目标代次
    @param [in] frame 借用帧
    @param [in] origin 根上下文
    @return OK或负数错误
    */
    int32_t deliver(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin);

    /** 回传无业务体的传输失败
    @param [in] endpoint 来源代次
    @param [in] original 原请求帧字段
    @param [in] status NetInterfaceRpcStatus
    @param [in] root 已取得的可信根端点；0时从有效父请求或原来源取得
    */
    void reject(uint64_t endpoint, const NetInterfaceFrame& original, uint32_t status, uint64_t root = 0);

    /** 一条已从等待表取出的请求终止，通知原来源及目标
    @param [in] request 元数据
    @param [in] status 超时/取消/不可用
    @param [in] notifySource 是否通知来源
    */
    void terminate(const NetInterfaceRequest& request, uint32_t status, bool notifySource);

    /** 从根连接代次获取可信转发上下文
    @param [in] endpoint 根代次
    @return 不存在时全零
    */
    NetEndpointOrigin origin(uint64_t endpoint) const;

    /** 授权成功的文件回复绑定来源/服务，并改写客户端可见流号
    @param [in] request 刚完成的可信RPC元数据
    @param [in] rpc 服务成功体
    @param [out] payload 完整新RPC体，不含业务内容变更
    @return OK、LIMIT_EXCEEDED或协议错误
    */
    int32_t bindFileReply(const NetInterfaceRequest& request, const NetInterfaceRpcView& rpc, std::vector<uint8_t>& payload);

    /** 双向文件流投递，普通客户端不能指定目标或窃用其他连接的流
    @param [in] endpoint 真实来源
    @param [in] frame DATA/ACK/可信服务终止通知
    @return OK或负数，LIMIT_EXCEEDED无副作用可重试
    */
    int32_t routeFile(uint64_t endpoint, const NetInterfaceFrame& frame);

    /** 从认证服务接纳定向推送或授权分组撤销
    @param [in] endpoint 服务号
    @param [in] frame 完整EVENT
    @return OK或负数，背压无副作用
    */
    int32_t routeEvent(uint64_t endpoint, const NetInterfaceFrame& frame);

    /** 终止流、删除双索引并通知仍在线的另一端
    @param [in] stream 公网流号
    @param [in] reason 负数原因
    @param [in] skip 关闭的端点，不向其投递
    */
    void dropFile(uint64_t stream, int32_t reason, uint64_t skip = 0);

    /** 关闭端点时只遍历其双向链；正常poll通过固定期限堆每轮回收至多64条
    @param [in] endpoint 关闭端点，0只处理到期
    */
    void collectFiles(uint64_t endpoint);

    /** 禁止复制独占资源
    @param [in] other 不可复制实例
    */
    NetEndpointRouter(const NetEndpointRouter& other);

    /** 禁止复制独占资源
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetEndpointRouter& operator=(const NetEndpointRouter& other);

private:
    // 独占路由资源
    NetEndpointRouterState* m_state;
};