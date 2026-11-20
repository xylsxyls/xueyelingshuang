#include "SemaphoreTest.h"
#include "Semaphore.h"
#include <Windows.h>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <thread>

/** 用公共接口执行固定工作量的往返和积存烟测
*/
int32_t SemaphoreBench()
{
    /** 读取性能测量的共享单调时钟
    @return 微秒
    */
    auto now = []() -> double
    {
        LARGE_INTEGER ticks, frequency;
        if (!::QueryPerformanceCounter(&ticks) || !::QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
        {
            throw std::runtime_error("QPC unavailable");
        }
        return static_cast<double>(ticks.QuadPart) * 1000000.0 / frequency.QuadPart;
    };
    try
    {
        Semaphore request;
        Semaphore reply;
        double latency[1000] = {};
        std::atomic<bool> failed(false);
        std::thread worker(
            /** 执行当前测试的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                try
                {
                    for (int32_t index = 0; index < 1000; ++index)
                    {
                        if (!request.wait(5000))
                        {
                            failed.store(true);
                            break;
                        }
                        if (!reply.signal())
                        {
                            failed.store(true);
                            return;
                        }
                    }
                }
                catch (...)
                {
                    failed.store(true);
                }
            });
        try
        {
            for (int32_t index = 0; index < 1000; ++index)
            {
                const double started = now();
                if (!request.signal() || !reply.wait(5000))
                {
                    failed.store(true);
                    break;
                }
                latency[index] = now() - started;
            }
        }
        catch (...)
        {
            failed.store(true);
        }
        worker.join();
        if (failed.load() || request.wait(0) || reply.wait(0))
        {
            return 1;
        }
        std::sort(latency, latency + 1000);
        const double started = now();
        for (int32_t index = 0; index < 50000; ++index)
        {
            if (!request.signal())
            {
                return 1;
            }
        }
        for (int32_t index = 0; index < 50000; ++index)
        {
            if (!request.wait(0))
            {
                return 2;
            }
        }
        const double elapsed = now() - started;
        std::printf("METRIC bulk_permits_per_second=%.0f roundtrip_us p50=%.3f p95=%.3f p99=%.3f\n",
            50000000000.0 / elapsed, latency[499], latency[949], latency[989]);
        return request.wait(0) ? 3 : 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}