#pragma once
#include "NetGatewayServer.h"
#include "NetSender/NetSenderAPI.h"
#include <map>
#include <vector>

// 显式旧链路模式的一项产品服务认证凭据
struct NetGatewayServiceCredential
{
    // 允许登记的产品号，0保留给公共Account服务
    uint32_t m_product;
    // 独立产品的32字节服务凭据，仅用于显式旧链路认证
    uint8_t m_secret[32];
};

// 本机服务运输的认证状态，由网关唯一执行者维护
struct NetGatewayLocalPeer
{
    // 本机TLS就绪后5秒的认证截止时间，单位为单调毫秒
    uint64_t m_deadline;
    // 认证成功后分配的服务端点号；0表示尚未授权
    uint64_t m_endpoint;
};

// 同一CTask拥有公网网关和经过认证的本地服务适配；不按PID直接授权。
class NetGatewayLocalServer : private NetGatewayServiceSink, private NetSenderLocalListener
{
  public:
    /** 创建尚未监听的网关及本地收件箱
    */
    NetGatewayLocalServer();

    /** 同步收敛ProcessWork回调后才销毁收件箱
    */
    ~NetGatewayLocalServer();

    /** 启用当前进程的ProcessWork接收和本地TLS服务；无凭据时不启用
    @param [in] tls 网关证书/私钥；缺失entropy时采用系统强随机
    @param [in] credentials 独立产品秘密，1至32项，产品不能重复
    @return OK或负数；失败不留下ProcessWork回调
    */
    int32_t startLocal(const NetInterfaceTlsOptions& tls, const std::vector<NetGatewayServiceCredential>& credentials);

    /** 驱动网络、IPC及认证固定期限
    @return 是否仍运行
    */
    bool poll();

    /** 关闭两种通道、请求表及所有本地回调
    */
    void shutdown();

    /** 公网核心，供宿主配置监听及只读查询
    @return 借用对象，不得在本地回调重入poll
    */
    NetGatewayServer& gateway();

    /** 已认证且尚未关闭的本机业务服务数
    @return 不超过配置的产品数
    */
    uint32_t serviceCount() const;

  private:
    /** 附上32字节可信来源并复制入有界队列
    @param [in] endpoint 认证服务号
    @param [in] frame 原始帧
    @param [in] origin 根来源
    @return OK或负数
    */
    virtual int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetGatewayOrigin& origin);

    /** 要求关闭已登记的服务代次
    @param [in] endpoint 代次
    */
    virtual void close(uint64_t endpoint);

    /** 开始独立5秒身份认证窗口
    @param [in] peer 本地TLS通道
    @param [in] pid 声明，不作为授权
    */
    virtual void onLocalReady(uint64_t peer, int32_t pid);

    /** 服务身份校验成功之后才转交业务
    @param [in] peer TLS通道
    @param [in] frame 借用帧
    @return 始终消费，错误关闭该通道
    */
    virtual bool onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 撤销端点并清理它的在途请求
    @param [in] peer 通道
    @param [in] reason 原因
    */
    virtual void onLocalClosed(uint64_t peer, int32_t reason);

    /** 适配系统强随机，不使用进程号/时钟生成秘密
    @param [in] context 忽略
    @param [out] output 随机字节
    @param [in] size 字节数
    @return 0成功，否则负数
    */
    static int32_t entropy(void* context, uint8_t* output, uint32_t size);

    /** 单调认证时钟
    @return 毫秒
    */
    static uint64_t now();

  private:
    // 借给ProcessWork的回调收件箱，接收器收敛后才可析构
    NetSenderLocalInbox m_inbox;
    // 拥有本机TLS运输，单执行者驱动，不按PID直接授权
    NetSenderLocal m_local;
    // 产品号到认证凭据的独占副本，析构时擦除秘密
    std::map<uint32_t, NetGatewayServiceCredential> m_credentials;
    // 本机运输代次到认证截止时间及已登记服务端点
    std::map<uint64_t, NetGatewayLocalPeer> m_peers;
    // 已授权服务端点到本机运输代次，用于定向回送
    std::map<uint64_t, uint64_t> m_routes;
    // 公网路由核心的独占实例，借用本对象作为本机服务出口
    std::unique_ptr<NetGatewayServer> m_gateway;
    // 本对象已取得本进程接收器，停止时同步收敛其回调
    bool m_receiving;
    // 不再接纳本机或公网工作，shutdown保持幂等
    bool m_stopping;
};