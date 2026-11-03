#pragma once
#include "NetPeer.h"
#include <vector>

// S1无网络／无文件线程的独立库回归；所有测试上下文由main持有
class NetPeerTest : public NetSessionListener
{
public:
    /** 清零测试观察
    */
    NetPeerTest();

    /** 记录就绪；本组不建立真实网络
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] requests 本轮协商后的最大在途请求数
    @param [in] chunk 本轮协商后的最大块字节数
    */
    virtual void onSessionReady(uint64_t generation, uint32_t requests, uint32_t chunk);

    /** 记录结果；本组尚无已接纳业务
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] id 实际请求号，必须对应本轮已接纳的请求
    @param [in] product 实际产品号
    @param [in] type 实际业务类型
    @param [in] status RPC终态状态
    @param [in] bytes 仅本次回调期间借用的正文，不保存裸指针
    @param [in] size 正文长度，单位字节
    */
    virtual void onSessionResult(uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** 接纳测试帧
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] frame 本次调用期间借用的完整帧
    @return true已消费，测试结果由独立观察状态记录
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 记录唯一关闭
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] reason NI关闭原因
    @param [in] detail 底层诊断码
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

    /** 仅供隔离测试使用的确定性随机
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [out] bytes 待填充的确定性测试字节，禁止用于正式密钥或随机数
    @param [in] size 输出缓冲的可写字节数
    @return 0填充完成，仅用于测试
    */
    static int32_t entropy(void* context, uint8_t* bytes, uint32_t size);

    /** 构造拒绝真实连接的完整宿主表
    */
    static NetHostFunctions functions();

    /** 拒绝创建，不获取任何系统资源
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [in] observer 借用的事件目标，在宿主释放前保持有效
    @param [in] connected 借用的连接完成回调
    @param [in] data 借用的输入回调
    @param [in] writable 借用的可写回调
    @param [in] closed 借用的终态回调
    @param [out] instance 模拟宿主句柄；拒绝创建时清空
    @return -1，本入口拒绝创建真实宿主
    */
    static int32_t create(void* context, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance);

    /** 拒绝连接并清理输出
    @param [in] instance 本测试模拟宿主，不代表真实网络资源
    @param [in] address 本测试不建立真实网络连接的地址参数
    @param [in] port 端口参数，本测试不监听或连接
    @param [out] peer 本测试拒绝连接时置0
    @return -1，本测试不实际建立连接
    */
    static int32_t connect(void* instance, const char* address, uint32_t port, uint64_t* peer);

    /** 无资源操作
    @param [in] instance 本测试模拟宿主，不代表真实网络资源
    @return 0，无资源需要推进
    */
    static int32_t operation(void* instance);

    /** 拒绝写入
    @param [in] instance 本测试模拟宿主，不代表真实网络资源
    @param [in] peer 当前运输连接号
    @param [in] bytes 仅本次回调期间借用的正文，不保存裸指针
    @param [in] size 正文长度，单位字节
    @return -1，本测试不接纳网络发送
    */
    static int32_t write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 无资源控制
    @param [in] instance 本测试模拟宿主，不代表真实网络资源
    @param [in] peer 当前运输连接号
    @param [in] value 暂停状态或关闭原因，本测试不操作系统资源
    @return 0，无实际网络资源
    */
    static int32_t control(void* instance, uint64_t peer, int32_t value);

    /** 查询空实例
    @param [in] instance 本测试模拟宿主，不代表真实网络资源
    @return 0，无监听端口或连接资源
    */
    static uint32_t query(void* instance);

    /** C就绪适配
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] requests 本轮协商后的最大在途请求数
    @param [in] chunk 本轮协商后的最大块字节数
    */
    static void ready(void* context, uint64_t generation, uint32_t requests, uint32_t chunk);

    /** C结果适配
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] id 实际请求号，必须对应本轮已接纳的请求
    @param [in] product 实际产品号
    @param [in] type 实际业务类型
    @param [in] status RPC终态状态
    @param [in] bytes 仅本次回调期间借用的正文，不保存裸指针
    @param [in] size 正文长度，单位字节
    */
    static void result(void* context, uint64_t generation, uint64_t id, uint32_t product, uint32_t type, uint32_t status, const uint8_t* bytes, uint32_t size);

    /** C帧适配
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] frame 本次调用期间借用的完整帧
    @return 非零已消费，零保留当前帧
    */
    static int32_t frame(void* context, uint64_t generation, const NetInterfaceFrame* frame);

    /** C关闭适配
    @param [in] context 本测试借用上下文，生命周期覆盖回调；随机填充入口不使用该值
    @param [in] generation 实际会话代次，由被测实现提供
    @param [in] reason NI关闭原因
    @param [in] detail 底层诊断码
    */
    static void closed(void* context, uint64_t generation, int32_t reason, int32_t detail);
public:
    /** 生成确定性无联网测试配置
    @return 完整v1参数
    */
    static NetPeerClientOptions options();

    /** 绑定本观察者的完整C回调表
    @return v1回调表
    */
    NetPeerClientCallbacks callbacks();

    /** 在C事件中检查非法重入未改变核心
    @param [in,out] test 当前观察者
    */
    static void inspectC(NetPeerTest& test);

    /** 在独立C++消费者的事件中检查同一重入契约
    @param [in,out] test 当前观察者
    */
    static void inspectCpp(NetPeerTest& test);
public:
    // 真实原始宿主create调用数
    static uint32_t m_created;
    // 实际回调计数；每轮必须只收到关闭
    uint32_t m_closed;
    // 不应发生的其他回调数
    uint32_t m_other;
    // 最近关闭代次
    uint64_t m_generation;
    // C回调重入目标，仅借用本测试对象。
    NetPeerClientHandle m_client;
    // 独立C++消费者的重入目标，NWT公共C组不使用。
    NetPeerSenderState* m_cpp;
    // 可选受控回调检查，避免测试夹具依赖单个NetSender提供方。
    void (*m_reenter)(NetPeerTest& test);
    // 注入ready/result/frame/closed四种用户异常。
    uint32_t m_throwEvents;
    // 记录回调内检查失败，不能让测试失败冒充预期业务异常。
    bool m_bad;
    // 业务原始请求号和正文首字节，独立核对映射与提交顺序。
    std::vector<uint64_t> m_resultIds;
    // 请求结果的独占副本，回调返回后仍可与预期字节比较
    std::vector<uint8_t> m_resultBytes;
};