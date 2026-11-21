#pragma once
#include <cstdint>
#include <functional>
#ifdef __EMSCRIPTEN__
#include <memory>
#else
#include <atomic>
#include <condition_variable>
#include <mutex>
#endif

/** 一代 Timer 实例的调度状态，回调执行期间由执行者持有
*/
struct TimerState
#ifdef __EMSCRIPTEN__
    : public std::enable_shared_from_this<TimerState>
#endif
{
public:
    // 每次到期时调用，创建成功后保持有效
    std::function<void()> m_callback;
    // 下一等待周期采用的毫秒数
    uint64_t m_interval;
#ifdef __EMSCRIPTEN__
    // 当前分段等待结束后剩余的毫秒数
    uint64_t m_remaining;
    // 宿主事件循环创建的定时句柄
    int m_handle;
    // 当前是否有需要撤销的宿主回调
    bool m_pending;
    // 当前事件循环是否应继续重复调度
    bool m_running;
#else
    // 串行化代次、等待条件和间隔更新
    std::mutex m_mutex;
    // 唤醒正在等启动、超时或退出的工作线程
    std::condition_variable m_changed;
    // 每次启动或停止时递增，取消旧周期
    uint64_t m_generation;
    // 供外部线程无锁读取的当前启停状态
    std::atomic<bool> m_running;
    // 置位后工作线程退出，仅在 m_mutex 保护下读写
    bool m_closed;
    // start 已提交、等待线程领取的首周期，0 表示已领取
    uint64_t m_startInterval;
#endif

public:
    /** 初始化回调和下一周期
    @param [in] callback 非空回调，状态取得其所有权
    @param [in] interval 毫秒，零表示1000毫秒
    */
    TimerState(std::function<void()> callback, uint64_t interval);

    /** 在调度线程执行用户回调并隔离用户异常
    */
    void invoke();

#ifdef __EMSCRIPTEN__
    /** 启动一次可取消的分段等待
    @param [in] delay 总等待毫秒数，可超过宿主32位上限
    */
    void schedule(uint64_t delay);

    /** 撤销当前分段并归还宿主事件循环引用
    */
    void cancel();

    /** 消费一次宿主定时事件
    @param [in] context 调度时登记的 TimerState，由定时器或执行中的回调持有
    */
    static void tick(void* context);
#else
    /** 等待有效代次到期后执行回调，直至所有者关闭本实例
    */
    void run();
#endif
};