#include "LockFreeMapTestThread.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <stdint.h>
#include <string>

/** 不链接被测容器的运行时对照，覆盖线程、堆增长、计时和异常边界
@param [in] argc 参数数量
@param [in] argv 唯一参数为报告文件名
@return 通过返回零，工作失败或异常返回一，参数错误返回二，报告失败返回三
*/
int main(int argc, char** argv)
{
    if (argc != 2 || *argv[1] == 0)
    {
        return 2;
    }
    FILE* report = std::fopen(argv[1], "wb");
    if (report == nullptr)
    {
        return 3;
    }
    std::atomic<unsigned> failures(0);
    std::atomic<unsigned> allocationFailures(0), workerExceptions(0), joinFailures(0);
    bool written = std::fprintf(report, "runtime_probe,no_map_linked=1\n") > 0;
    try
    {
        for (unsigned round = 0; round < 10; ++round)
        {
            std::vector<LockFreeMapTestThread> threads;
            LockFreeMapTestThreadGroup cleanup(threads);
            threads.reserve(16);
            for (unsigned worker = 0; worker < 16; ++worker)
            {
                threads.push_back(LockFreeMapTestThread([&failures, &allocationFailures, &workerExceptions, report, worker]()
                {
                    try
                    {
                        // 同时持有的缓冲区超过默认初始堆，触发真实共享内存增长
                        std::vector<unsigned> storage(512 * 1024, worker);
                        std::function<unsigned(unsigned)> transform = [](unsigned value) -> unsigned
                        {
                            return (value * 7 + 3) % 1009;
                        };
                        unsigned caught = 0;
                        for (unsigned iteration = 0; iteration < 10000; ++iteration)
                        {
                            const auto start = std::chrono::steady_clock::now();
                            std::vector<unsigned> values(32 + iteration % 64);
                            uint64_t expected = 0;
                            for (size_t index = 0; index < values.size(); ++index)
                            {
                                const unsigned input = static_cast<unsigned>(index) + iteration + worker;
                                values[index] = transform(input);
                                expected += (input * 7 + 3) % 1009;
                            }
                            std::sort(values.begin(), values.end());
                            uint64_t actual = 0;
                            for (size_t index = 0; index < values.size(); ++index)
                            {
                                actual += values[index];
                            }
                            const std::string text(64 + iteration % 31, static_cast<char>('a' + worker));
                            const std::string copy(text);
                            if (actual != expected || copy != text || storage[iteration % storage.size()] != worker ||
                                std::chrono::steady_clock::now() < start)
                            {
                                failures.fetch_add(1);
                            }
                            if (iteration % 1000 == 0)
                            {
                                try
                                {
                                    throw std::runtime_error("runtime probe exception");
                                }
                                catch (const std::runtime_error&)
                                {
                                    ++caught;
                                }
                            }
                        }
                        if (caught != 10)
                        {
                            failures.fetch_add(1);
                        }
                    }
                    catch (const std::bad_alloc&)
                    {
                        allocationFailures.fetch_add(1);
                    }
                    catch (const std::exception& error)
                    {
                        std::fprintf(report, "runtime_exception,worker=%u,message=%s\n", worker, error.what());
                        workerExceptions.fetch_add(1);
                    }
                    catch (...)
                    {
                        std::fprintf(report, "runtime_exception,worker=%u,type=unknown\n", worker);
                        workerExceptions.fetch_add(1);
                    }
                }));
            }
            for (size_t index = 0; index < threads.size(); ++index)
            {
                if (!threads[index].join())
                {
                    joinFailures.fetch_add(1);
                }
            }
            written = std::fprintf(report, "round,%u,errors,%u\n", round + 1, failures.load()) > 0 && written;
            written = std::fflush(report) == 0 && written;
        }
    }
    catch (...)
    {
        failures.fetch_add(1);
    }
    const unsigned errors = failures.load() + allocationFailures.load() + workerExceptions.load() + joinFailures.load() +
        (LockFreeMapTestThread::m_errors.load() != 0 ? 1 : 0);
    written = std::fprintf(report, "runtime_counts,allocation_failures=%u,worker_exceptions=%u,join_failures=%u,thread_errors=%llu\n",
        allocationFailures.load(), workerExceptions.load(), joinFailures.load(),
        static_cast<unsigned long long>(LockFreeMapTestThread::m_errors.load())) > 0 && written;
    written = std::fprintf(report, "failures,%u\n", errors) > 0 && written;
    written = std::ferror(report) == 0 && written;
    written = std::fclose(report) == 0 && written;
    return !written ? 3 : errors == 0 ? 0 : 1;
}