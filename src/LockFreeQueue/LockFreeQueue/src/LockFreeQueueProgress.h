#pragma once

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <thread>

// 单执行流拥有的调度让步计数；不执行其他生产者的载荷，不改变系统线程优先级。
class LockFreeQueueProgress
{
public:
    /** 初始化尚未共享的生产者登记的操作预算
    @param [in] operations 每次让步之间的操作数，已由配置入口校验为 1..65535
    */
    void configureInterval(uint16_t operations);

    /** 建立确定状态，默认内部重试；公开尝试接口须显式 begin(false)
    */
    LockFreeQueueProgress();

    /** 开始一次操作，保留跨调用的调度计数
    @param [in] wait true 持续重试竞争，false 允许竞争失败返回
    */
    void begin(bool wait);

    /** 处理一次竞争失败；持续竞争时让步，长期竞争时短暂休眠；休眠失败回退为让步
    @param [in] attemptBudget try 模式允许的有限重试次数；默认立即返回，push 忽略此上限
    @return true 继续由本线程重试，false 本次尝试未提交
    */
    bool retry(uint8_t attemptBudget = 0);

    /** 标记保护验证中断；调用方可以重新开始本线程的尝试
    */
    void interrupt();

    /** 判断上次尝试是否因竞争中断，区别于真实资源不足
    @return 是否因竞争中断
    */
    bool interrupted() const;

    /** 判断本次调用是否持续重试竞争
    @return true 为 push 模式，false 为尝试模式
    */
    bool waiting() const;

    /** 清除中断标记，保留持续竞争的退避计数
    */
    void resume();

    /** 累积本线程实际执行的操作，连续成功或空轮询也定期让出执行机会
    @param [in] count 本轮操作数量，0不计费；整批达到剩余额度只让步一次，超额不结转
    */
    void advance(size_t count = 1);

    /** 容量不足时按配置重试，每16次让步；进入休眠阶段后不再累加计数
    @param [in,out] retries 本次操作的容量重试次数，开始时为0
    @param [in] retriesBeforeSleep 转入休眠前的重试上限，0 直接进入休眠阶段
    @param [in] sleepMilliseconds 等待毫秒数，调用方已校验为正
    @return true继续重试；宿主不支持阻塞或休眠失败时false
    */
    static bool waitCapacity(uint32_t& retries, uint32_t retriesBeforeSleep, uint32_t sleepMilliseconds);

    /** Short scheduler backoff for sustained contention or a lost publication CAS
    Sleep failure falls back to yield; actual wake-up latency is host-controlled
    No ownership is retained on behalf of another producer; it retries itself
    */
    static void pausePublisher();

private:
    /** 按配置重置本执行流的操作预算并让出执行机会；仅由预算耗尽路径调用
    */
    void yieldOperations();

private:
    // 距离下一次协作让步的剩余操作数，始终为1..m_interval。
    uint16_t m_remaining;
    // 预算用尽后的重置值，默认 4096
    uint16_t m_interval;
    // 状态字低八位保存周期退避次数
    static const int32_t kFailureMask = 255;
    // 本次调用持续重试竞争的标志位
    static const int32_t kWaiting = 256;
    // 本次尝试因竞争中断的标志位
    static const int32_t kInterrupted = 512;
    // 低八位为周期退避计数，高位保存模式及中断标志；开始操作只写一个状态字。
    uint16_t m_state;
};

#include "LockFreeQueueProgress.inl"