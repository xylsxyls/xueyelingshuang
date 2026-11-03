#pragma once
#include "NetSenderMacro.h"
#include "NetInterface/NetInterfaceAPI.h"

class NetSenderServiceGateway;
class NetSenderServiceGatewayState;

// 一次服务依赖连接的唯一阶段，管理认证由所属服务代次提供。
enum NetSenderServiceOutgoingPhase
{
    // 尚未取得逻辑号
    NET_SERVICE_OUTGOING_IDLE,
    // 已取得逻辑号，等待交付管理就绪
    NET_SERVICE_OUTGOING_ATTACHED,
    // 管理身份已通知，允许提交一次OPEN
    NET_SERVICE_OUTGOING_ANNOUNCED,
    // OPEN完整接纳，等待网关应答
    NET_SERVICE_OUTGOING_OPEN_WAIT,
    // 已接受成功应答，允许密文及额度控制
    NET_SERVICE_OUTGOING_OPEN,
    // 已解除监听和拥有者借用，不可再次连接
    NET_SERVICE_OUTGOING_CLOSED
};

// 复用已经认证的SERVICE管理连接。只供服务器端适配，不携带注册凭证。
// 宿主每轮先poll服务接入，再poll业务客户端；本对象不递归驱动宿主。
class NetSenderAPI NetSenderServiceOutgoingTransport : public NetGatewayTransport
{
public:
    /** 借用已完成注册的服务接入，只允许一个当前出站通道
    @param [in] gateway 生命周期覆盖本对象
    */
    explicit NetSenderServiceOutgoingTransport(NetSenderServiceGateway& gateway);

    /** 脱离监听者并请求关闭自身逻辑号，不关闭服务管理连接
    */
    virtual ~NetSenderServiceOutgoingTransport();

    /** 借用已经认证的本地服务代次；以下全部接口只由同一协议执行者调用
    @param [in] address 非空逻辑地址，不建立额外网络连接
    @param [in] port 1至65535的逻辑端口
    @param [in] options v3 CLIENT角色；管理身份沿用所属服务
    @param [in] listener 借用至shutdown，回调内不得析构本对象
    @param [out] peer 本次唯一逻辑号，失败为0
    @return OK已接纳、INVALID_STATE所属服务未就绪或其他错误
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
        NetLinkTransportListener* listener, uint64_t& peer);

    /** 推进本次借用，不创建线程或递归驱动宿主；回调可shutdown
    @return 是否仍借用有效监听者
    */
    virtual bool poll();

    /** 有界复制并映射OPEN或CREDIT；失败不提交阶段或产品
    @param [in] peer 当前逻辑号
    @param [in] frame 正文最多16KiB，借用到返回
    @return 管理链接接纳结果，LIMIT_EXCEEDED无副作用
    */
    virtual int32_t sendControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 映射已打开通道的密文，不解密或压缩
    @param [in] peer 当前逻辑号
    @param [in] record 通道1、产品匹配的密文，借用到返回
    @return 管理链接接纳结果，不表示远端业务完成
    */
    virtual int32_t sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 解除监听并请求关闭本通道；可重复，不关闭宿主或回调已解除监听者
    */
    virtual void shutdown();

    /** 检查是否交给管理IPC；共享链接其他待发数据可保守延迟OK
    @param [in] peer 当前逻辑号
    @return OK本层及管理链接全部交付，READY仍有待发，负数代次失效
    */
    virtual int32_t outputState(uint64_t peer) const;

private:
    friend class NetSenderServiceGateway;
    // 仅隔离测试观察阶段和真实映射，不增加运行时测试入口。
    friend class NetSenderServiceOutgoingTest;

    /** 交付网关管理响应，恢复调用方独立的逻辑号
    @param [in] frame 当前出站通道的认证控制帧
    @return false无副作用背压
    */
    bool control(const NetInterfaceFrame& frame);

    /** 交付当前出站通道的密文
    @param [in] record 由宿主先验证号域和拥有者
    @return false无副作用背压
    */
    bool record(const NetInterfaceTunnelRecord& record);

    /** 管理代次失效时唯一通知借用监听者
    @param [in] reason 原因
    @param [in] notifyRemote 是否请求远端同时回收本通道
    */
    void closed(int32_t reason, bool notifyRemote = false);

    /** 无分配解除借用，远端清理由所属服务poll执行
    @param [in] notifyRemote 是否需要发送逻辑关闭
    */
    void detach(bool notifyRemote);

    /** 禁止复制代次所有权
    @param [in] other 不可复制实例
    */
    NetSenderServiceOutgoingTransport(const NetSenderServiceOutgoingTransport& other);

    /** 禁止复制代次所有权
    @param [in] other 不可复制实例
    @return 不可调用
    */
    NetSenderServiceOutgoingTransport& operator=(const NetSenderServiceOutgoingTransport& other);
    // 借用服务状态，生命周期覆盖本对象
    NetSenderServiceGatewayState* m_owner;
    // 终态提交前摘除的借用监听者
    NetLinkTransportListener* m_listener;
    // 管理链接内不复用的逻辑号
    uint64_t m_channel;
    // 本次OPEN的管理请求号
    uint64_t m_request;
    // OPEN接纳后固定的目标产品
    uint32_t m_product;
    // 当前逻辑连接的唯一阶段
    NetSenderServiceOutgoingPhase m_phase;
    // 单执行者重入守卫，不是跨线程同步锁
    bool m_polling;
};