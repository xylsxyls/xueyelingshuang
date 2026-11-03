#pragma once
#include "NetPeerMacro.h"
#include "NetInterface/NetInterfaceAPI.h"

class NetRelayTransportState;

// 回调在poll执行者上，通道建立回调只表示TCP连接，不表示身份已认证。
class NetPeerAPI NetRelayTransportListener
{
public:
    /** 不拥有运输层
    */
    virtual ~NetRelayTransportListener();

    /** TCP连接建立，上层必须完成NTW3协商和管理认证再开放业务路由
    @param [in] peer 当前运输层内不复用的代次
    */
    virtual void onRelayConnected(uint64_t peer) = 0;

    /** 借用完整路由记录，网关不能解密业务记录
    @param [in] peer 当前TCP代次
    @param [in] record 借用至回调结束
    @return true已接纳；false必须无副作用，暂停读并由下次poll重试
    */
    virtual bool onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record) = 0;

    /** 资源回收后的唯一终态
    @param [in] peer 已结束代次
    @param [in] reason 0主动或TCP关闭，负数为错误，TLS认证关闭仍由端点判断
    */
    virtual void onRelayClosed(uint64_t peer, int32_t reason) = 0;
};

// NTW3字节中继，不选择网络实现，不包含TLS密钥或压缩；管理会话与业务端点另用NetInterfaceConnection。
// 同一对象一个执行者，不建线程，不复用已启动实例；停止后重新构造。
class NetPeerAPI NetRelayTransport
{
public:
    /** 创建独占事件循环
    */
    NetRelayTransport();

    /** 收敛所有回调，监听者必须仍存活
    */
    ~NetRelayTransport();

    /** 监听数值地址；公网监听必须显式指定0.0.0.0或::
    @param [in] address 数值IP
    @param [in] port 0用于本机测试，生产为1至65535
    @param [in] listener 借用至shutdown，不可空
    @param [in] maxPeers 1至256
    @return NI_OK或底层错误
    */
    int32_t listen(const char* address, uint32_t port, NetRelayTransportListener* listener, uint32_t maxPeers = 64);

    /** 异步连接；TCP建立最多5秒，上层仍必须给管理/TLS握手设期限
    @param [in] address 数值IP
    @param [in] port 1至65535
    @param [in] listener 借用至shutdown
    @param [out] peer 成功为本代次，否则0
    @return NI_OK或底层错误
    */
    int32_t connect(const char* address, uint32_t port, NetRelayTransportListener* listener, uint64_t& peer);

    /** 推进网络、有界队列和关闭回收，不允许重入
    @return 是否还有监听或连接
    */
    bool poll();

    /** 接纳外层头和密文副本；仅单执行者调用，不等待下层接纳
    每连接业务最多4*16408字节/32条、控制最多2*16408字节/32条；两类共用FIFO
    成功后由poll交给宿主，outputState为OK才表示本层全部交付；不是NetSender的同步send
    @param [in] peer 已建TCP代次
    @param [in] record 一条完整记录，返回后不借用数据
    @return NI_OK接纳、NI_LIMIT_EXCEEDED未接纳可重试、其他负数错误
    */
    int32_t send(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 标记关闭一代TCP，丢弃尚未送出的记录；不自动重放
    @param [in] peer 代次
    @param [in] reason 原因
    */
    void close(uint64_t peer, int32_t reason = 0);

    /** 停止接受和发送，在回调内只请求停止，由外层poll收敛
    */
    void shutdown();

    /** 查询实际监听端口，供动态登记和隔离测试使用
    @return 未监听时0
    */
    uint32_t localPort() const;

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @param [in] peer 当前连接代次
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    int32_t outputState(uint64_t peer) const;

private:
    /** 禁止复制独占运输状态
    @param [in] other 原实例
    */
    NetRelayTransport(const NetRelayTransport& other);

    /** 禁止赋值独占运输状态
    @param [in] other 原实例
    @return 不可调用
    */
    NetRelayTransport& operator=(const NetRelayTransport& other);

private:
    // 独占连接和宿主；析构前先shutdown收敛
    NetRelayTransportState* m_state;
};