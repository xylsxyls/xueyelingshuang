#pragma once
#include "Semaphore.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

/** 测试线程所有者，捕获全部工作线程异常并在被借用状态销毁前回收线程
*/
class SemaphoreTestThreads
{
public:
    /** 预留测试线程容器，申请失败由测试入口捕获
    */
    SemaphoreTestThreads();

    /** 判定抛异常时仍回收所有已启动线程
    */
    ~SemaphoreTestThreads();

    /** 启动一个捕获异常的测试线程
    @param [in] task 测试步骤，捕获对象必须活到线程回收
    */
    void start(const std::function<void()>& task);

    /** 回收全部线程并将工作线程异常报告为测试失败
    */
    void join();

private:
    // 测试线程独立于被测Semaphore，不经线程管理器反向使用被测对象
    std::vector<std::thread> m_threads;
    // 工作线程异常累计，不允许异常逃出线程入口
    std::atomic<int32_t> m_errors;
};

/** 独立于被测对象的定向暂停门，只用于测试交错
*/
class SemaphoreTestGate
{
public:
    /** 准备暂停点
    @param [in] point 本地钩子编号或池钩子编号加100
    */
    explicit SemaphoreTestGate(int32_t point);

    /** 析构前释放暂停线程
    */
    ~SemaphoreTestGate();

    /** 仅暂停第一个命中指定位置的线程
    @param [in] point 实际到达的钩子编号
    */
    void hit(int32_t point);

    /** 至多等待五秒确认测试线程到达
    @return 是否在期限内到达
    */
    bool reached();

    /** 解除暂停并禁止本轮再次进入
    */
    void release();

private:
    // 测试暂停协议使用独立系统同步，不属于Semaphore实现或性能计时
    std::mutex m_mutex;
    // 暂停门状态变化通知
    std::condition_variable m_changed;
    // 本轮选定的钩子编号
    int32_t m_point;
    // 第一个目标线程是否已到达
    bool m_arrived;
    // 本轮是否已解除暂停
    bool m_released;
};

/** 稳定用例25起的功能断言，历史1至24不再执行且编号不复用
*/
class SemaphoreTests
{
public:
    /** 验证等待者进展、直接交付和持续竞争；私有交错仅在测试生成副本启用
    @param [in] id 稳定用例82至86，大计数85由runCase执行
    @return 已执行返回true，不适用配置返回false
    */
    static bool fairness(int32_t id);

    /** 命名通知子进程入口
    @param [in] name 父进程生成的本轮唯一名称
    @param [in] exchange 是否进行200次组合往返
    @return 退出码
    */
    static int32_t peer(const std::string& name, bool exchange = false);

    /** 通过异常将失败交给测试入口记录，不主动崩溃
    @param [in] ok 独立预期是否满足
    @param [in] text 失败说明
    */
    static void check(bool ok, const char* text);

    /** 取得测试用单调时钟
    @return 毫秒
    */
    static double now();

    /** 测试调度延迟
    @param [in] ms 毫秒
    */
    static void pause(int32_t ms);

public:
    /** 执行稳定用例
    @param [in] id 稳定编号
    @return 本配置不适用返回false，判定失败抛异常
    */
    static bool runCase(int32_t id);

    /** 生成本进程本用例独占名称
    @param [in] suffix 用例编号
    @return 逻辑名称
    */
    static std::string name(int32_t suffix);
};