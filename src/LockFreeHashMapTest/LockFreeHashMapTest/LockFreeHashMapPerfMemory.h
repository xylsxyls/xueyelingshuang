#pragma once
#include <stddef.h>
#include <stdint.h>

/** 只在独立的单线程内存计量程序中链接，不进入吞吐计时二进制
*/
class LockFreeHashMapPerfMemory
{
public:
    // 是否给新申请添加计量标记，已有标记的释放始终撤销计数
    static bool s_tracking;
    // 当前被计量的请求堆字节，不含计账头和分配器开销
    static size_t s_bytes;
    // 被计量请求字节峰值
    static size_t s_peak;
    // 成功系统申请次数
    static uint64_t s_allocations;
};