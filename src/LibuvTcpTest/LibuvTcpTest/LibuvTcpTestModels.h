#pragma once
#include "uv.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <functional>
#include <vector>
#include <stdint.h>

/** 通过正式线程库执行可合作停止的测试任务
*/
class LibuvTestTask : public CTask
{
public:
    /** 复制执行和停止操作
    @param [in] run 工作线程操作
    @param [in] stop 合作停止操作
    */
    LibuvTestTask(const std::function<void()>& run, const std::function<void()>& stop);

    /** 执行测试操作
    */
    virtual void doTask();

    /** 通知测试操作停止，不等待工作线程
    */
    virtual void stopTask();
private:
    // 创建后不再修改的执行操作
    std::function<void()> m_run;
    // 可并发调用的停止操作
    std::function<void()> m_stop;
};

/** 拥有一条正式线程库线程，释放前结束全部任务
*/
class LibuvTestThread
{
public:
    /** 创建空的测试执行者
    */
    LibuvTestThread();

    /** 请求停止并等待线程退出
    */
    ~LibuvTestThread();

    /** 启动一项长任务
    @param [in] run 实际操作
    @param [in] stop 合作退出通知
    @return 是否已被线程库接纳
    */
    bool start(const std::function<void()>& run, const std::function<void()>& stop);

    /** 正常排空并等待已启动任务完成
    @return 线程是否已结束
    */
    bool join();
private:
    // 非零时拥有线程注册，析构完成注销
    uint32_t m_id;
};

/** 多生产者共用的长度帧连接，只在回调线程发布连接地址
*/
class LibuvTestClient : public LibuvTcp
{
public:
    /** 初始化空观测状态
    */
    LibuvTestClient();

    /** 发布可发送连接
    @param [in] server 当前连接
    */
    virtual void uvServerConnected(uv_tcp_t* server);

    /** 记录接收字节和内容，不延长回调缓冲寿命
    @param [in] sender 当前连接
    @param [in] buffer 回调借用缓冲
    @param [in] length 实际字节数
    */
    virtual void receive(uv_tcp_t* sender, const char* buffer, int32_t length);

    /** 记录连接失败
    */
    virtual void uvServerNotFind();

    /** 清除连接借用
    @param [in] server 已断开连接
    */
    virtual void uvServerDisconnected(uv_tcp_t* server);

    // 连接回调发布，测试发送者只在连接存活期借用
    std::atomic<uv_tcp_t*> m_target;
    // 返回字节数，作为实际接收证据
    std::atomic<uint64_t> m_received;
    // 接收内容或异步连接发生错误
    std::atomic<bool> m_failed;
    // 连接前设置；测试用系统发送缓冲限制，0 使用系统默认
    int32_t m_sendBufferSize;
};

/** 单 owner 原始 TCP 服务，用独立的字节校验器验证每条生产者顺序
*/
class LibuvTestReceiver : public LibuvChannelListener
{
public:
    /** 固定期望帧长度与生产者数
    @param [in] payload 正文长度，至少 8 字节
    @param [in] producers 1 至 16 个发送者
    @param [in] timed 是否解析性能测试的时间字段并采样延迟
    */
    LibuvTestReceiver(uint32_t payload, uint32_t producers, bool timed = false);

    /** 记录接入 ID
    @param [in] peer 已接纳连接号
    */
    virtual void onConnected(uint64_t peer);

    /** 验证拆包、内容和每个生产者的序号
    @param [in] peer 连接号
    @param [in] data 借用字节
    @param [in] size 当前字节数
    */
    virtual void onData(uint64_t peer, const uint8_t* data, uint32_t size);

    /** 记录写完成
    @param [in] peer 当前连接号
    */
    virtual void onWritable(uint64_t peer);

    /** 记录连接关闭
    @param [in] peer 当前连接号
    @param [in] reason 关闭原因
    */
    virtual void onClosed(uint64_t peer, int32_t reason);

    // 仅服务 owner 访问的原始字节通道
    LibuvChannel m_channel;
    // 每生产者已验证的帧数，生产者据此限制测试窗口
    std::atomic<uint32_t> m_counts[16];
    // 错误只置位，不因后续成功掩盖
    std::atomic<bool> m_failed;
    // 服务器运行操作的合作退出标志
    std::atomic<bool> m_exit;
    // 只由 owner 维护的连接号
    uint64_t m_peer;
    // 单 owner 实际连接事件数
    uint32_t m_connectedCount;
    // 单 owner 实际终态事件数
    uint32_t m_closedCount;
    // 性能运行时每 64 帧采样一次发送至完整校验的延迟；只由 owner 写入
    std::vector<uint64_t> m_latencies;
    // 最后一次完整帧校验时间，完成后由调用线程读取
    std::atomic<uint64_t> m_lastReceive;
private:
    // 固定正文大小
    uint32_t m_payload;
    // 有效发送者数
    uint32_t m_producers;
    // true 时正文第 8 至 15 字节为单调发送时间，其余仍逐字节验证
    bool m_timed;
    // 累积一帧的有界缓冲
    std::vector<uint8_t> m_frame;
};

/** 单线程 host 入口观测器，拥有实例且在结束后检查唯一终态
*/
class LibuvTestHost
{
public:
    /** 创建空观测器
    */
    LibuvTestHost();

    /** 回收尚未显式结束的实例
    */
    ~LibuvTestHost();

    /** 通过正式 hostCreate 创建实例
    @return 是否成功
    */
    bool initialize();

    /** 通过正式 hostDestroy 回收，成功后清空借用
    @return 是否完整结束
    */
    bool finish();

    /** 连接回调，核对重入保护并按指定模式抛出测试异常
    @param [in] context 当前观测器
    @param [in] peer 连接号
    */
    static void connected(void* context, uint64_t peer);

    /** 核对实际数据
    @param [in] context 当前观测器
    @param [in] peer 连接号
    @param [in] data 借用数据
    @param [in] size 字节数
    */
    static void data(void* context, uint64_t peer, const uint8_t* data, uint32_t size);

    /** 核对写完成回调
    @param [in] context 当前观测器
    @param [in] peer 连接号
    */
    static void writable(void* context, uint64_t peer);

    /** 记录终态并核对关闭回调中重入限制
    @param [in] context 当前观测器
    @param [in] peer 已关闭连接号
    @param [in] reason 关闭原因
    */
    static void closed(void* context, uint64_t peer, int32_t reason);

    // 本观测器拥有的 host 实例
    void* m_instance;
    // 最近连接号，只由当前测试执行者读写
    uint64_t m_peer;
    // 实际收到的正文长度
    uint64_t m_received;
    // 建立通知次数
    uint32_t m_connected;
    // 完成写次数
    uint32_t m_written;
    // 终态次数
    uint32_t m_closed;
    // 注入阶段：0 不注入，1 连接，2 数据，3 写完成，4 关闭
    uint32_t m_throwOn;
    // 任一内容或契约校验失败后不再清零
    bool m_failed;
};

/** 只在测试 owner 回调中观察异步写窗口，跨线程只发布原子结果
*/
class LibuvTestWindowProbe
{
public:
    /** 初始化尚未注册的观察器
    */
    LibuvTestWindowProbe();

    /** 在 loop 启动前注册定时观察，观察器须存活至 loop 结束
    @param [in,out] client 已初始化且尚未运行的客户端
    @return 注册是否成功
    */
    bool start(LibuvTcp& client);

    /** 观察满窗口和取消后资源回落，停止时关闭本测试拥有的定时器
    @param [in] timer 本观察器的句柄
    */
    static void tick(uv_timer_t* timer);

    // 是否实际达到 256 个在途异步写请求
    std::atomic<bool> m_full;
    // 满窗口后连接关闭且发送预算已回到零
    std::atomic<bool> m_canceled;
    // 启动前配置：在真实满预算时核对循环内等待被拒绝
    bool m_checkOwnerWait;
    // 实际执行过循环内调用，不以没有卡死替代入口验证
    std::atomic<bool> m_ownerWaitChecked;
    // 满预算时循环内 trySend(true,true) 返回非法上下文
    std::atomic<bool> m_ownerWaitRejected;
    // 只用于固定交错：暂时不让 owner 消费取消队列，停止时必须退出等待
    std::atomic<bool> m_pauseLoop;
    // owner 已进入固定交错等待点
    std::atomic<bool> m_loopPaused;
    // owner发布的定时回调次数，用于不借用布局地验证循环仍在推进
    std::atomic<uint32_t> m_ticks;
private:
    // 独立测试句柄，由 tick 在宿主停止时关闭
    uv_timer_t m_timer;
    // 借用至 loop 完全结束
    LibuvTcp* m_client;
};