#pragma once
#include <Windows.h>
#include <atomic>
#include <functional>
#include <thread>
#include <vector>

/** 性能测试线程所有者，起跑门独立于被测库
*/
class SemaphorePerformanceThreads
{
public:
    /** 准备初始关闭的系统起跑门，异常由测试入口捕获
    */
    SemaphorePerformanceThreads();

    /** 取消测试步骤，释放起跑门并回收所有已启动线程
    */
    ~SemaphorePerformanceThreads();

    /** 添加受异常边界保护的工作线程
    @param [in] task 有限测试步骤，捕获状态活到线程回收
    */
    void add(const std::function<void()>& task);

    /** 释放全部已经就绪的工作线程
    */
    void start();

    /** 回收线程并报告工作线程失败
    */
    void join();

    /** 检查协作取消状态
    @return 测试已经失败或正在收尾时为true
    */
    bool cancelled() const;

private:
    // 测试专用系统事件，不进入Semaphore实现
    HANDLE m_gate;
    // 被捕获状态销毁前必须回收的线程
    std::vector<std::thread> m_threads;
    // 有限循环使用的协作取消标记
    std::atomic<bool> m_cancelled;
    // 在线程入口捕获的异常数
    std::atomic<int32_t> m_errors;
};

/** 稳定性能用例，原版和当前版共用工作量、计时边界及断言
*/
class SemaphorePerformance
{
public:
    /** 执行指定性能组
    @param [in] id 55完整组，66冷唤醒与IPC组，71对齐，72事件，73计数，74命名IPC，79记录池
    @param [in] combined 是否使用组合接口，原版不支持
    @return 计数或超时判定失败返回非零
    */
    static int32_t run(int32_t id = 55, bool combined = false);

    /** 执行独立启动的命名IPC对端
    @param [in] name 父进程生成的唯一名称
    @param [in] combined 是否使用组合接口
    @return 退出码
    */
    static int32_t peer(const char* name, bool combined = false);

    /** 判定性能测量前后的正确性
    @param [in] value 必须为true的条件
    @param [in] message 失败说明
    */
    static void require(bool value, const char* message);

private:
    /** 在不同递归深度中检查实际对象地址，禁止优化成单个栈帧
    @param [in] depth 剩余递归层数，测试固定不超过15
    @return 不满足8字节边界的对象数
    */
    static uint32_t stackAlignment(uint32_t depth);

    /** 检查主线程及工作线程的实际栈对象，不以类型alignof代替地址检查
    */
    static void alignment();

    /** 取得共享单调时钟
    @return 微秒
    */
    static double now();

    /** 读取进程CPU时间
    @return 用户态与内核态累计毫秒
    */
    static double cpu();

    /** 保留有序原始样本，再按最近秩输出延迟分位数
    @param [in] name 指标名称
    @param [in,out] samples 全部样本，输出后排序
    */
    static void latency(const char* name, std::vector<double>& samples);

    /** 测量一百万个先发布后消费的积存许可
    */
    static void bulk();

    /** 测量并发发布消费
    @param [in] name 指标名称
    @param [in] producers 生产者数，必须整除固定工作量
    @param [in] consumers 消费者数，必须整除固定工作量
    */
    static void contention(const char* name, int32_t producers, int32_t consumers);

    /** 测量请求回复或发布到观察的延迟
    @param [in] cold 是否在计时前先空闲5毫秒
    */
    static void handoff(bool cold);

    /** 使用外部代次谓词测量非积存事件
    @param [in] count 等待者数，1或8
    @param [in] prepared 是否在启动线程前准备系统事件后端，仅用于72号诊断
    */
    static void events(int32_t count, bool prepared = false);

    /** 测量两秒空等待及进程CPU消耗
    */
    static void idle();

    /** 使用正常分配器测量完整构造和析构
    */
    static void lifecycle();

    /** 仅在独立静态库分配统计配置中统计构造、计数和首次/复用事件等待的C++申请
    */
    static void allocations();

    /** 测量预分配事件记录完整循环，每个原始样本包含若干完整批次并换算为每条记录的平均微秒
    仅静态消费配置启用内部类型，避免把测试符号导出到生产DLL
    @param [in] count 同时登记记录数，1、8或64
    */
    static void poolCycles(size_t count);

    /** 测量真实独立进程的通知往返
    */
    static void ipc();
};