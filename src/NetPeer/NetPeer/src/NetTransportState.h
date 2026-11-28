#pragma once
#include "NetTransport.h"
#include "NetHostSocket.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

// 单连接的唯一资源负责人。全部字段仅由transport所属执行者访问。
class NetTransportPeer
{
public:
    /** 创建尚未分配TLS的连接状态
    */
    NetTransportPeer();

public:
    // 实际TLS/帧实现，连接建立后创建
    std::unique_ptr<NetInterfaceConnection> m_connection;
    // 字节宿主返回而C++密文队列暂未接纳的后缀，最大16KiB
    std::vector<uint8_t> m_pending;
    // 待输入后缀的消费位置
    uint32_t m_pendingOffset;
    // 初次TCP连接或EOF排空的开始时间
    uint64_t m_started;
    // TCP已经建立
    bool m_connected;
    // onReady已经回报
    bool m_ready;
    // 已请求关闭，不再交付业务帧
    bool m_closing;
    // 原始字节宿主已报告本连接终态
    bool m_tcpClosed;
    // 最终原因及TLS底层诊断
    int32_t m_reason;
    // 原始宿主或TLS诊断码，不替代NI关闭原因
    int32_t m_detail;
};

// 私有实现，不跨DLL暴露容器；不自建执行线程。
class NetTransportState : public NetHostSocketListener
{
public:
    /** 初始化独占状态
    @param [in] config 实例独占的运输配置
    */
    explicit NetTransportState(const NetDirectTransportConfig& config);

    /** 收敛句柄并擦除自有私钥副本
    */
    ~NetTransportState();

    /** 有界复制及验证启动配置
    @param [in] options TLS配置
    @param [in] listener 终态前有效的回调
    @param [in] server 期望角色
    @return 0或负数错误
    */
    int32_t configure(const NetInterfaceTlsOptions& options, NetTransportListener* listener, bool server);

    /** 查询仍允许提交数据和更改编解码设置的 TLS 连接
    @param [in] id 本实例的连接号
    @return 借用至执行者下一次推进，连接未创建或已进入关闭流程时返回空
    */
    NetInterfaceConnection* writableConnection(uint64_t id) const;

    /** 推进一个连接，每轮最多推进 m_frameBudget 步；异常只关闭本连接
    @param [in] id 连接号
    @param [in,out] peer 独占状态
    */
    void pump(uint64_t id, NetTransportPeer& peer);

    /** 处理事件、时限及最终回收；禁止重入
    @return 是否仍拥有资源
    */
    bool poll();

    /** 请求有界关闭全部资源，回调内延迟收敛
    */
    void shutdown();

    /** 标记并关闭，首次原因保持不变
    @param [in] id 连接号
    @param [in] reason 终态原因
    */
    void close(uint64_t id, int32_t reason);

    /** 删除已经关闭的状态再回报终态，避免回调重入旧对象
    */
    void collectClosed();

    /** 取得系统单调毫秒，不使用墙上时间作截止时间
    @return 单调毫秒
    */
    static uint64_t now();

    /** 底层TCP连接成功，延后到poll创建TLS以限制一次宿主事件派发中的计算
    @param [in] peer 新连接号
    */
    virtual void onConnected(uint64_t peer);

    /** 复制密文前缀，暂不接纳的后缀至多16KiB并暂停TCP读取
    @param [in] peer 连接号
    @param [in] data 本次借用数据
    @param [in] size 字节数
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size);

    /** 可写通知，不在宿主回调内递归推进TLS
    @param [in] peer 连接号
    */
    virtual void onWritable(uint64_t peer);

    /** 接收已规范化的原始传输终态，不把宿主错误码解释成NI结果
    @param [in] peer 当前连接
    @param [in] reason NI关闭原因
    @param [in] detail 原始宿主诊断
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);

public:
    // 新连接使用的独立TLS帧池配置，连接启动后不可替换
    std::unique_ptr<NetConnectionPoolConfig> m_connectionPools;
    // 直连双向密文字节预算
    CEncodeDecodeTlsQueueLimits m_ciphertext;
    // 单帧正文合法性上限
    uint32_t m_payloadLimit;
    // 独占字节宿主，监听者就是本对象；不依赖具体TCP库
    NetHostSocket m_channel;
    // 实际连接记录池，必须晚于所有代次状态销毁
    std::unique_ptr<NetObjectPool<NetTransportPeer>> m_peerPool;
    // 有界连接表，删除只发生在collectClosed
    std::map<uint64_t, NetObjectReference<NetTransportPeer>> m_peers;
    // 借用到shutdown，所有调用都在所属执行者
    NetTransportListener* m_listener;
    // 指针均指向下面自有配置或借用随机上下文
    NetInterfaceTlsOptions m_options;
    // 客户端校验对端使用的信任根副本
    std::vector<uint8_t> m_authority;
    // 本端证书的独占PEM副本，m_options指向其存储
    std::vector<uint8_t> m_certificate;
    // 本端私钥的独占副本，析构时安全擦除
    std::vector<uint8_t> m_privateKey;
    // 客户端TLS校验的期望主机名，运行期不更换
    std::string m_hostname;
    // 本实例的启动配置已锁定
    bool m_started;
    // 已停止接纳新连接，等待所属运输收敛
    bool m_stopping;
    // 唯一执行者的poll重入守卫，不提供跨线程同步
    bool m_polling;
    // 本轮 poll 采样的单调毫秒，用于连接握手和关闭期限
    uint64_t m_now;
    // 每条连接每轮最多推进的步数，每步最多交付一帧
    uint32_t m_frameBudget;
    // 每轮最多输出的密文字节数
    uint32_t m_outputBudget;
    // EOF后允许余下输入排空的毫秒数
    uint32_t m_drainTimeoutMs;
};