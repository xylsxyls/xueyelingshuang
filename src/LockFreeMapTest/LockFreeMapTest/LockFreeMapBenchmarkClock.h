#pragma once
#include <stdint.h>
#include <chrono>
#include <limits>
#ifdef _WIN32
#include <Windows.h>
#endif

/** 基准专用计时，两个被测版本共用完全相同的采样实现
*/
class LockFreeMapBenchmarkClock
{
public:
    /** 读取单调纳秒样本，避免计时函数调用改变短负载口径
    @return 纳秒加一，系统失败或换算溢出时返回零
    */
    static uint64_t nanoseconds();
};

#include "LockFreeMapBenchmarkClock.inl"