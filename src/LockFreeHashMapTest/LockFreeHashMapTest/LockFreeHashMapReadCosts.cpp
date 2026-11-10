#include "LockFreeHashMapAPI.h"
#include "LockFreeHashMapTestThread.h"
#include <iostream>
#include <vector>
#include <chrono>
#if defined(_WIN32)
#include <Windows.h>
#endif

/** 读取诊断使用单调时钟，不改动正式性能用例
@return 秒数，失败返回零
*/
static double ReadClock()
{
#if defined(_WIN32)
    LARGE_INTEGER frequency = {}, tick = {};
    return QueryPerformanceFrequency(&frequency) && QueryPerformanceCounter(&tick) ?
        double(tick.QuadPart) / double(frequency.QuadPart) : 0;
#else
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

/** 比较相邻输出和分离输出，只用于定位缓存行争用，不能替代验收数据
@param [in] threads 线程数
@param [in] stride 相邻线程输出间隔，单位为整数
@param [in] control 是否在不访问容器的情况下模拟每次读取的本线程原子屏障
@return 校验和计时有效时返回 true
*/
static bool ReadBatch(size_t threads, size_t stride, bool control)
{
    LockFreeHashMap<int, int> map(4096);
    for (int key = 0; key < 4096; ++key)
    {
        if (!map.set(key, key + 1))
        {
            return false;
        }
    }
    const size_t iterations = 2000000;
    std::vector<int> outputs(threads * stride, 0);
    std::vector<uint64_t> failures(threads, 0);
    std::atomic<size_t> ready(0);
    std::atomic<bool> start(false);
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(threads);
    try
    {
        for (size_t thread = 0; thread < threads; ++thread)
        {
            workers.push_back(LockFreeHashMapTestThread([&, thread]()
            {
                // 本线程的原子对象在栈上，与其他线程没有共享写入
                LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<uint64_t> fence(0);
                int& output = outputs[thread * stride];
                uint32_t random = 123 + static_cast<uint32_t>(thread);
                uint64_t errors = 0;
                ready.fetch_add(1);
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                for (size_t i = 0; i < iterations; ++i)
                {
                    random = random * 1664525U + 1013904223U;
                    int key = static_cast<int>((random >> 8) & 4095);
                    if (control)
                    {
                        uint64_t zero = 0;
                        if (!fence.compare_exchange_strong(zero, 1))
                        {
                            ++errors;
                        }
                        output = key + 1;
                        fence.store(0, std::memory_order_release);
                    }
                    else if (!map.get(key, output))
                    {
                        ++errors;
                    }
                    if (output != key + 1)
                    {
                        ++errors;
                    }
                }
                failures[thread] = errors;
            }));
        }
    }
    catch (...)
    {
        start.store(true);
        throw;
    }
    while (ready.load() != threads)
    {
        std::this_thread::yield();
    }
    double begin = ReadClock();
    start.store(true);
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    double elapsed = ReadClock() - begin;
    uint64_t errors = 0;
    for (size_t i = 0; i < failures.size(); ++i)
    {
        errors += failures[i];
    }
    std::cout << "READ_COST threads=" << threads << " stride=" << stride << " control=" << control
        << " seconds=" << elapsed << " ops=" << double(iterations * threads) / elapsed
        << " errors=" << errors << std::endl;
    return elapsed > 0 && errors == 0 && LockFreeHashMapTestThread::failures() == 0;
}

/** 独立读取诊断入口；全部异常转为失败退出，线程先收拢再销毁夹具
@return 全部校验通过返回零
*/
int main()
{
    try
    {
        for (size_t round = 0; round < 3; ++round)
        {
            for (size_t threads = 1; threads <= 16; threads *= 4)
            {
                for (size_t stride = 1; stride <= 32; stride *= 32)
                {
                    if (!ReadBatch(threads, stride, false) || !ReadBatch(threads, stride, true))
                    {
                        return 1;
                    }
                }
            }
        }
        return 0;
    }
    catch (...)
    {
        std::cerr << "读取诊断发生异常" << std::endl;
        return 1;
    }
}