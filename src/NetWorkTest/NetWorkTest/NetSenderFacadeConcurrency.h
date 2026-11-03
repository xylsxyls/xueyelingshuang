#pragma once
#include "NetWork/NetWorkAPI.h"
#include "NetPeer/NetPeerAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <mutex>
#include <map>
#include <vector>

// 隔离监听器的服务消息副本，全部属于测试main执行者
struct NetSenderFacadeConcurrencyMessage
{
    // 真实路由代次
    uint64_t m_endpoint;
    // 原帧及独占消息体
    NetInterfaceFrame m_frame;
    // 本用例拥有的消息字节副本，避免在回调外保留失效借用
    std::vector<uint8_t> m_bytes;
};

// 仅模拟业务处理器，不实现TLS、RPC或路由
class NetSenderFacadeConcurrencySink : public NetEndpointServiceSink
{
public:
    /** 默认只验证原有回显；保序模式另核对每个生产者序列
    */
    NetSenderFacadeConcurrencySink();

    /** 复制借用帧，返回真实接纳结果
    @param [in] endpoint 当前逻辑端点号
    @param [in] frame 本次调用期间借用的完整帧
    @param [in] origin 本次调用期间借用的可信来源
    @return NI_OK已复制测试帧，异常交由测试运行器记录
    */
    virtual int32_t send(uint64_t endpoint, const NetInterfaceFrame& frame, const NetEndpointOrigin& origin);

    /** 此测试没有业务进程可停止
    @param [in] endpoint 当前逻辑端点号
    */
    virtual void close(uint64_t endpoint);

    /** 在main上处理并回送全部已投递请求
    @param [in,out] router 真实Peer路由器
    @return 是否所有帧合法且接纳成功
    */
    bool respond(NetPeerEndpointRouter& router);
public:
    // 当前main拥有的消息
    std::vector<NetSenderFacadeConcurrencyMessage> m_messages;
    // 只由用例owner配置／访问，接收顺序直接取自真实路由投递。
    bool m_verifyOrder;
    // 四个生产者各自下一条预期到达序号
    uint32_t m_next[4];
};

// C消费句柄的公开不透明类型，布局由提供方隐藏
struct NetWorkSenderClientOpaque;

// 多生产者及回调的有界观察状态，不持锁进行网络或任务等待
struct NetSenderFacadeConcurrencyContext
{
public:
    // main回收，生产者只借用
    NetWorkSenderClientOpaque* m_client;
    // 测试main系统线程
    uint32_t m_mainThread;
    // 所有回调必须来自本代同一owner
    std::atomic<uint32_t> m_callbackThread;
    // 最近实际握手代次
    std::atomic<uint64_t> m_generation;
    // 并发观察
    std::atomic<uint32_t> m_ready;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    std::atomic<uint32_t> m_closed;
    // 本用例工作已完成，拥有者仍须收敛运输或回收任务
    std::atomic<uint32_t> m_done;
    // 实际收到的请求终态次数，用于检查一请求一终态
    std::atomic<uint32_t> m_results;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    std::atomic<bool> m_bad;
    // 在ready回调抛受控异常；只在连接前设置，验证协议线程能正常收敛
    bool m_throwReady;
    // 启动前固定，四个生产者各发送带独立递增序号的正文。
    bool m_verifyOrder;
    // 只保护测试的有界编号集合
    std::mutex m_mutex;
    // 同步send真实调用耗时，单位微秒，仅在生产者回收后统计，不作为时延硬阈值。
    std::vector<uint32_t> m_sendMicros;
    // 本用例成功接纳的请求号集合，用于与实际终态逐项对照
    std::vector<uint64_t> m_accepted;
    // 请求号到实际终态次数的映射，用于发现漏回调和重复回调
    std::map<uint64_t, uint32_t> m_terminal;

public:
    /** 清零独占句柄及观察计数
    */
    NetSenderFacadeConcurrencyContext();

    /** 记录真实握手、回调线程并核对禁止回调中销毁／重入
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] requests 本轮协商后的最大在途请求数
    @param [in] chunk 本轮协商后的最大块字节数
    */
    static void ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 核对本轮请求的实际终态、正文和回调约束
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] id 本轮实际请求号，必须与已提交请求匹配
    @param [in] product 协议产品号，按本用例独立预期核对
    @param [in] type 业务消息类型，按本用例独立预期核对
    @param [in] status RPC终态状态，不等同于底层发送返回值
    @param [in] bytes 回调或同步调用期间借用的正文；不保存裸指针
    @param [in] size 正文字节数，不能用C字符串长度代替
    */
    static void result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 记录本用例非预期的独立帧回调
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] frame 本次调用期间借用的完整帧，背压时由调用方保留
    @return 非零表示此帧已消费，避免失败观察再次触发回调
    */
    static int32_t frame(void* context, uint64_t generation, const NetInterfaceFrame* frame);

    /** 核对本轮会话的正常终态与协议线程归属
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] reason 本连接或服务的NI关闭原因
    @param [in] detail 底层诊断码，不替代NI关闭原因
    */
    static void closed(void* context, uint64_t generation, int32_t reason, int32_t detail);
};

// 所有生产者也使用CTask，不私建系统线程
class NetSenderFacadeConcurrencyProducer : public CTask
{
public:
    /** 借用本轮观察状态至任务退出
    @param [in] context 生命期main保留
    @param [in] producer 本轮生产者编号，保序测试为0至3
    */
    explicit NetSenderFacadeConcurrencyProducer(NetSenderFacadeConcurrencyContext* context, uint32_t producer = 0);

    /** 独立提交8次send／post混合请求，每三次取消一次；同步交付不持测试锁
    */
    virtual void DoTask();
private:
    // main先回收生产者再释放
    NetSenderFacadeConcurrencyContext* m_context;
    // 当前CTask固定的生产者下标0至3，不使用系统线程ID作为协议字段。
    uint32_t m_producer;
};
// 本用例独占的生命期owner，异常时先停止生产者、收敛网络再回收句柄。
class NetSenderFacadeConcurrencyOwner
{
public:
    /** 借用比本owner更早创建的观察状态，生产者和回调不得越过其生命期
    @param [in,out] context 本用例观察状态
    */
    explicit NetSenderFacadeConcurrencyOwner(NetSenderFacadeConcurrencyContext& context);

    /** 在runner执行者回收本用例独占的资源，不操作其他线程ID。
    */
    ~NetSenderFacadeConcurrencyOwner();
    // 最多4个独占生产者，每轮结束后清零，失败时由析构回收。
    uint32_t m_producers[4];
private:
    /** 禁止复制独占资源
    @param [in] other 不可复制
    */
    NetSenderFacadeConcurrencyOwner(const NetSenderFacadeConcurrencyOwner& other);

    /** 禁止赋值独占资源
    @param [in] other 不可复制
    @return 不可调用
    */
    NetSenderFacadeConcurrencyOwner& operator=(const NetSenderFacadeConcurrencyOwner& other);
    // 外部观察状态仍然存活，供关闭时的最终回调使用。
    NetSenderFacadeConcurrencyContext* m_context;
};

/** 独立编译本提供方的接口回归，避免同名C/C++门面混用
*/
class NetSenderFacadeConcurrency
{
public:
    /** 共同C接口经C++门面验证真实TLS、多生产者send/post保序及回调重入拒绝
    */
    static void workConcurrency();
};