#pragma once
#include "NetWork/NetWorkAPI.h"
#include <atomic>
#include <vector>
#include "Semaphore/SemaphoreAPI.h"
#include <memory>

class NetGatewayPerformanceTest;

// 测试侧观测宿主，原回调和上下文借用至对应运输销毁，统计不改变字节或推进方式。
struct NetGatewayPerformanceObserver
{
public:
    // 原始回调上下文
    void* m_context;
    // 原始连接事件
    NetHostConnected m_connected;
    // 原始收包事件
    NetHostData m_data;
    // 原始可写事件
    NetHostWritable m_writable;
    // 原始关闭事件
    NetHostClosed m_closed;
    // 统计拥有者，晚于所有宿主销毁
    NetGatewayPerformanceTest* m_owner;
};

// 一个生产者独占槽；发布编号前填好时间，回调清零后才能复用
struct NetGatewayPerformanceSlot
{
public:
    // 非零编号以release发布时间，以acquire取得回调归属
    std::atomic<uint64_t> m_request;
    // 对应业务调用开始时的单调微秒数
    uint64_t m_started;
    // 本槽已验证的终态数量，仅回调写，批次收敛后读取
    uint32_t m_received;

public:
    /** 初始化空闲槽
    */
    NetGatewayPerformanceSlot();
};

// 仅测试使用的真实NTW3客户端；同份源码分别链接隔离基准与候选SDK
class NetGatewayPerformanceTest : public NetSessionListener
{
public:
    /** 初始化观测状态，不启动线程或连接
    */
    NetGatewayPerformanceTest();

    /** 执行有界跨进程收发对比；正文完整校验，统计不包含连接准备
    @param [in] argc RPC共6项，单向共7项，均包含程序名
    @param [in] argv port、正文长度、总数、生产线程数(1/4)、send/post或data-send/data-post；单向追加停止文件
    @return 0全部校验及清理成功，非零本轮不可用于性能结论
    */
    static int run(int argc, char** argv);

    /** 记录真实握手就绪，不在回调中发送
    @param [in] generation 当前代次
    @param [in] maxRequests 对端接纳窗口
    @param [in] maxChunk 对端块上限
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 校验回显与唯一终态，发布槽归还
    @param [in] generation 当前代次
    @param [in] requestId 原请求号
    @param [in] product 服务号
    @param [in] type 消息类型
    @param [in] status RPC状态
    @param [in] bytes 借用正文
    @param [in] size 正文字节数
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
        uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 本RPC测试不接收数据流帧
    @param [in] generation 代次
    @param [in] frame 借用帧
    @return true已消费并记失败
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 区分主动收敛与收发过程中断连
    @param [in] generation 代次
    @param [in] reason 原因
    @param [in] detail 底层明细
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

private:
    /** 建立只读观测回调并透传实际LibuvTcp创建
    @param [in] providerContext 本次性能测试
    @param [in] observerContext 原宿主观察上下文
    @param [in] connected 原连接回调
    @param [in] data 原收包回调
    @param [in] writable 原可写回调
    @param [in] closed 原关闭回调
    @param [out] instance 原LibuvTcp实例
    @return LibuvTcp创建结果，分配失败返回负数
    */
    static int32_t createHost(void* providerContext, void* observerContext, NetHostConnected connected,
        NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance);

    /** 透传实际网络提交，仅累计成功接纳的密文字节
    @param [in] instance 原宿主实例
    @param [in] peer 原连接号
    @param [in] bytes 原字节
    @param [in] size 字节数
    @return 原LibuvTcp结果
    */
    static int32_t writeHost(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 转发连接通知
    @param [in] context 观测包装
    @param [in] peer 原连接号
    */
    static void hostConnected(void* context, uint64_t peer);

    /** 累计实际TCP交付字节并转发
    @param [in] context 观测包装
    @param [in] peer 原连接号
    @param [in] bytes 本次借用输入
    @param [in] size 字节数
    */
    static void hostData(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 转发可写通知
    @param [in] context 观测包装
    @param [in] peer 原连接号
    */
    static void hostWritable(void* context, uint64_t peer);

    /** 转发关闭通知
    @param [in] context 观测包装
    @param [in] peer 原连接号
    @param [in] reason 原关闭原因
    */
    static void hostClosed(void* context, uint64_t peer, int32_t reason);

    /** 单向连续提交，不分配RPC槽、不等待业务回包；等待宿主确认接收端统计后关闭
    @param [in] count 总帧数，至少8
    @param [in] producers 生产者1或4
    @param [in] synchronous 是否使用同步交付接口
    @param [in] stopFile 宿主收到最后一条解密统计后创建的停止文件
    @return 本地提交及受控收敛是否成功，远端统计由宿主另行校验
    */
    bool oneWay(uint32_t count, uint32_t producers, bool synchronous, const char* stopFile);

    /** 逐条提交正文，生产者序号放在帧头，保持六字节正文不变
    @param [in] producer 生产者下标
    @param [in] count 本线程帧数
    @param [in] synchronous 是否同步交付
    @param [in] deadline 单调微秒截止时间，仅防止测试永久挂起
    */
    void produceData(uint32_t producer, uint32_t count, bool synchronous, uint64_t deadline);

    /** 在已就绪连接执行固定数量请求
    @param [in] count 请求总数，至少8
    @param [in] producers 生产者1或4，使用显式配置并由握手确认的资源上限
    @param [in] synchronous 是否调用send
    @param [in] report 是否输出性能统计
    @return 本批次数量、数据和终态全部正确
    */
    bool batch(uint32_t count, uint32_t producers, bool synchronous, bool report);

    /** 独占本生产者槽提交；竞争或背压不重新生成业务消息
    @param [in] producer 生产者下标
    @param [in] producers 线程总数
    @param [in] count 本线程请求总数
    @param [in] synchronous 是否同步交付
    @param [in] deadline 单调微秒截止时间
    */
    void produce(uint32_t producer, uint32_t producers, uint32_t count, bool synchronous, uint64_t deadline);

private:
    // 单进程只运行一个性能用例，执行者创建前设置、全部收敛后清除
    static NetGatewayPerformanceTest* s_active;
    // 实际网络库接受的TCP出站字节，含协议密文、无IP/TCP头
    std::atomic<uint64_t> m_wireSent;
    // 宿主观察包装，执行者收敛后随测试销毁
    std::vector<std::unique_ptr<NetGatewayPerformanceObserver>> m_observers;
    // 实际TCP入站字节，包含TLS和网关控制帧，不包含IP/TCP头
    std::atomic<uint64_t> m_wireReceived;
    // 生命周期owner持有，生产者与回调收敛后释放
    NetWorkSenderState* m_client;
    // 性能运行已完成握手的独立会话，生产者按编号选择；测量期间只读
    std::vector<NetWorkSenderState*> m_senders;
    // 所有请求使用同一不可变正文
    std::vector<uint8_t> m_body;
    // 启动前配置并由握手协商，不把测试窗口写死为8
    uint32_t m_window;
    // 按配置上限分配，各生产者只访问自己分片
    std::unique_ptr<NetGatewayPerformanceSlot[]> m_slots;
    // 四个生产者和一个收敛观察者的积存通知，避免非积存事件的登记竞态
    Semaphore m_progress[5];
    // 回调唯一写入，完成计数release后由owner读取
    std::vector<uint64_t> m_latency;
    // 当前握手代次
    std::atomic<uint64_t> m_generation;
    // 本批次已验证响应数
    std::atomic<uint32_t> m_completed;
    // 首次协议或提交错误，不把失败计为吞吐
    std::atomic<int32_t> m_failure;
    // 是否开始正常主动关闭
    std::atomic<bool> m_stopping;
    // 单向提交成功数；不等同于服务端已收到
    std::atomic<uint32_t> m_submitted;
    // 各生产者独占记录容量背压次数，join后汇总
    uint64_t m_backpressure[4];
};