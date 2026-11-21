#pragma once
#include <functional>
#include <memory>
#ifndef __EMSCRIPTEN__
#include <thread>
#endif
#include <cstdint>
#include "TimerMacro.h"

struct TimerState;

/** 定时器。Windows、Linux、macOS、Android、iOS在独立线程运行回调。
Wasm由创建实例的宿主事件循环串行调度。创建、修改、停止和销毁均在此事件循环执行。
*/
class TimerAPI Timer
{
public:
    /** 创建定时器，空回调抛出std::invalid_argument
    @param [in] callback 启动后重复执行的回调，定时器持有其副本或移动结果
    @param [in] interval_ms 首次等待毫秒数，0使用1000毫秒
    */
    Timer(std::function<void()> callback, uint64_t interval_ms = 1000);

    /** 撤销后续回调，原生端从外部析构时等待正在执行的回调退出
    */
    ~Timer();

    /** 启动重复调度，首个周期采用本次启动时的间隔。已启动时重复调用无效
    */
    void start();

    /** 撤销后续回调且立即唤醒原生等待，可在回调中停止并重新启动
    原生工作线程已经取得执行资格的回调仍可能在 stop 返回后开始或继续执行。外部线程析构后回调已经结束。
    */
    void stop();

    /** 更新下一个等待周期
    @param [in] interval_ms 毫秒，0保持当前值，正在等待的周期不改期
    */
    void set_interval(uint64_t interval_ms);

    /** 查询当前启停状态
    @return 已启动且未停止为true
    */
    bool is_running() const;

private:
    /** 禁止复制已拥有调度状态的实例
    @param [in] other 原实例
    */
    Timer(const Timer& other) = delete;

    /** 禁止通过复制赋值共享调度状态或工作线程
    @param [in] other 原实例
    @return 不可调用
    */
    Timer& operator=(const Timer& other) = delete;

private:
    // 回调执行期间保持有效的调度状态
    std::shared_ptr<TimerState> m_state;
#ifndef __EMSCRIPTEN__
    // 原生工作线程，外部析构时等待结束
    std::shared_ptr<std::thread> m_worker;
#endif
};