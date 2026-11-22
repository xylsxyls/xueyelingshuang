#pragma once
#include "libuv/uv.h"
#include <atomic>

/** 原始字节宿主的共同执行循环；连接由一个执行者拥有，外部线程只允许wake
*/
class LibuvHostLoop
{
public:
    /** 初始化空状态，init成功后才能借给通道
    */
    LibuvHostLoop();

    /** 释放无通道、无外部唤醒者的静止循环
    */
    ~LibuvHostLoop();

    /** 创建循环、合并唤醒和一次性期限定时器
    @return 0成功，负数libuv错误
    */
    int32_t init();

    /** 唯一执行者驱动一次；回调内拒绝重入
    @param [in] mode libuv运行方式，只使用NOWAIT或ONCE
    @return 0成功，负数重入错误
    */
    int32_t run(uv_run_mode mode);

    /** 发布持久代次并合并跨线程唤醒，借用必须延续到返回
    */
    void wake();

    /** 等待本轮事件；若通知代次已变化，非阻塞驱动一次循环后返回
    @param [in] observed 本轮开始前读取的代次
    @param [in] timeoutMs 最迟维护期限，0只非阻塞推进
    @return 0正常，负数宿主失败
    */
    int32_t wait(uint64_t observed, uint32_t timeoutMs);

    /** 交换当前执行者的通道创建归属，不改变已有通道
    @param [in] loop 本轮执行循环，可空恢复独立通道模式
    @return 之前的借用循环
    */
    static LibuvHostLoop* exchange(LibuvHostLoop* loop);

    /** 查询当前执行者借用的创建归属
    @return 当前循环或空
    */
    static LibuvHostLoop* current();

private:
    /** 消费已合并通知，队列和结果由上层执行者检查
    @param [in] handle 本循环异步句柄
    */
    static void notified(uv_async_t* handle);

    /** 结束本次等待，具体超时由协议使用自己的单调时钟判断
    @param [in] handle 本循环定时器
    */
    static void elapsed(uv_timer_t* handle);

    /** 禁止复制循环资源
    @param [in] other 原对象
    */
    LibuvHostLoop(const LibuvHostLoop& other);

    /** 禁止复制循环资源
    @param [in] other 原对象
    @return 不可调用
    */
    LibuvHostLoop& operator=(const LibuvHostLoop& other);

public:
    // 所有借用通道共享的操作系统事件循环
    uv_loop_t m_loop;
    // 只允许wake跨线程操作，生命周期由上层封闭后再销毁
    uv_async_t m_async;
    // 每次等待独立启动，任何事件返回后立即停止
    uv_timer_t m_timer;
    // 事件代次，避免poll消费通知后误入等待；不承载业务数据
    std::atomic<uint64_t> m_generation;
    // 仍持有本循环的通道数，只由唯一执行者修改
    uint32_t m_channels;
    // 拒绝同一循环嵌套驱动和回调内释放通道
    bool m_running;
    // 分阶段初始化成功标记，失败只释放已初始化句柄
    bool m_initialized;
    // 异步句柄初始化成功标记
    bool m_asyncInitialized;
    // 定时器初始化成功标记
    bool m_timerInitialized;
};