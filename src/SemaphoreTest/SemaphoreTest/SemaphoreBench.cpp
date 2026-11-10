#include "SemaphoreTest.h"
#include "Semaphore.h"
#include <Windows.h>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <thread>

/** 仅使用公共接口的历史性能烟测，保留原工作量
*/
int32_t SemaphoreBench()
{
    /** 读取性能测量的共享单调时钟
    @return 微秒
    */
    auto now = []() -> double
    {
        LARGE_INTEGER ticks, frequency;
        ::QueryPerformanceCounter(&ticks);
        ::QueryPerformanceFrequency(&frequency);
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
                        reply.signal();
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
                request.signal();
                if (!reply.wait(5000))
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
        if (failed.load())
        {
            return 1;
        }
        std::sort(latency, latency + 1000);
        const double started = now();
        for (int32_t index = 0; index < 50000; ++index)
        {
            request.signal();
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
            50000000000.0 / elapsed, latency[500], latency[950], latency[990]);
        return request.wait(0) ? 3 : 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}