#pragma once
#include "SemaphoreResult.h"

/** 一次调用局部的重试退避，不拥有资源，也不等待独占持有者
*/
class SemaphoreRetry
{
public:
    /** 容量不足先快速重试4096次，此后每次等待1毫秒
    @param [in,out] attempts 本次操作的容量失败计数，饱和于4096
    @return OK可以重新尝试；宿主不支持阻塞或系统失败时返回错误
    */
    static SemaphoreResult capacity(uint32_t& attempts);

    /** 竞争失败后x86/x64短退避16至256条PAUSE，每16次失败让出时间片；其他架构定期让出时间片。
    不设置竞争重试总次数上限；该退避不等待许可，无许可时仍由系统阻塞
    @param [in,out] attempts 本次操作的竞争失败计数
    */
    static void contention(uint32_t& attempts);
};