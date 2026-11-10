#include "LockFreeHashMapBenchmark.h"
#include <LockFreeHashMap.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#endif

bool LockFreeHashMapBenchmark::s_tracking = false;
uint64_t LockFreeHashMapBenchmark::s_allocations = 0;
size_t LockFreeHashMapBenchmark::s_bytes = 0;
size_t LockFreeHashMapBenchmark::s_peakBytes = 0;

double LockFreeHashMapBenchmark::timestamp()
{
#if defined(_WIN32)
    LARGE_INTEGER ticks = {}, frequency = {};
    if (QueryPerformanceCounter(&ticks) && QueryPerformanceFrequency(&frequency) && frequency.QuadPart != 0)
    {
        return double(ticks.QuadPart) / double(frequency.QuadPart);
    }
#endif
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

/** 追踪本测试进程的实际申请，故障注入只属于测试程序
@param [in] size 请求字节数，先检查额外计账空间的溢出
@return 正确对齐的申请地址，失败抛 bad_alloc 供调用边界处理
*/
void* operator new(size_t size)
{
    if (size > (std::numeric_limits<size_t>::max)() - 32)
    {
        throw std::bad_alloc();
    }
    unsigned char* raw = static_cast<unsigned char*>(std::malloc(size + 32));
    if (raw == nullptr)
    {
        throw std::bad_alloc();
    }
    size_t* header = reinterpret_cast<size_t*>(raw);
    header[0] = size;
    header[1] = LockFreeHashMapBenchmark::s_tracking ? 1 : 0;
    if (header[1] != 0)
    {
        ++LockFreeHashMapBenchmark::s_allocations;
        LockFreeHashMapBenchmark::s_bytes += size;
        LockFreeHashMapBenchmark::s_peakBytes = (std::max)(LockFreeHashMapBenchmark::s_peakBytes, LockFreeHashMapBenchmark::s_bytes);
    }
    return raw + 32;
}

/** 释放配对申请并精确撤销计账
@param [in] pointer 对应测试申请地址，可为空，载荷必须已析构
*/
void operator delete(void* pointer) throw()
{
    if (pointer != nullptr)
    {
        unsigned char* raw = static_cast<unsigned char*>(pointer) - 32;
        size_t* header = reinterpret_cast<size_t*>(raw);
        if (header[1] != 0)
        {
            LockFreeHashMapBenchmark::s_bytes -= header[0];
        }
        std::free(raw);
    }
}

/** 追踪本测试进程的实际申请，故障注入只属于测试程序
@param [in] size 请求字节数，先检查额外计账空间的溢出
@return 正确对齐的申请地址，失败抛 bad_alloc 供调用边界处理
*/
void* operator new[](size_t size)
{
    return ::operator new(size);
}

/** 释放配对申请并精确撤销计账
@param [in] pointer 对应测试申请地址，可为空，载荷必须已析构
*/
void operator delete[](void* pointer) throw()
{
    ::operator delete(pointer);
}

/** 执行独立对照或组合测试，所需资源在返回前释放
@return 0 通过，非零表示数据、资源或设施失败
*/
int main()
{
    std::cout << "BENCH pairs=10000 keys=64 single_thread=1 requested_heap_bytes=1 warm_numeric=" << 0.0 << uint64_t(0) << size_t(0) << std::endl;
    LockFreeHashMapBenchmark::s_tracking = true;
    try
    {
        for (int mode = 0; mode < 4; ++mode)
        {
            size_t buckets = mode == 0 ? 1024 : (mode == 1 ? 8 : 1);
            std::vector<double> latencies(10000);
            size_t before = LockFreeHashMapBenchmark::s_bytes;
            uint64_t beforeAllocations = LockFreeHashMapBenchmark::s_allocations;
            LockFreeHashMapBenchmark::s_peakBytes = before;
            {
                LockFreeHashMap<int, int> map(buckets);
                double start = LockFreeHashMapBenchmark::timestamp();
                for (int i = 0; i < 10000; ++i)
                {
                    double begin = LockFreeHashMapBenchmark::timestamp();
                    int key = i % 64;
                    int value = -1;
                    if (!map.set(key, i) || !map.get(key, value) || value != i || (mode == 3 && !map.erase(key)))
                    {
                        std::cerr << "BENCH incorrect data at " << i << std::endl;
                        return 1;
                    }
                    latencies[i] = (LockFreeHashMapBenchmark::timestamp() - begin) * 1000000;
                }
                double elapsed = LockFreeHashMapBenchmark::timestamp() - start;
                std::sort(latencies.begin(), latencies.end());
                size_t occupied = map.retainedNodeCount();
                size_t bytes = LockFreeHashMapBenchmark::s_bytes - before;
                size_t peak = LockFreeHashMapBenchmark::s_peakBytes - before;
                uint64_t allocations = LockFreeHashMapBenchmark::s_allocations - beforeAllocations;
                map.clear();
                std::cout << "BENCH mode=" << mode << " buckets=" << buckets << " ops_per_second=" << 10000 / elapsed
                    << " p50_us=" << latencies[5000] << " p95_us=" << latencies[9500] << " p99_us=" << latencies[9900]
                    << " retained_units=" << occupied << " retained_bytes=" << bytes << " peak_bytes=" << peak
                    << " allocations=" << allocations << " after_clear_units=" << map.retainedNodeCount()
                    << " after_clear_bytes=" << LockFreeHashMapBenchmark::s_bytes - before << std::endl;
            }
            if (LockFreeHashMapBenchmark::s_bytes != before)
            {
                std::cerr << "BENCH resources remain after destruction" << std::endl;
                return 2;
            }
        }
    }
    catch (...)
    {
        std::cerr << "BENCH unexpected exception" << std::endl;
        return 3;
    }
    return 0;
}