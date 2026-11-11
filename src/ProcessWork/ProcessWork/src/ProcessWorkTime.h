#pragma once
#include "ProcessWorkMacro.h"
#include <stdint.h>

/** 维护用单调时间，避开VS2013将steady_clock实现为系统墙钟的问题。
*/
class ProcessWorkAPI ProcessWorkTime
{
public:
    /** 返回系统启动以来或平台单调时钟纪元以来的毫秒数。
    @return 同一进程内可比较的单调时间，不用于不同系统的时间同步
    */
    static uint64_t milliseconds();

    /** 容量不足快速重试4096次，之后每次等待1毫秒
    @param [in,out] attempts 本次操作容量失败计数，饱和于4096
    @return 可以重试返回true，等待系统接口失败返回false
    */
    static bool waitCapacity(uint32_t& attempts);
};