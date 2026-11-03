#pragma once
#include "NetWork.h"
#include <NetPeer/NetPeerAPI.h>
#include <CTaskThreadManager/CTaskThreadManagerAPI.h>
#include <atomic>
#include <mutex>
#include <map>
#include <vector>

// 隔离监听器的服务消息副本，全部属于测试main执行者
struct NetWorkThreadTestMessage
{
    // 真实路由代次
    uint64_t m_endpoint;
    // 原帧及独占消息体
    NetInterfaceFrame m_frame;
    // 本用例拥有的消息字节副本，避免在回调外保留失效借用
    std::vector<uint8_t> m_bytes;
};

// 仅模拟业务处理器，不实现TLS、RPC或路由
class NetWorkThreadTestSink : public NetEndpointServiceSink
{
public:
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
    std::vector<NetWorkThreadTestMessage> m_messages;
};

// 多生产者及回调的有界观察状态，不持锁进行网络或任务等待
struct NetWorkThreadTestContext
{
public:
    // main回收，生产者只借用
    NetWorkClientHandle m_client;
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
    // 只保护测试的有界编号集合
    std::mutex m_mutex;
    // 本用例成功接纳的请求号集合，用于与实际终态逐项对照
    std::vector<uint64_t> m_accepted;
    // 请求号到实际终态次数的映射，用于发现漏回调和重复回调
    std::map<uint64_t, uint32_t> m_terminal;

public:
    /** 清零独占句柄及观察计数
    */
    NetWorkThreadTestContext();

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

    /** 记录或核对本轮唯一终态及其线程归属
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] generation 当前会话代次，用于核对陈旧回调不会进入新会话
    @param [in] reason 本连接或服务的NI关闭原因
    @param [in] detail 底层诊断码，不替代NI关闭原因
    */
    static void closed(void* context, uint64_t generation, int32_t reason, int32_t detail);
};

// 所有生产者也使用CTask，不私建系统线程
class NetWorkThreadTestProducer : public CTask
{
public:
    /** 借用本轮观察状态至任务退出
    @param [in] context 生命期main保留
    */
    explicit NetWorkThreadTestProducer(NetWorkThreadTestContext* context);

    /** 独立提交8次，每三次取消一次；不等网络
    */
    virtual void DoTask();
private:
    // main先回收生产者再释放
    NetWorkThreadTestContext* m_context;
};