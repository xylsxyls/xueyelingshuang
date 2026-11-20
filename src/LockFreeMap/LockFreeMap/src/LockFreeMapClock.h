#pragma once
#include <stdint.h>
#include <chrono>
#include <limits>
#ifdef _WIN32
#include <Windows.h>
#endif

/** 单调计时适配；Windows 避开 VS2013 steady_clock 的墙上时钟实现
*/
class LockFreeMapClock
{
public:
    /** 读取单调毫秒时间，保留零表示不可用
    @return 单调毫秒加一，系统失败或换算溢出时返回零
    */
    static uint64_t milliseconds();

    /** 达到快速重试阈值后，按实例配置等待容量
    @param [in,out] retries 本次操作已使用的快速重试次数，达到阈值后保持不变
    @param [in] fastRetries 首次休眠前可快速重试的次数
    @param [in] sleepMilliseconds 休眠时长，配置入口保证为正毫秒数
    @return true允许重试；阻塞不可用或系统异常时false
    */
    static bool waitCapacity(uint32_t& retries, uint32_t fastRetries = 4096, int32_t sleepMilliseconds = 1);

private:
    /** 换算系统计数器，并检查整数及小数乘积溢出
    @param [in] units 每秒单位数，当前毫秒接口传入 1000
    @return 非零单调时间，不可用时为零
    */
    static uint64_t sample(uint64_t units);
};

#include "LockFreeMapClock.inl"