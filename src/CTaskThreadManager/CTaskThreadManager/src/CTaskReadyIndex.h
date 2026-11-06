#pragma once
#include <atomic>
#include <stdint.h>
#if defined(_MSC_VER) && _MSC_VER == 1800 && (defined(_M_X64) || (defined(_M_IX86) && _M_IX86_FP >= 2))
#include <emmintrin.h>
#include <intrin.h>
#endif

/** 常用优先级的可帮助最高位置提示，不拥有任务，不等待发布者
*/
class CTaskReadyIndex
{
public:
    /** 最低级别始终检查，初始最高提示为 1
    */
    CTaskReadyIndex();

    /** 在任务及级别提示发布后提高最高位置
    @param [in] level 正数优先级；本库常用路径使用 1 至 16
    */
    void publish(int32_t level);

    /** 观察当前最高提示，可能暂时包含已经消费完的级别
    @return 至少为 1 的优先级
    */
    int32_t highest() const;

    /** 唯一消费者将空级别下调一步；成功后必须重新检查该级别
    @param [in] level 刚观察为空的最高级别；重新发现任务时调用 publish 恢复
    @return 是否下调成功；失败不等待并发发布者
    */
    bool lower(int32_t level);
    /** 发布与下调复查间的 SC 屏障；v120 x86/x64 不借用标准库的全局 Guard 缓存行
    唤醒协议也使用此屏障连接队列发布、通知清除与二次检查。
    */
    static void fence();
private:
    // 生产者只提高，消费者逐级降低；两侧用 SC fence 连接队列发布与复查
    std::atomic<int32_t> m_highest;
};

#include "CTaskReadyIndex.inl"