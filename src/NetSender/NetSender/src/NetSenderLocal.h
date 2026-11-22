#pragma once
#include "NetSenderMacro.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetSenderLocalRawListener.h"
#include "NetSenderLocalInboxPools.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include <memory>

class NetSenderLocalState;
class NetSenderLocalInboxState;
struct NetSenderLocalInput;

// 原ProcessWork的单独消息类型，不改变既有枚举值或接收实现。
#define NET_SENDER_LOCAL_MESSAGE GATEWAY_LOCAL_MESSAGE

// 回调只拥有有界收件箱，不直接访问网络/业务状态。必须在ProcessWork停止回调后销毁。
class NetSenderAPI NetSenderLocalInbox : public ProcessReceiveCallback
{
    friend class NetSenderLocalState;
public:
    /** 创建停用的分通道收件箱，不注册或初始化ProcessWork；容量由实际池决定
    */
    NetSenderLocalInbox();

    /** 在首次start或注册到ProcessWork之前替换实际接收池配置
    不得与任何本对象调用并发；外部不得仍持有旧收件箱输入租约。
    @param [in] pools 数据、控制、OPEN、入口节点和路由池配置
    @return NI_OK或配置、内存、状态错误；失败保留原配置
    */
    int32_t configurePools(const NetSenderLocalInboxPools& pools);

    /** 释放队列；宿主必须已经uninitReceive收敛原回调
    */
    ~NetSenderLocalInbox();

    /** 原ProcessWork回调仅做有界复制，忽略其他消息类型
    @param [in] sendPid 原IPC头中的进程号，不单独用作业务授权
    @param [in] buffer 本次借用数据
    @param [in] length 字节数
    @param [in] type NET_SENDER_LOCAL_MESSAGE才接纳
    */
    virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

    /** 唯一协议owner停止接纳并等在途回调退出后释放缓存，不改ProcessWork单例；不能在原接收回调内调用
    */
    void stop();

private:
    /** 新传输实例启用收件箱，清除旧代次队列
    @param [in] server 是否允许未识别OPEN
    */
    void start(bool server);

    /** 绑定本箱拥有的随机目标，仅运输owner调用
    @param [in] generation 16字节非零本地随机数
    @param [in] pid 已绑定的实际发送PID
    @param [in] peer owner分配且不复用的非零通道号
    @param [in] remote 服务端已经验证的远端16字节代次；客户端尚未知时为空
    @return OK或有界错误
    */
    int32_t allowGeneration(const uint8_t* generation, int32_t pid, uint64_t peer, const uint8_t* remote = nullptr);

    /** 删除已收敛目标，不再为其分配输入
    @param [in] generation 原16字节目标
    */
    void removeGeneration(const uint8_t* generation);

    /** 移走一条消息，只允许所属传输执行者调用
    @param [out] input 本DLL内部拥有的字节
    @return READY一条输入或单通道失败通知，NEED_MORE空；失败不终止其他通道
    */
    int32_t take(NetSenderLocalInput& input);

    /** 禁止复制独占同步对象
    @param [in] other 不可复制实例
    */
    NetSenderLocalInbox(const NetSenderLocalInbox& other);

    /** 禁止复制独占同步对象
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetSenderLocalInbox& operator=(const NetSenderLocalInbox& other);

private:
    // 本模块独占状态，析构在本模块停止后释放。
    NetSenderLocalInboxState* m_state;
};

// 本机TLS通道监听者，回调仅由所属协议执行者调用
class NetSenderAPI NetSenderLocalListener
{
public:
    /** 析构接口
    */
    virtual ~NetSenderLocalListener();

    /** TLS就绪；仍须完成本地服务/客户端身份认证
    @param [in] peer 本传输内不复用的代次号
    @param [in] pid 已绑定该TLS通道的本地PID声明
    */
    virtual void onLocalReady(uint64_t peer, int32_t pid) = 0;

    /** 交付完整解密帧；不可同步等待或重入poll
    @param [in] peer 通道代次号
    @param [in] frame 本次借用，异步保存必须复制
    @return true消费；false不得产生副作用，暂停后续输入确认
    */
    virtual bool onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame) = 0;

    /** 已释放协议和IPC句柄后的唯一终态
    @param [in] peer 本轮代次号
    @param [in] reason 原因
    */
    virtual void onLocalClosed(uint64_t peer, int32_t reason) = 0;
};

// 单执行者本地TLS通道。宿主先登记inbox，再初始化ProcessWork；单块至少16452字节。
// listen/connect及Raw入口检查容量；缺接收器返回INVALID_STATE，容量不足返回BUFFER_TOO_SMALL。
// 默认64块，块数只影响并发预算；增加块数不能替代单块长度。
// 不修改/初始化/关闭ProcessWork，不创建线程；CTask宿主拥有接收及停止次序。
class NetSenderAPI NetSenderLocal
{
public:
    /** 首次listen/connect前配置透明传输输出的实际正文池
    @param [in] config 五级正文池配置，不额外设置排队条数上限
    @return NI_OK或配置、内存、状态错误；失败保留原池
    */
    int32_t configureOutputPool(const NetBufferPoolConfig& config);

    /** 在listen/connect之前配置实际通道对象池；只由生命周期owner串行调用
    @param [in] config 真实初始/最大槽数及增长策略，包含通道内发送器和生命周期状态
    @return NI_OK或配置/内存/状态错误；失败不替换旧配置
    */
    int32_t configurePeerPool(const LockFreeHashMapPoolConfig& config);

    /** 首次连接前复制TLS帧池配置；不得与本对象其他调用并发
    池资源在创建物理连接时申请并验证，失败走原有连接失败路径；不修改其他连接。
    @param [in] config 独立数据/控制记录及正文的基础池配置
    @return NI_OK配置已复制；状态、内存或不支持错误
    */
    int32_t configureConnectionPools(const NetConnectionPoolConfig& config);

    /** 借用安全收件箱，生命周期须覆盖本对象
    @param [in] inbox 已注册到原ProcessWork的收件箱
    */
    explicit NetSenderLocal(NetSenderLocalInbox& inbox);

    /** 收敛本实例，监听者仍须存活，不可在自身回调中销毁
    */
    ~NetSenderLocal();

    /** 接受本机TLS连接，不依据PID或进程名称自动信任服务
    @param [in] tls server=1，证书/私钥复制，entropy必须提供强随机且上下文持续有效
    @param [in] listener 借用到shutdown
    @param [in] maximum 非零启动预热提示，默认32；实际总容量由configurePeerPool决定
    @return OK或负数错误
    */
    int32_t listen(const NetInterfaceTlsOptions& tls, NetSenderLocalListener* listener, uint32_t maximum = 32);

    /** 连接指定本机网关PID，TLS仍必须验证证书及域名
    @param [in] pid 正数，不能为本进程
    @param [in] tls server=0，根及域名复制，entropy必须有效
    @param [in] listener 借用到shutdown
    @param [out] peer 接纳后的非零代次，失败清零；此时尚未TLS就绪
    @return OK或负数错误
    */
    int32_t connect(int32_t pid, const NetInterfaceTlsOptions& tls, NetSenderLocalListener* listener, uint64_t& peer);

    /** 监听NTW3密文字节，不创建本机TLS终止点；只供透明网关/端点管理适配
    @param [in] entropy 强随机代次源，不能为空
    @param [in] context 随机上下文，借用至shutdown
    @param [in] listener 借用监听者
    @param [in] maximum 非零启动预热提示；实际总容量由configurePeerPool决定
    @return NI_OK或错误；身份仍由端点管理/业务TLS验证
    */
    int32_t listenRaw(NetInterfaceEntropyFunction entropy, void* context, NetSenderLocalRawListener* listener, uint32_t maximum = 32);

    /** 打开透明本机路由，绝不依据PID信任服务
    @param [in] pid 网关发送地址
    @param [in] entropy 强随机源
    @param [in] context 随机上下文
    @param [in] listener 借用监听者
    @param [out] peer 成功非零，否则0
    @return NI_OK或错误
    */
    int32_t connectRaw(int32_t pid, NetInterfaceEntropyFunction entropy, void* context, NetSenderLocalRawListener* listener, uint64_t& peer);

    /** 复制一个外层路由记录，分片使用NIP2有序编号，ACK不限制发送数量
    @param [in] peer 原生IPC代次
    @param [in] record NTW3记录，正文必须已由端点保护
    @return NI_OK、NI_LIMIT_EXCEEDED未接纳或错误
    */
    int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 分轮驱动收件箱/TLS及累计确认，绝不调用旧阻塞send
    @return 启动且未停止为true
    */
    bool poll();

    /** 复制帧到共享C++连接队列，返回不代表业务成功
    @param [in] peer TLS就绪代次
    @param [in] frame 借用完整帧
    @return OK、LIMIT_EXCEEDED背压或其他错误
    */
    int32_t send(uint64_t peer, const NetInterfaceFrame& frame);

    /** 标记关闭，在所属poll中唯一回报终态
    @param [in] peer 本轮代次
    @param [in] reason 终止原因
    */
    void close(uint64_t peer, int32_t reason = 0);

    /** 停止本地传输及收件箱，不操作宿主ProcessWork单例
    */
    void shutdown();

    /** 当前通道数
    @return 包含标记关闭但尚未回报的通道
    */
    uint32_t peerCount() const;

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @param [in] peer 当前连接代次
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    int32_t outputState(uint64_t peer) const;

private:
    /** 禁止复制协议所有权
    @param [in] other 不可复制实例
    */
    NetSenderLocal(const NetSenderLocal& other);

    /** 禁止复制协议所有权
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetSenderLocal& operator=(const NetSenderLocal& other);

private:
    // 本模块独占运输状态，shutdown后在本模块释放。
    NetSenderLocalState* m_state;
};