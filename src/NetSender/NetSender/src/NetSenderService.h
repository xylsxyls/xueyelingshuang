#pragma once
#include "NetSenderLocal.h"

class NetSenderServiceState;

// 业务服务接收入口，来源由网关附加，账号权限仍由业务校验
class NetSenderAPI NetSenderServiceListener
{
  public:
    /** 虚析构
    */
    virtual ~NetSenderServiceListener();

    /** 网关已认证本产品且分配新代次
    @param [in] endpoint 本轮服务号，不作为账号身份
    */
    virtual void onServiceReady(uint64_t endpoint) = 0;

    /** 接收网关转发的业务或取消；来源只由已验证的网关附加
    @param [in] frame 本次借用原始RPC帧
    @param [in] origin 根客户端声明及网关代次
    @return true消费；false无副作用背压
    */
    virtual bool onServiceFrame(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin) = 0;

    /** 当前服务通道唯一终态，全部未完成业务须停止或返回不可用
    @param [in] reason 失败或关闭原因
    */
    virtual void onServiceClosed(int32_t reason) = 0;
};

// 各业务服务器复用的本地服务接入。宿主仍负责ProcessWork接收生命周期。
class NetSenderAPI NetSenderService : private NetSenderLocalListener
{
    // 仅注释main配置白盒服务状态，不改变运行时接入入口。
    friend class NetSenderServiceProxyTest;
  public:
    /** 发现本机已就绪服务端网关，启动或替换窗口返回NOT_FOUND
    @param [in] specified 0自动发现，正数固定到该PID
    @param [out] pid 唯一已就绪PID，失败为0
    @return NI_OK、NI_NOT_FOUND或多个就绪实例时NI_INVALID_STATE
    */
    static int32_t findGateway(int32_t specified, int32_t& pid);

    /** 绑定已注册到ProcessWork的收件箱
    @param [in] inbox 生命周期覆盖本对象
    */
    explicit NetSenderService(NetSenderLocalInbox& inbox);

    /** 停止后释放状态；监听者须仍存活
    */
    ~NetSenderService();

    /** 连接本机网关，TLS校验后才发送产品凭据
    @param [in] pid 网关进程号
    @param [in] tls 客户端TLS配置，根/域名不可空
    @param [in] product 0为通用内部服务，其余为独立产品
    @param [in] secret 32字节配置秘密，调用时复制
    @param [in] listener 本实例借用至shutdown
    @param [in] limit 1至128
    @return OK表示启动，认证结果通过回调给出
    */
    int32_t connect(int32_t pid, const NetInterfaceTlsOptions& tls, uint32_t product, const uint8_t* secret,
                    NetSenderServiceListener* listener, uint32_t limit = 32);

    /** 使用v3服务端点TLS；业务回调、父子请求、文件和推送复用原有接口
    @param [in] pid 本机网关PID，宿主已经初始化ProcessWork
    @param [in] product 独立服务号，0只供经过管理认证的内部服务访问
    @param [in] proof 服务器应用显式提供的注册证明，不可放入公共客户端SDK
    @param [in] context 借用证明上下文至shutdown
    @param [in] listener 借用至shutdown，回调内可以send但不能重入poll或销毁本对象
    @param [in] limit 业务处理器在途请求上限1至128，单客户端协商请求数也受此值限制
    @param [in] weight 网关分配权重1至1000
    @param [in] channels 在线通道容量1至512，0沿用limit；未登录握手也占位
    @return NI_OK启动，完成注册通过onServiceReady报告，不重放断线前业务
    */
    int32_t connectManaged(int32_t pid, uint32_t product, NetInterfaceLinkServiceProof proof, void* context,
        NetSenderServiceListener* listener, uint32_t limit = 32, uint32_t weight = 1, uint32_t channels = 0);

    /** 推进同一执行者上的I/O及固定5秒认证期限
    @return 是否仍运行
    */
    bool poll();

    /** 转发业务响应、内部子请求或取消；不允许重发HELLO或附带自造来源
    @param [in] frame RPC消息，退出后不借用输入
    @return OK或负数；LIMIT_EXCEEDED尚未接纳，可稍后尝试
    */
    int32_t send(const NetInterfaceFrame& frame);

    /** connectManaged后、首次poll前配置公开RPC白名单；默认空，不能放入含秘密或文件内容的类型。
    @param [in] types 公开RPC类型数组，调用时复制；count为0可为空
    @param [in] count 类型数量0至64，0清空白名单
    @return NI_OK或状态、参数、内存错误
    */
    int32_t configureCompression(const uint32_t* types, uint32_t count);

    /** 分配本服务代次唯一的内部子请求号，所有业务处理器共用此计数
    @param [out] requestId 成功非零，否则0；分配后不回退、不复用，按号递增发送
    @return OK、INVALID_STATE或编号耗尽LIMIT_EXCEEDED
    */
    int32_t nextRequestId(uint64_t& requestId);

    /** 经已认证服务向已知根连接推送，或撤销一组文件授权；不接受客户端自报目标
    @param [in] target 根连接号，revokeScope可为0广播匹配分组
    @param [in] product 普通服务限本产品，通用服务0可用0代表全产品通知
    @param [in] type 业务类型；撤销必须FILE_REVOKE_SCOPE
    @param [in] bytes 业务体/16字节非零分组
    @param [in] size 字节数
    @param [in] revokeScope 是否撤销文件分组，默认false
    @return OK接纳或负数；背压未接纳可重试
    */
    int32_t sendEvent(uint64_t target, uint32_t product, uint32_t type, const uint8_t* bytes, uint32_t size, bool revokeScope = false);

    /** 停止本代次，之后不得重用旧请求号和回调
    */
    void shutdown();

    /** 已认证端点
    @return 未认证或关闭时0
    */
    uint64_t endpoint() const;

  private:
    /** TLS完成后发送凭据
    @param [in] peer 通道
    @param [in] pid 对端声明
    */
    virtual void onLocalReady(uint64_t peer, int32_t pid);

    /** 验证认证响应并去除可信上下文
    @param [in] peer 通道
    @param [in] frame 完整帧
    @return 消费状态
    */
    virtual bool onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 回收唯一代次
    @param [in] peer 通道
    @param [in] reason 结果
    */
    virtual void onLocalClosed(uint64_t peer, int32_t reason);

    /** 禁止复制
    @param [in] other 独占对象
    */
    NetSenderService(const NetSenderService& other);

    /** 禁止赋值
    @param [in] other 独占对象
    @return 不可调用
    */
    NetSenderService& operator=(const NetSenderService& other);

  private:
    // 本模块独占状态；shutdown收敛后在本模块释放。
    NetSenderServiceState* m_state;
};