#pragma once
#include "NetInterfaceTypes.h"
#include "NetInterfaceTlsTypes.h"
#include "NetInterfaceLinkTypes.h"
#include "NetInterfaceTunnelTypes.h"

#ifdef __cplusplus
// 所有回调属于poll执行者；不得同步等待、销毁transport或重入poll。
class NetInterfaceAPI NetTransportListener
{
public:
    /** 析构接口，不拥有transport
    */
    virtual ~NetTransportListener();

    /** TLS认证完成，仍需应用握手及用户认证
    @param [in] peer 本transport内不复用的连接号
    */
    virtual void onReady(uint64_t peer) = 0;

    /** 处理完整帧；回调期间允许send或close
    @param [in] peer 连接号
    @param [in] frame 消息体仅在回调中借用，异步使用必须复制
    @return true已接纳；false不得产生副作用，下次poll重试并暂停该连接读取
    */
    virtual bool onFrame(uint64_t peer, const NetInterfaceFrame& frame) = 0;

    /** TCP及协议资源回收后的唯一终态
    @param [in] peer 连接号，异步连接失败也回报
    @param [in] reason 0主动/认证关闭，负数为NetInterface或libuv错误
    @param [in] detail TLS底层错误，无诊断时为0
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail) = 0;
};

// 客户端通道适配契约；桌面IPC与原生TCP向同一NetSession交付完整TLS帧。
class NetInterfaceAPI NetClientChannel
{
public:
    /** 不拥有listener
    */
    virtual ~NetClientChannel();

    /** 异步连接，地址解释由适配器规定；TLS配置仍必须验证
    @param [in] address 适配器地址
    @param [in] port 端口
    @param [in] options 客户端TLS
    @param [in] listener 借用至shutdown
    @param [out] peer 连接号或0
    @return NI_OK或负数
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint64_t& peer) = 0;

    /** 非阻塞推进
    @return 是否仍有资源
    */
    virtual bool poll() = 0;

    /** 接纳完整帧的有界副本
    @param [in] peer 连接
    @param [in] frame 借用帧
    @return NI_OK或负数，LIMIT_EXCEEDED无副作用
    */
    virtual int32_t send(uint64_t peer, const NetInterfaceFrame& frame) = 0;

    /** 标记关闭，由poll结算
    @param [in] peer 连接
    @param [in] reason 原因
    */
    virtual void close(uint64_t peer, int32_t reason = 0) = 0;

    /** 收敛资源及回调；监听者仍活着
    */
    virtual void shutdown() = 0;

    /** 应用握手后启用压缩扩展；旧适配器默认不支持，不复制协议。
    @param [in] peer 当前运输代次
    @param [in] types 公开RPC类型数组，调用时复制；count为0可为空
    @param [in] count 类型数量0至64，0清空白名单
    @param [in] negotiated 是否已完成双方压缩能力协商
    @return NI_OK或状态、参数、内存错误；默认不支持的适配器返回NI_NOT_SUPPORTED
    */
    virtual int32_t configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated);

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @param [in] peer 当前连接代次
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    // 统一NetSender的send／post保序均依赖真实输出状态；定制后端必须实现，不能把本层队列当作已交付。
    virtual int32_t outputState(uint64_t peer) const;
};

// 仅描述服务器端字节/帧适配能力，不分配任何物理传输
class NetInterfaceAPI NetServerChannel : public NetClientChannel
{
public:
    /** 不拥有监听者，具体适配器负责收敛资源
    */
    virtual ~NetServerChannel();

    /** 接纳服务器监听，实际认证/消息在poll交付
    @param [in] address 数值地址，借用至返回
    @param [in] port 0至65535，0供隔离测试
    @param [in] options 服务端TLS配置，证书/密钥由适配复制
    @param [in] listener 借用至shutdown完成
    @param [in] maxPeers 最大连接数1至256
    @return OK接纳或负数错误，不表示业务握手/账号授权完成
    */
    virtual int32_t listen(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint32_t maxPeers = 64) = 0;

    /** 实际服务器监听端口
    @return 未监听0，成功非零
    */
    virtual uint32_t localPort() const = 0;

    /** 包含正在关闭连接的拥有数
    @return 当前连接资源数
    */
    virtual uint32_t peerCount() const = 0;
};

/** 管理运输的借用事件接收者，须存活至shutdown结束，回调不得销毁正在驱动的运输
*/
class NetInterfaceAPI NetLinkTransportListener
{
public:
    /** 不拥有运输层
    */
    virtual ~NetLinkTransportListener();

    /** 管理认证成功；仍须注册服务/授权逻辑通道及业务端点TLS
    @param [in] peer 当前运输层内唯一代次
    @param [in] identity 已验证管理身份，借用至回调返回
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity) = 0;

    /** 处理管理请求，按已认证角色再次校验操作权限
    @param [in] peer 代次
    @param [in] frame 完整认证管理消息，借用至返回
    @return false无副作用背压，true接纳
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame) = 0;

    /** 转发业务密文；不能因外层通道号存在就免除归属/额度检查
    @param [in] peer 代次
    @param [in] record 借用至返回
    @return false无副作用背压，true接纳
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) = 0;

    /** 已回收管理/TCP资源的唯一终态
    @param [in] peer 代次
    @param [in] reason NI/TLS/libuv错误，0主动停止；不代表业务成功
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason) = 0;
};

class NetGatewayTransport;

// 每次新TLS尝试创建新的传输代次；返回对象由会话拥有，工厂由宿主保持到shutdown。
class NetInterfaceAPI NetGatewayTransportFactory
{
public:
    /** 析构工厂，调用方须先结束所有仍借用本工厂的会话
    */
    virtual ~NetGatewayTransportFactory();

    /** 为一次新管理连接创建独立运输，不复用上一代连接状态
    @return 新运输的所有权转移给会话，失败可返回空；会话负责停止后销毁
    */
    virtual NetGatewayTransport* create() = 0;
};

// 薄的管理运输适配；桌面由NetSender实现IPC，进程内由宿主显式提供适配。
class NetInterfaceAPI NetGatewayTransport
{
public:
    /** 不拥有监听者
    */
    virtual ~NetGatewayTransport();

    /** 接纳管理连接；地址由具体运输适配解释，身份始终由NetInterfaceLink验证
    @param [in] address 地址
    @param [in] port 端口
    @param [in] options 管理CLIENT配置
    @param [in] listener 借用监听者
    @param [out] peer 接纳代次，尚未认证
    @return NI_OK或错误
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
        NetLinkTransportListener* listener, uint64_t& peer) = 0;

    /** 同一执行者非阻塞推进
    @return 是否还有资源
    */
    virtual bool poll() = 0;

    /** 排队管理帧，LIMIT_EXCEEDED表示未接纳
    @param [in] peer 代次
    @param [in] frame 借用管理帧
    @return NI_OK或错误
    */
    virtual int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame) = 0;

    /** 排队业务密文，LIMIT_EXCEEDED表示未接纳
    @param [in] peer 代次
    @param [in] record 借用外层记录
    @return NI_OK或错误
    */
    virtual int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) = 0;

    /** 关闭所属代次并回收回调
    */
    virtual void shutdown() = 0;

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @param [in] peer 当前连接代次
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    virtual int32_t outputState(uint64_t peer) const;
};
#endif