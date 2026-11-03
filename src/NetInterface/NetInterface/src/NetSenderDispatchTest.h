#pragma once
#include "NetInterface.h"
#include <vector>
#include <memory>

// 隔离运输，使用真实NI握手及RPC；不作为TLS互通证据
class NetSenderDispatchTestChannel : public NetClientChannel
{
public:
    /** 创建独占服务端握手，不创建网络
    @param [in] options 合法服务端能力、nonce及期限
    */
    explicit NetSenderDispatchTestChannel(const NetInterfaceHandshakeOptions& options);

    /** 先收敛借用监听者
    */
    virtual ~NetSenderDispatchTestChannel();

    /** 接纳单次Mock连接，事件仅在poll/shutdown交付
    @param [in] address 未使用
    @param [in] port 未使用
    @param [in] options 已由共用会话验证的配置
    @param [in] listener 借用至shutdown
    @param [out] peer 成功为1，失败0
    @return OK或INVALID_STATE
    */
    virtual int32_t connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
        NetTransportListener* listener, uint64_t& peer);

    /** 交付原始ready和共享握手输出
    @return 是否仍有借用连接
    */
    virtual bool poll();

    /** 将握手帧交给已有服务端握手，不实现另一套协议
    @param [in] peer 当前1
    @param [in] frame 借用完整帧
    @return 共用握手接纳结果
    */
    virtual int32_t send(uint64_t peer, const NetInterfaceFrame& frame);

    /** 标记关闭，下一poll交付
    @param [in] peer 当前1
    @param [in] reason 首个NI原因
    */
    virtual void close(uint64_t peer, int32_t reason);

    /** 收敛一次终态并解除借用
    */
    virtual void shutdown();

    /** 假宿主在send返回前已复制接纳全部输出，待回调响应不属于输出后缀
    @param [in] peer 本测试固定为1
    @return OK已交付，否则INVALID_STATE
    */
    virtual int32_t outputState(uint64_t peer) const;

private:
    /** 禁止复制独占握手
    @param [in] other 原通道
    */
    NetSenderDispatchTestChannel(const NetSenderDispatchTestChannel& other);

    /** 禁止复制借用
    @param [in] other 原通道
    @return 不可调用
    */
    NetSenderDispatchTestChannel& operator=(const NetSenderDispatchTestChannel& other);

private:
public:
    // 已交付的真实NI请求号，按进入通道顺序
    std::vector<uint64_t> m_requests;
    // 已复制的请求首字节，用于验证借用期
    std::vector<uint8_t> m_bytes;
    // 回包及其独占体，仅单执行者访问
    NetInterfaceFrame m_reply;
    // 本用例拥有的独立预期正文，用于逐字节核对传输结果
    std::vector<uint8_t> m_payload;
private:
    // 既有服务端握手算法
    std::unique_ptr<NetInterfaceHandshake> m_handshake;
    // 借用的客户端帧观察者
    NetTransportListener* m_listener;
    // 原始就绪是否已交付
    bool m_announced;
    // 是否请求关闭
    bool m_closing;
    // 首个关闭原因
    int32_t m_reason;
};

// 观察分派结果及回调重入保护
class NetSenderDispatchTestListener : public NetSessionListener
{
public:
    /** 初始化本轮计数，不获取资源
    */
    NetSenderDispatchTestListener();

    /** 记录真实握手；参数为本轮代次和协商额度
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] requests 本轮协商后的最大在途请求数
    @param [in] chunk 本轮协商后的最大块字节数
    */
    virtual void onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 记录唯一结果，并验证回调内禁止销毁／重入
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] id 实际请求号，必须对应本轮已接纳的请求
    @param [in] product 实际产品号
    @param [in] type 实际业务类型
    @param [in] status RPC终态状态
    @param [in] bytes 仅本次回调期间借用的正文，不保存裸指针
    @param [in] size 正文长度，单位字节
    */
    virtual void onSessionResult(uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 本组不接收业务流，出现即失败
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] frame 本次调用期间借用的完整帧
    @return true已消费，测试结果由独立观察状态记录
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 记录收敛完成
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] reason NI关闭原因
    @param [in] detail 底层诊断码
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 测试专用确定性随机，不用于正式网络
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [out] bytes 待填充的确定性测试字节，禁止用于正式密钥或随机数
    @param [in] size 输出缓冲的可写字节数
    @return 0填充完成，仅用于测试
    */
    static int32_t entropy(void* context, uint8_t* bytes, uint32_t size);
public:
    // 不拥有被测执行层
    NetSenderDispatch* m_dispatch;
    // 实际握手次数
    uint32_t m_ready;
    // 实际关闭次数
    uint32_t m_closed;
    // 错误回调／重入防护失败
    bool m_bad;
    // 唯一请求及结果
    std::vector<uint64_t> m_ids;
    // 按实际回调顺序保存的RPC结果码，与独立期望序列比较
    std::vector<uint32_t> m_status;
};