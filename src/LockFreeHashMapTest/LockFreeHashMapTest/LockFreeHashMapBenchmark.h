#pragma once
#include <stddef.h>
#include <stdint.h>

/** 单线程对照分配器，计量实际申请的堆字节，不含分配器额外开销
*/
struct LockFreeHashMapBenchmark
{
public:
    // 仅在被测对象创建前开启，单线程运行
    static bool s_tracking;
    // 跟踪期间累计成功申请次数
    static uint64_t s_allocations;
    // 尚未释放的实际申请字节数，不是进程 RSS
    static size_t s_bytes;
    // 跟踪申请字节数的历史峰值
    static size_t s_peakBytes;
public:
    /** Windows 使用 QPC，其他平台使用单调时钟
    @return 单调秒数，只用于同一进程内时间差
    */
    static double timestamp();
};