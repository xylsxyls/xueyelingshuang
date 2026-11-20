#pragma once
#include "SemaphoreResult.h"

struct SemaphoreWaitPolicy;

/** 一次调用局部的重试退避，不拥有资源，也不等待独占持有者
*/
class SemaphoreRetry
{
public:
    /** 容量不足按实例配置先快速重试，再睡眠退让
    @param [in,out] attempts 本次操作的容量失败计数，饱和于fastAttempts
    @param [in] fastAttempts 睡眠前的快速重试次数
    @param [in] sleepMilliseconds 达到阈值后每轮睡眠毫秒数
    @return OK可以重新尝试；宿主不支持阻塞或系统失败时返回错误
    */
    static SemaphoreResult capacity(uint32_t& attempts, uint32_t fastAttempts, uint32_t sleepMilliseconds);

    /** 竞争失败后按配置退避，x86/x64先发出处理器提示，各架构达到阈值后让出时间片
    不设置竞争重试总次数上限；该退避不等待许可，无许可时仍由系统阻塞
    @param [in,out] attempts 本次操作的竞争失败计数
    @param [in] policy 本实例的只读退避配置
    */
    static void contention(uint32_t& attempts, const SemaphoreWaitPolicy& policy);
};