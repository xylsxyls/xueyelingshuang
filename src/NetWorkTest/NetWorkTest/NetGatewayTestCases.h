#pragma once
#include "NetServerManagerTestIncludes.h"
#include "NetInterfaceTransportProbe.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <memory>
#include <vector>
#include <set>

// 服务投递快照，保留真实目标、来源和独占正文
struct NetGatewayTestDelivery
{
    // 实际路由或服务端点编号，用于核对投递目标
    uint64_t m_endpoint;
    // 本次投递的可信根来源副本，用于核对来源隔离
    NetGatewayOrigin m_origin;
    // 保存实际投递帧及正文的独占副本
    NetInterfaceTransportMessage m_message;
};

// 仅测试中的受控本地投递边界；不充当Account实现或IPC认证证据。
class NetGatewayTestSink : public NetGatewayServiceSink
{
public:
    /** 初始化可控背压
    */
    NetGatewayTestSink();

    /** 保存借用帧的完整副本
    @param [in] endpoint 服务代次
    @param [in] frame 消息
    @param [in] origin 真实根上下文
    @return 受控结果
    */
    virtual int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetGatewayOrigin& origin);

    /** 记录失效代次
    @param [in] endpoint 代次
    */
    virtual void close(uint64_t endpoint);

public:
    // 受控服务背压开关，拒绝时不得消费或复制输入
    bool m_busy;
    // 服务边界实际接纳的投递序列
    std::vector<NetGatewayTestDelivery> m_deliveries;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    std::set<uint64_t> m_closed;
};

// 通过真实TLS和应用握手验证网关路由的可控客户端
class NetGatewayTestClient : public NetInterfaceTransportProbe
{
public:
    /** 创建带可控应用限额的测试客户端
    @param [in] limit 1至128
    */
    explicit NetGatewayTestClient(uint32_t limit = 32);

    /** TLS完成后创建真实C++应用握手
    @param [in] peer TCP连接号
    */
    virtual void onReady(uint64_t peer);

    /** 校验握手或记录业务响应
    @param [in] peer TCP连接号
    @param [in] frame 借用帧
    @return 已消费
    */
    virtual bool onFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 驱动传输和应用HELLO
    */
    void poll();

public:
    // 本测试客户端实际使用的TLS帧运输
    NetTransport m_network;
    // 本次独占的业务能力握手状态
    std::unique_ptr<NetInterfaceHandshake> m_handshake;
    // 当前用例使用的真实运输代次，不以PID替代
    uint64_t m_peer;
    // 本测试客户端显式请求的并行额度
    uint32_t m_limit;
    // 暂停提交应用HELLO，用于打乱TLS与应用握手完成顺序
    bool m_holdHello;
    // 实际完成应用能力握手，不能由TCP连接成功代替
    bool m_applicationReady;
};

// 网关路由、业务会话和文件测试集合，逐用例独立建立预期并检查实际结果
class NetGatewayTestCases
{
public:
    /** 真实TCP加两级路由，验证根上下文及独立A/B原号
    */
    static void nestedRoute();

    /** 应用握手乱序完成、协商额度、忙及取消转发
    */
    static void budgetsAndHandshakeOrder();

    /** 服务断开/换代、未授权回复及未知产品
    */
    static void serviceGeneration();

    /** 公开原生客户端C ABI基础往返、取消、超时、断线与重连
    */
    static void clientSession();

    /** 单执行者NetSender的post延后与副本、send交付边界及DATA真实往返
    */
    static void senderDelivery();

    /** 同一Peer执行者混合提交RPC／授权DATA／RPC，验证服务端接收顺序
    */
    static void senderMixedOrder();

    /** 先预留低号但先提交高号，按实际提交先后发送并保留原业务号／期限
    */
    static void senderRequestOrder();

    /** 真实TCP/TLS下六种低层监听者异常不逃逸，取消／超时／关闭仍可解绑重用
    */
    static void lowSessionCallbackFailures();

    /** 真实TLS中区分本地关闭与远端失联，已发／未发请求各有一致唯一终态
    */
    static void lowSessionCloseStatus();

    /** 超时回调预留新请求，原执行轮次不得倒退请求表时钟或丢失终态
    */
    static void sessionCallbackDeadline();

    /** 基础文件路由：流号映射、双向块及授权分组撤销，不替代实际文件I/O测试
    */
    static void fileRouting();

private:
    /** 共用真实发送／文件夹具，按选择验证基础交付或混合业务接收顺序
    @param [in] orderedMix 是否检查RPC、授权DATA和后续RPC的提交顺序
    */
    static void runSenderDelivery(bool orderedMix);
};

// 原生C会话回调观察者，独立核对编号、正文和终态次数
class NetGatewaySessionProbe
{
public:
    /** 安全初始化事件接收者
    */
    NetGatewaySessionProbe();

    /** 兜底销毁本测试客户端，回调接收者仍存活
    */
    ~NetGatewaySessionProbe();

    /** 本对象的v1回调表
    @return 调用时复制的表
    */
    NetWorkClientCallbacks callbacks();

    /** 接纳就绪代次
    @param [in] context 本对象
    @param [in] generation 代次
    @param [in] maxRequests 在途上限
    @param [in] maxChunk 块大小
    */
    static void Ready(void* context, uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 拷贝借用的完成数据
    @param [in] context 本对象
    @param [in] generation 代次
    @param [in] id 原请求
    @param [in] product 产品
    @param [in] type 命令
    @param [in] status RPC结果
    @param [in] bytes 消息体
    @param [in] size 长度
    */
    static void Result(void* context, uint64_t generation, uint64_t id, uint32_t product,
        uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 不接收额外业务推送
    @param [in] context 本对象
    @param [in] generation 代次
    @param [in] frame 推送
    @return 1消费并记录不符合预期
    */
    static int32_t Frame(void* context, uint64_t generation, const NetInterfaceFrame* frame);

    /** 记录关闭唯一性
    @param [in] context 本对象
    @param [in] generation 代次
    @param [in] reason 原因
    @param [in] detail 诊断
    */
    static void Closed(void* context, uint64_t generation, int32_t reason, int32_t detail);

public:
    // 本用例拥有的Peer C客户端，析构前收敛
    NetPeerClientHandle m_client;
    // 本用例记录的实际就绪代次，用于拒绝旧代回调
    uint64_t m_generation;
    // 实际关闭通知次数，用于检查唯一终态
    uint32_t m_closes;
    // 实际请求结果回调次数，用于检查唯一终态
    uint32_t m_completions;
    // 最近实际结果携带的原始业务请求号
    uint64_t m_lastId;
    // 最近一次实际RPC结果码
    uint32_t m_status;
    // 回调借用正文的独占副本，用于回调外逐字节验证
    std::vector<uint8_t> m_body;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    bool m_bad;
    // 受控超时回调只预留一次新的独立请求
    bool m_reissueOnTimeout;
    // 回调中创建的新请求及接纳结果
    uint64_t m_reissuedId;
    // 超时回调内重新预留请求的实际返回码
    int32_t m_reissueResult;
};

// 仅观察低层会话回调；受控抛出验证产品catch，不复制协议或TLS。
class NetGatewaySessionFaultProbe : public NetSessionListener
{
public:
    /** 选择故障入口：0就绪、1回应、2事件、3取消、4超时、5关闭及其未完成请求
    @param [in] mode 本轮固定故障方式
    */
    explicit NetGatewaySessionFaultProbe(uint32_t mode);

    /** 记录真实就绪并在mode0抛出受控测试异常
    @param [in] generation 当前代次
    @param [in] requests 实际协商请求上限
    @param [in] chunk 实际协商块长
    */
    virtual void onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 记录唯一请求终态；mode1／3／4／5第一次回调抛受控异常
    @param [in] generation 当前代次
    @param [in] id 本轮原号
    @param [in] product 固定4
    @param [in] type 固定700
    @param [in] status 真实协议结果
    @param [in] bytes 成功时借用单字节7
    @param [in] size 成功1，失败0
    */
    virtual void onSessionResult(uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 记录经真实网关授权转发的定向事件，mode2抛出受控异常
    @param [in] generation 当前代次
    @param [in] frame 单字节7的事件
    @return true已消费
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 记录唯一关闭；mode5抛出受控异常
    @param [in] generation 当前代次
    @param [in] reason 原关闭原因
    @param [in] detail 仅诊断
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);
public:
    // 本轮生命期owner；回调只探测禁止的重入poll，不释放会话。
    NetSession* m_session;
    // 构造后不再修改的故障入口。
    uint32_t m_mode;
    // 实际各类回调计数。
    uint32_t m_ready;
    // 实际收到的请求终态次数，用于检查一请求一终态
    uint32_t m_results;
    // 实际业务帧回调次数，用于检查异常隔离
    uint32_t m_frames;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    uint32_t m_closed;
    // 以原号独立检测重复终态。
    std::set<uint64_t> m_ids;
    // 输入损坏、重复回调或回调内poll未被拒绝。
    bool m_bad;
};