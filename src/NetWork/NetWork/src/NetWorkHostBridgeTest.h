#pragma once
#include "NetWorkHostBridge.h"
#include <CTaskThreadManager/CTaskThreadManagerAPI.h>
#include <atomic>

// 纯事件假宿主，验证跨线程借用和暂停，不作为TCP互通证据
struct NetWorkHostBridgeTest
{
public:
    // bridge活到原始宿主destroy返回成功
    void* m_bridge;
    // 原始事件借用目标
    void* m_target;
    // 借用的原始连接回调，模拟宿主销毁后不再调用
    NetHostConnected m_connected;
    // 借用的原始输入回调，只在其上下文仍有效时调用
    NetHostData m_data;
    // 可写事件观察状态；宿主回调只在有效上下文中调用
    NetHostWritable m_writable;
    // 借用的原始终态回调，模拟宿主销毁后不再调用
    NetHostClosed m_closed;
    // 每次观察必须来自此系统线程
    uint32_t m_owner;
    // owner观察计数
    uint32_t m_connections;
    // 指定场景核对新连接通告的全局递增顺序，不能被数据轮询游标重排。
    uint64_t m_lastConnected;
    // 启用按宿主递增连接号检查连接通告顺序
    bool m_checkConnectionOrder;
    // 实际输入回调次数或各连接输入计数，用于核对事件没有丢失
    uint32_t m_inputs;
    // 实际派发给观察者的可写通知次数，不表示网络写入次数
    uint32_t m_writes;
    // 实际关闭通知次数，用于检查唯一终态
    uint32_t m_closes;
    // 实际调用原始宿主停止入口的次数，用于检查幂等
    uint32_t m_stops;
    // 原始connect分配计数，验证256上限在宿主调用前生效
    uint64_t m_allocated;
    // 前两连接的输入和关闭次数，用于隔离断言
    uint32_t m_peerInputs[2];
    // 前两个连接各自实际终态次数，验证故障隔离
    uint32_t m_peerCloses[2];
    // 最近一次关闭原因
    int32_t m_reason;
    // 自动在首块输入后暂停peer1
    bool m_pause;
    // 普通测试逐字节检查7；容量测试仅计数和检查线程
    bool m_checkPayload;
    // 以生产者编号和递增序号检查同线程先发先到
    bool m_sequence;
    // 四个生产者各自期待的序号
    uint32_t m_expected[4];
    // 四个任务完成计数，owner在收敛任务后读取结果
    std::atomic<uint32_t> m_producersDone;
    // 底层是否已停止
    bool m_stopped;
    // 在原始poll注入C++宿主异常，验证外层仍能回收。
    bool m_throwPoll;
    // 错误借用／线程／回调销毁
    bool m_bad;
    // 生产者实际结束
    std::atomic<bool> m_done;

public:
    /** 初始化借用回调及观察状态
    */
    NetWorkHostBridgeTest();

    /** 构造不分配系统资源的完整函数表
    */
    NetHostFunctions functions();

    /** 执行固定队列、容量、终态、并发和失败清理审计
    @return 0全部通过，非零检查失败
    */
    static int32_t audit();

    /** 保存原始宿主的回调表，返回本用例借用状态
    @param [in] provider 本测试的模拟宿主状态，在桥接器释放前保持有效
    @param [in] observer 借用的事件目标，在宿主释放前保持有效
    @param [in] connected 借用的连接完成回调
    @param [in] data 借用的输入回调
    @param [in] writable 借用的可写回调
    @param [in] closed 借用的终态回调
    @param [out] instance 模拟宿主句柄；拒绝创建时清空
    @return 0，返回本用例借用的模拟宿主
    */
    static int32_t create(void* provider, void* observer, NetHostConnected connected, NetHostData data, NetHostWritable writable, NetHostClosed closed, void** instance);

    /** 提供无需系统资源的成功操作入口
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @return 0接纳成功
    */
    static int32_t operation(void* instance);

    /** 返回模拟宿主活动状态；受控场景注入异常以验证隔离
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @return 1仍活动，0已停止；受控异常交由被测桥接器处理
    */
    static int32_t poll(void* instance);

    /** 记录模拟宿主停止请求，供外层核对资源收敛
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @return 0已接纳停止
    */
    static int32_t shutdown(void* instance);

    /** 通过已保存回调同步报告本次模拟关闭
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] reason 本连接或服务的NI关闭原因
    @return 0已提交模拟关闭通知
    */
    static int32_t close(void* instance, uint64_t peer, int32_t reason);

    /** 接纳模拟暂停请求，不建立后台网络或线程
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] paused 非零暂停、零恢复；此模拟入口仅接纳控制操作
    @return 0已接纳
    */
    static int32_t pause(void* instance, uint64_t peer, int32_t paused);

    /** 分配递增测试连接号，供外层验证接纳容量限制
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @param [in] address 本测试不实际建立网络的地址参数
    @param [in] port 本测试不实际建立网络的端口参数
    @param [out] peer 本次分配的模拟连接号
    @return 0成功，输出本轮分配的连接号
    */
    static int32_t connect(void* instance, const char* address, uint32_t port, uint64_t* peer);

    /** 接纳本次测试字节，不实际建立网络连接
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] bytes 回调或同步调用期间借用的正文；不保存裸指针
    @param [in] size 正文字节数，不能用C字符串长度代替
    @return 0接纳成功，未实际发送网络数据
    */
    static int32_t write(void* instance, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 报告模拟宿主没有监听端口或额外连接资源
    @param [in] instance 本次用例有效的模拟宿主，不由此入口释放观察状态
    @return 0，模拟宿主没有该类系统资源
    */
    static uint32_t query(void* instance);

    /** 检查连接通告中的真实编号和用例约束
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    */
    static void connected(void* context, uint64_t peer);

    /** 核对输入字节、连接顺序和终态前后事件边界
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] bytes 回调或同步调用期间借用的正文；不保存裸指针
    @param [in] size 正文字节数，不能用C字符串长度代替
    */
    static void data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 观察可写通知，不将其解释为业务消息已送达
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    */
    static void writable(void* context, uint64_t peer);

    /** 记录或核对本轮唯一终态及其线程归属
    @param [in] context 本次用例借用的观察状态，仅在所属执行者回调中访问
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] reason 本连接或服务的NI关闭原因
    */
    static void closed(void* context, uint64_t peer, int32_t reason);
};

// 从独立CTask产生原始事件的测试任务，拥有者回收后才释放观察状态
class NetWorkHostBridgeTestTask : public CTask
{
public:
    /** 借用本轮测试状态，测试owner在join之后释放
    @param [in] test 借用测试状态，owner回收任务后才释放
    */
    explicit NetWorkHostBridgeTestTask(NetWorkHostBridgeTest* test);

    /** 从另一执行者提交两个peer的原始事件，随后改写源缓冲
    */
    virtual void DoTask();
private:
    // owner保留至任务退出
    NetWorkHostBridgeTest* m_test;
};

// 四个生产者向同一桥发送，由唯一owner核对各自FIFO
class NetWorkHostBridgeSequenceTask : public CTask
{
public:
    /** 借用审计状态到owner回收任务
    @param [in] test 有效且在任务结束前存活
    @param [in] producer 0至3
    */
    NetWorkHostBridgeSequenceTask(NetWorkHostBridgeTest* test, uint32_t producer);

    /** 发送400个带序号输入，不推进owner
    */
    virtual void DoTask();

private:
    // 借用到任务结束
    NetWorkHostBridgeTest* m_test;
    // 本生产者编号
    uint32_t m_producer;
};