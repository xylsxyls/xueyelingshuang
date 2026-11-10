#pragma once

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <thread>

// 单执行流拥有的调度让步计数；不执行其他生产者的载荷，不改变系统线程优先级。
class LockFreeQueueProgress
{
public:
    /** 建立确定状态，默认内部重试；公开尝试接口须显式 begin(false) */
    LockFreeQueueProgress();

    /** 开始一次操作，保留跨调用的调度计数
    @param [in] wait true 持续重试竞争，false 允许竞争失败返回
    */
    void begin(bool wait);

    /** 处理一次竞争失败；持续竞争时让出时间片，长期竞争时短暂休眠
    @param [in] attemptBudget try 模式允许的有限重试次数；默认立即返回，push 忽略此上限
    @return true 继续由本线程重试，false 本次尝试未提交
    */
    bool retry(uint8_t attemptBudget = 0);

    /** 标记保护验证中断；调用方可以重新开始本线程的尝试 */
    void interrupt();

    /** 判断上次尝试是否因竞争中断，区别于真实资源不足
    @return 是否因竞争中断
    */
    bool interrupted() const;

    /** 判断本次调用是否持续重试竞争
    @return true 为 push 模式，false 为尝试模式
    */
    bool waiting() const;

    /** 清除中断标记，保留持续竞争的退避计数 */
    void resume();

    /** 累积本线程实际执行的操作，连续成功或空轮询也定期让出执行机会
    @param [in] count 本轮操作数量，默认一项
    */
    void advance(size_t count = 1);

private:
    // 小于 4096 的连续操作数量。
    uint16_t m_operations;
    enum
    {
        kFailureMask = 255,
        kWaiting = 256,
        kInterrupted = 512
    };
    // 低八位为周期退避计数，高位保存模式及中断标志；开始操作只写一个状态字。
    uint16_t m_state;
};

#include "LockFreeQueueProgress.inl"