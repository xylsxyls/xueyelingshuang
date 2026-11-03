#pragma once
#include "NetSenderLink.h"

class NetSenderServiceGatewayState;
class NetSenderServiceOutgoingTransport;

// 服务注册及每条业务TLS通道的监听者，生命周期覆盖所属接入对象
class NetSenderAPI NetSenderServiceGatewayListener
{
public:
    /** 不拥有服务接入对象
    */
    virtual ~NetSenderServiceGatewayListener();

    /** 本机注册及服务身份验证完毕，允许网关分配新客户端
    */
    virtual void onGatewayServiceReady() = 0;

    /** 某一客户端的业务TLS及能力握手已完成
    @param [in] channel 本代次内不复用的逻辑号
    @param [in] role 网关管理认证的来源角色，不是用户/VIP权限
    @param [in] origin 本次TLS内的设备/平台声明及库分配来源号
    */
    virtual void onGatewayClientReady(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin) = 0;

    /** 交付业务明文；请求映射、账号、文件授权由上层公共服务适配处理
    @param [in] channel 已完成能力握手的通道
    @param [in] role 可信管理来源角色
    @param [in] frame 完整业务帧，只借用到返回
    @param [in] origin 设备/平台是声明，不能单独作为身份授权
    @return false无副作用背压，true消费
    */
    virtual bool onGatewayClientFrame(uint64_t channel, uint32_t role, const NetInterfaceFrame& frame,
        const NetInterfaceOrigin& origin) = 0;

    /** 单个逻辑客户端关闭，不影响其他已固定的实例通道
    @param [in] channel 逻辑号
    @param [in] reason 原因，0不表示请求已成功
    */
    virtual void onGatewayClientClosed(uint64_t channel, int32_t reason) = 0;

    /** 当前本机管理代次结束，所有客户端先回报关闭
    @param [in] reason 原因
    */
    virtual void onGatewayServiceClosed(int32_t reason) = 0;
};

// 服务端管理注册和每客户端业务TLS的通用组合；不持有CA私钥或内置服务注册凭证。
// 宿主先注册inbox并初始化ProcessWork，停止本对象后才可uninitReceive。
class NetSenderAPI NetSenderServiceGateway : private NetLinkTransportListener
{
public:
    /** 借用已经注册的本机收件箱
    @param [in] inbox 生命周期覆盖本对象
    */
    explicit NetSenderServiceGateway(NetSenderLocalInbox& inbox);

    /** 收敛所属通道并擦除业务私钥副本，监听者仍须有效
    */
    ~NetSenderServiceGateway();

    /** 自动生成业务私钥/CSR，管理认证后由网关签发对应逻辑服务证书
    @param [in] pid 本机网关PID，仅为运输地址
    @param [in] product 逻辑服务号，0只允许可信服务访问
    @param [in] proof 服务器专属宿主的注册证明回调，不能编进公共客户端库
    @param [in] context 注册证明上下文，借用至shutdown
    @param [in] listener 借用监听者
    @param [in] limit 并发逻辑连接及协商在途上限，1至128
    @param [in] weight 新连接分配权重，1至1000
    @param [in] channels 允许的逻辑客户端数，0采用limit，非零为1至512
    @return NI_OK启动或错误，不代表业务可用
    */
    int32_t connect(int32_t pid, uint32_t product, NetInterfaceLinkServiceProof proof, void* context,
        NetSenderServiceGatewayListener* listener, uint32_t limit = 32, uint32_t weight = 1, uint32_t channels = 0);

    /** 单执行者有界驱动注册/每通道TLS/握手/背压/关闭
    @return 当前接入是否仍启用
    */
    bool poll();

    /** 在指定业务TLS内排队完整明文，由公共库加密
    @param [in] channel 真实已授权逻辑号
    @param [in] frame 业务帧，借用到返回
    @return NI_OK、NI_LIMIT_EXCEEDED无副作用或其他错误
    */
    int32_t send(uint64_t channel, const NetInterfaceFrame& frame);

    /** 请求关闭单个逻辑号，通知网关回收，禁止重放原密文
    @param [in] channel 逻辑号
    @param [in] reason 原因
    */
    void close(uint64_t channel, int32_t reason = 0);

    /** 关闭本机代次及全部所属客户端，不操作ProcessWork单例
    */
    void shutdown();

    /** 本机服务已经注册，不能代替具体业务操作完成
    @return 是否就绪且未停止
    */
    bool ready() const;

    /** 取得已经完成业务握手的四项协商值，不接受客户端外层自报
    @param [in] channel 当前逻辑号
    @param [out] fields 平台、能力、在途、块上限，失败清零
    @return NI_READY或NOT_FOUND/INVALID_STATE
    */
    int32_t clientInfo(uint64_t channel, uint32_t fields[4]) const;

private:
    friend class NetSenderServiceOutgoingTransport;
    // 仅隔离测试建立不联网的生命周期夹具，不添加运行时入口。
    friend class NetSenderServiceOutgoingTest;
public:
    /** 仅在尚无业务客户端时配置公开RPC压缩白名单
    @param [in] types 调用期间借用的消息类型数组，count为0时可为空
    @param [in] count 允许压缩的类型数，0至64
    @return NI_OK或参数、状态、内存错误
    */
    int32_t configureCompression(const uint32_t* types, uint32_t count);
private:
    /** 记录已验证网关管理身份，并准备提交服务注册
    @param [in] peer 本管理运输代次
    @param [in] identity 回调期间借用的网关身份及证书
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 处理服务注册、通道建立和额度等管理控制
    @param [in] peer 当前管理运输代次
    @param [in] frame 回调期间借用的完整控制帧
    @return true已消费或拒绝当前代次，false为未消费背压
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 按逻辑通道转交客户端密文或出站依赖记录
    @param [in] peer 当前管理运输代次
    @param [in] record 回调期间借用，接纳时保存必要副本
    @return true已消费；false表示保留输入等待背压解除
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 标记管理代次失效，由外层poll收敛业务客户端及服务终态
    @param [in] peer 原管理代次
    @param [in] reason NI关闭原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);

    /** 禁止复制活动服务接入及回调地址
    @param [in] other 不可复制对象
    */
    NetSenderServiceGateway(const NetSenderServiceGateway& other);

    /** 禁止赋值活动服务接入
    @param [in] other 不可赋值对象
    @return 不可调用
    */
    NetSenderServiceGateway& operator=(const NetSenderServiceGateway& other);
    // 本模块独占服务注册、业务TLS及出站依赖状态
    NetSenderServiceGatewayState* m_state;
};