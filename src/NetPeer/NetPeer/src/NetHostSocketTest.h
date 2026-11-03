#pragma once
#include "NetHostSocket.h"
#include <vector>

#ifdef NET_HOST_TEST_LIBUV
extern "C"
{
    /** 纯C编译单元直接把Libuv函数填入宿主表，检验签名无需强转
    @param [out] functions 完整v1函数表
    */
    void netHostSocketTestLibuvFunctions(NetHostFunctions* functions);
}
#endif

// 确定性宿主替身，只产生原始事件；不复制被测队列或关闭算法
class NetHostSocketTestDriver
{
public:
    /** 初始化原始宿主可观察状态
    */
    NetHostSocketTestDriver();

    /** 返回指向本测试宿主的v1表
    @return 完整函数表，context借用本对象
    */
    NetHostFunctions functions();

    /** 保存独占回调入口
    @param [in] providerContext 测试宿主
    @param [in] observerContext 被测组合状态
    @param [in] connected 连接入口
    @param [in] data 输入入口
    @param [in] writable 可写入口
    @param [in] closed 终态入口
    @param [out] instance 测试宿主本身
    @return 0或指定失败
    */
    static int32_t create(void* providerContext, void* observerContext, NetHostConnected connected, NetHostData data,
        NetHostWritable writable, NetHostClosed closed, void** instance);

    /** 接纳测试监听
    @param [in] instance 测试宿主
    @param [in] address 地址
    @param [in] port 端口
    @param [in] maxPeers 容量
    @return 0
    */
    static int32_t listen(void* instance, const char* address, uint32_t port, uint32_t maxPeers);

    /** 接纳测试连接，可选择在返回前触发连接事件
    @param [in] instance 测试宿主
    @param [in] address 地址
    @param [in] port 端口
    @param [out] peer 固定1
    @return 0
    */
    static int32_t connect(void* instance, const char* address, uint32_t port, uint64_t* peer);

    /** 返回可控活动标记，不产生协议逻辑
    @param [in] instance 测试宿主
    @return 0/1
    */
    static int32_t poll(void* instance);

    /** 记录接纳字节数，不修改输入
    @param [in] instance 测试宿主
    @param [in] peer 代次
    @param [in] data 借用字节
    @param [in] size 长度
    @return 可控接纳/背压/失败值
    */
    static int32_t write(void* instance, uint64_t peer, const uint8_t* data, uint32_t size);

    /** 记录暂停/恢复调用
    @param [in] instance 测试宿主
    @param [in] peer 代次
    @param [in] paused 暂停标记
    @return 0或可控失败
    */
    static int32_t pauseRead(void* instance, uint64_t peer, int32_t paused);

    /** 同步通知一次连接终态
    @param [in] instance 测试宿主
    @param [in] peer 代次
    @param [in] reason 原因
    @return 0
    */
    static int32_t close(void* instance, uint64_t peer, int32_t reason);

    /** 仅停止底层活动，故意不给终态以验证组合层结算
    @param [in] instance 测试宿主
    @return 0
    */
    static int32_t shutdown(void* instance);

    /** 记录唯一释放
    @param [in] instance 测试宿主
    @return 0
    */
    static int32_t destroy(void* instance);

    /** 提供固定隔离监听端口
    @param [in] instance 测试宿主
    @return 固定端口
    */
    static uint32_t localPort(void* instance);

    /** 返回可观察活动数量
    @param [in] instance 测试宿主
    @return 0/1
    */
    static uint32_t peerCount(void* instance);

    /** 测试专用确定性熵，只用于宿主契约测试，禁止用于真实安全会话
    @param [in] context 测试提供方
    @param [out] output size字节有效缓冲
    @param [in] size 长度
    @return 0完成，按用例失败或抛异常
    */
    static int32_t entropy(void* context, uint8_t* output, uint32_t size);

public:
    // 被测层回调上下文
    void* m_observer;
    // 原始连接事件
    NetHostConnected m_connected;
    // 原始输入事件
    NetHostData m_data;
    // 原始可写事件
    NetHostWritable m_writable;
    // 原始关闭事件
    NetHostClosed m_closed;
    // 每次write的指定返回值
    int32_t m_writeResult;
    // 每次pauseRead的指定返回值
    int32_t m_pauseResult;
    // 熵行为：0测试成功、1部分写后失败、2部分写后异常
    uint32_t m_entropyMode;
    // 还需拒绝的destroy次数，验证所有权不会提前释放
    uint32_t m_destroyFailures;
    // 宿主活动标记
    bool m_active;
    // 是否在connect同步触发事件
    bool m_syncConnect;
    // 是否create失败
    bool m_createFail;
    // 宿主抛异常位：create/listen/connect/poll/write/pause/close/shutdown/destroy/query。
    uint32_t m_fault;
    // 暂停时同步EOF，或恢复时同步交付指定次数的新输入。
    bool m_closeOnPause;
    // 恢复读取时同步产生的受控输入量，用于验证重入借用边界
    uint32_t m_dataOnResume;
    // 成功接纳的总字节
    uint32_t m_written;
    // pauseRead调用数
    uint32_t m_pauses;
    // stop调用数
    uint32_t m_stops;
    // destroy调用数
    uint32_t m_destroys;
};

// 仅记录上层公开事件和输入副本，不实现宿主或协议
class NetHostSocketTestListener : public NetHostSocketListener
{
public:
    /** 初始化空观测状态
    */
    NetHostSocketTestListener();

    /** 记录事件并按用例选择重入/停止/异常
    @param [in] peer 代次
    */
    virtual void onConnected(uint64_t peer);

    /** 复制输入观测值
    @param [in] peer 代次
    @param [in] data 借用输入
    @param [in] size 长度
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size);

    /** 记录合并后的可写事件
    @param [in] peer 代次
    */
    virtual void onWritable(uint64_t peer);

    /** 记录终态及原因
    @param [in] peer 代次
    @param [in] reason 终态原因
    @param [in] detail 原始宿主诊断
    */
    virtual void onClosed(uint64_t peer, int32_t reason, int32_t detail);

public:
    // 不拥有被测组合
    NetHostSocket* m_socket;
    // 已交付连接数
    uint32_t m_connected;
    // 已交付输入块数
    uint32_t m_data;
    // 已交付可写数
    uint32_t m_writable;
    // 已交付终态数
    uint32_t m_closed;
    // 最近终态原因
    int32_t m_reason;
    // 收到的原始宿主诊断
    int32_t m_detail;
    // 在连接事件请求停止
    bool m_stopOnConnect;
    // 在连接事件模拟上层异常
    bool m_throwOnConnect;
    // 在输入回调恢复读取，验证同步新输入不覆盖仍被借用的旧数据。
    bool m_resumeOnData;
    // 输入观察回调抛出测试异常的开关，不进入生产宿主
    bool m_throwOnData;
    // 需要时记录连接/数据/可写/终态的先后；普通吞吐测试不记录。
    bool m_trace;
    // 本用例保留的事件序列，按实际回调发生顺序检查
    std::vector<uint32_t> m_events;
    // 输入是否始终为合法代次和范围
    bool m_valid;
    // 上层实际收到的输入副本
    std::vector<uint8_t> m_bytes;
};