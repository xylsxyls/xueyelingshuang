#pragma once
#include "NetInterfaceTransport.h"

class NetGatewayClientChannelState;

// 桌面IPC、原生及Wasm宿主共用的业务TLS通道；同一执行者串行调用，不创建线程
class NetInterfaceAPI NetGatewayClientChannel : public NetClientChannel, private NetLinkTransportListener
{
public:
    /** 固定一个逻辑目标，通道只启动一次，重连创建新实例
    @param [in] product 目标服务号
    @param [in] external 借用到shutdown；空仅允许构造，connect明确返回NOT_SUPPORTED
    */
    explicit NetGatewayClientChannel(uint32_t product, NetGatewayTransport* external = nullptr);

    /** 监听者有效期间收敛全部资源
    */
    virtual ~NetGatewayClientChannel();

    /** 管理认证、申请目标实例后再与业务端点建立TLS；固定内置CA及service.<编号>
    @param [in] address 网关数值IP，IPC适配仅作为统一入口参数
    @param [in] port 网关端口，1至65535
    @param [in] options server必须0，强随机回调及期限必需；不采用外部根/域名
    @param [in] listener 借用监听者
    @param [out] peer 本次业务代次，成功非零
    @return NI_OK启动或错误；就绪须等待业务TLS回调
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint64_t& peer);

    /** 有界推进认证、额度和数据，捕获适配器异常；禁止重入
    @return 是否尚有本次通道资源
    */
    virtual bool poll();

    /** 复制完整业务帧到TLS队列；不等于下层输出已全部交付
    @param [in] peer 必须为本次业务代次1
    @param [in] frame 借用至返回，产品必须匹配固定目标
    @return OK接纳、LIMIT_EXCEEDED未接纳或错误
    */
    virtual int32_t send(uint64_t peer, const NetInterfaceFrame& frame);

    /** 记录首个停止原因，允许在本对象回调内调用
    @param [in] peer 必须为本次业务代次1
    @param [in] reason 首个NI/TLS原因
    */
    virtual void close(uint64_t peer, int32_t reason = 0);

    /** 非回调执行者同步收尾；回调内仅请求停止，禁止在回调销毁对象
    */
    virtual void shutdown();

    /** 设置公开帧类型的压缩白名单
    @param [in] peer 本次业务代次1
    @param [in] types 借用类型数组
    @param [in] count 数组长度，最大64
    @param [in] negotiated 能力是否协商通过
    @return 统一连接层的结果，停止后明确拒绝
    */
    virtual int32_t configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated);

public:
    /** 查询本层待发数据是否全部交给下层；只由协议执行者调用
    @param [in] peer 当前连接代次
    @return OK全部交付，READY仍有本层待发数据，负数失败；不表示远端收到
    */
    virtual int32_t outputState(uint64_t peer) const;

private:
    /** 提交缓存的OPEN；背压不重编码、不提前推进阶段
    */
    void sendOpen();

    /** 合并并提交已接纳密文的额度，成功前不扣待返还值
    */
    void sendCredit();

    /** 每轮至多32条密文，下层接纳后才扣额度和消费TLS输出
    */
    void sendOutput();

    /** 推进TLS及至多32条应用帧，拒收保留原帧及首次期限
    @param [in] now 当前单调毫秒
    */
    void advance(uint64_t now);

    /** 先进入终态再停止运输、释放协议并取走唯一关闭通知，外部异常不得逃逸
    */
    void finish();

    /** 当前管理代次认证为节点后允许OPEN
    @param [in] peer 管理代次
    @param [in] identity 已验证角色
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 校验OPEN响应、额度及关闭事件，不解释业务正文
    @param [in] peer 管理代次
    @param [in] frame 借用管理帧，最大16KiB
    @return 已消费，包括协议错误关闭
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 接纳本通道密文，已有积压时不消费新记录
    @param [in] peer 管理代次
    @param [in] record 借用至返回的完整16KiB内记录
    @return 接纳或拒绝重试，拒绝没有预算副作用
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 将当前管理代次失效归并到唯一关闭路径
    @param [in] peer 管理代次
    @param [in] reason 管理关闭原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);

    /** 禁止复制独占协议状态
    @param [in] other 原对象
    */
    NetGatewayClientChannel(const NetGatewayClientChannel& other);

    /** 禁止复制独占协议状态
    @param [in] other 原对象
    @return 不可调用
    */
    NetGatewayClientChannel& operator=(const NetGatewayClientChannel& other);
private:
    // 本模块拥有的单执行者状态
    NetGatewayClientChannelState* m_state;
};