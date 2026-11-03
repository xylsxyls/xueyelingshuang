#pragma once
#include "NetSender/NetSenderAPI.h"
#include <set>

// 真实独立服务进程联调入口；固定service.1回显，不接受或导出任意CSR/身份秘密。
class NetGatewayServiceProbe : public NetSenderServiceGatewayListener, private NetSessionListener
{
public:
    // 独立测试宿主允许持续压力；正式网关的显式探针仍限制十分钟。
#ifdef NET_GATEWAY_TEST_INSTANCE
    static const uint32_t MaximumDurationMs = 3600000;
#else
    // 正式进程显式测试模式的最长运行时间，单位毫秒
    static const uint32_t MaximumDurationMs = 600000;
#endif
    /** 初始化本次固定回显服务
    */
    NetGatewayServiceProbe();

    /** 记录真实服务注册完成，输出测试宿主就绪标记
    */
    virtual void onGatewayServiceReady();

    /** 记录已建立的业务客户端，检查产品及角色归属
    @param [in] channel 网关分配的逻辑通道号
    @param [in] role 该通道经管理协议确认的角色
    @param [in] origin 本次调用期间借用的网关可信来源
    */
    virtual void onGatewayClientReady(uint64_t channel, uint32_t role, const NetInterfaceOrigin& origin);

    /** 检查业务TLS帧并回显或施加本用例的受控背压
    @param [in] channel 网关分配的逻辑通道号
    @param [in] role 该通道经管理协议确认的角色
    @param [in] frame 本次调用期间借用的完整帧，背压时由调用方保留
    @param [in] origin 本次调用期间借用的网关可信来源
    @return true已消费，false表示本次未消费并保持背压
    */
    virtual bool onGatewayClientFrame(uint64_t channel, uint32_t role, const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 清理本轮通道的人工背压记录并输出关闭标记
    @param [in] channel 网关分配的逻辑通道号
    @param [in] reason 本连接或服务的NI关闭原因
    */
    virtual void onGatewayClientClosed(uint64_t channel, int32_t reason);

    /** 记录服务注册连接终态，异常关闭使本用例失败
    @param [in] reason 本连接或服务的NI关闭原因
    */
    virtual void onGatewayServiceClosed(int32_t reason);

    /** 只在显式隔离测试模式启动，不构成产品业务服务
    @param [in] pid 拥有本机注册入口的网关PID
    @param [in] stopFile 测试宿主独占停止路径
    @param [in] duration 1至MaximumDurationMs毫秒，仅测试进程使用
    @param [in] product 本测试服务注册的产品号
    @param [in] internalTest 是否验证两轮内部依赖会话
    @return 0正常，非零失败
    */
    static int run(int32_t pid, const std::string& stopFile, uint32_t duration, uint32_t product = 1, bool internalTest = false);

private:
    /** 记录实际会话就绪及本用例的握手约束
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] maxRequests 已协商的最大在途请求数
    @param [in] maxChunk 已协商的最大文件分块字节数
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 核对本轮请求号、业务状态和实际返回正文
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] requestId 本轮实际请求号，必须与已提交请求匹配
    @param [in] product 协议产品号，按本用例独立预期核对
    @param [in] type 业务消息类型，按本用例独立预期核对
    @param [in] status RPC终态状态，不等同于底层发送返回值
    @param [in] bytes 回调或同步调用期间借用的正文；不保存裸指针
    @param [in] size 正文字节数，不能用C字符串长度代替
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product, uint32_t type,
        uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 核对本用例的非请求帧，异常输入记录为失败
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] frame 本次调用期间借用的完整帧，背压时由调用方保留
    @return true已消费；测试失败另行记录
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 记录本轮会话终态，供外层推进及结果断言使用
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] reason 本连接或服务的NI关闭原因
    @param [in] detail 底层诊断码，不替代NI关闭原因
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 推进两轮独立内部业务TLS，验证同一服务管理连接不会被出站关闭破坏
    */
    void pollInternal();

    // 本用例服务接入对象，生命周期覆盖服务回调与协议推进
    NetSenderServiceGateway* m_service;
    // 本轮内部依赖的独占出站管理适配，晚于会话销毁
    std::unique_ptr<NetSenderServiceOutgoingTransport> m_transport;
    // 本轮内部业务TLS的独占通道，借用m_transport
    std::unique_ptr<NetGatewayClientChannel> m_channel;
    // 本轮内部依赖请求会话，先于通道和运输释放
    std::unique_ptr<NetSession> m_session;
    // 本用例拥有的独立预期正文，用于逐字节核对传输结果
    std::vector<uint8_t> m_payload;
    // 人为拒收的逻辑通道集合，关闭时移除并记录原因
    std::set<uint64_t> m_stalled;
    // 本轮实际分配的请求号，回包必须精确匹配
    uint64_t m_request;
    // 本用例固定的服务产品号，不从回包中反推期望值
    uint32_t m_product;
    // 已完成内部依赖往返轮数，本测试目标为2轮
    uint32_t m_round;
    // 是否启用真实内部服务出站往返验证
    bool m_internalTest;
    // 当前内部业务TLS及能力握手已就绪
    bool m_sessionReady;
    // 已向实际接入层提交的发送观察状态，不能视作远端收到
    bool m_sent;
    // 本用例工作已完成，拥有者仍须收敛运输或回收任务
    bool m_done;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    bool m_ready;
    // 本用例首次失败后保留的错误标志，后续成功不能清除
    bool m_failed;
};