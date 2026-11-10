#include "SemaphoreTest.h"
#include "Semaphore.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include "LockFreeHashMap/LockFreeHashMapAPI.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

/** 只读使用其他三库的当前公开头进行集成验证
*/
int32_t SemaphoreQueueTests()
{
    try
    {
        LockFreeQueue<int32_t> queue;
        LockFreeMap<int32_t, int32_t> ordered;
        LockFreeHashMap<int32_t, int32_t> hashed;
        if (!ordered.empty() || !hashed.empty())
        {
            std::fprintf(stderr, "FAIL id=47 both map templates must start empty\n");
            return 1;
        }
        Semaphore wake;
        std::atomic<int32_t> errors(0);
        std::atomic<int32_t> retries(0);
        std::vector<unsigned char> seen(8000, 0);
        std::vector<std::thread> producers;
        producers.reserve(4);
        try
        {
            for (int32_t producer = 0; producer < 4; ++producer)
            {
                producers.emplace_back(
                    /** 执行当前测试的线程步骤，捕获状态必须活到线程回收
                    */
                    [&, producer]()
                    {
                        try
                        {
                            for (int32_t index = 0; index < 2000; ++index)
                            {
                                const int32_t value = producer * 2000 + index;
                                int32_t attempts = 0;
                                for (;;)
                                {
                                    const LockFreeQueueResult accepted = queue.tryPush(value);
                                    if (accepted == LockFreeQueueSuccess)
                                    {
                                        break;
                                    }
                                    if (accepted != LockFreeQueueTemporaryUnavailable || ++attempts > 5000)
                                    {
                                        throw LockFreeQueueError(accepted);
                                    }
                                    retries.fetch_add(1);
                                    // 测试调用方背压，未成功提交的队列项不发布许可
                                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                }
                                wake.signal();
                            }
                        }
                        catch (...)
                        {
                            errors.fetch_add(1);
                        }
                    });
            }
        }
        catch (...)
        {
            errors.fetch_add(1);
        }
        int32_t consumed = 0;
        try
        {
            while (consumed < 8000 && wake.wait(5000))
            {
                int32_t value = -1;
                if (!queue.pop(&value) || value < 0 || value >= 8000 || seen[value] != 0)
                {
                    errors.fetch_add(1);
                    break;
                }
                seen[value] = 1;
                ++consumed;
            }
        }
        catch (...)
        {
            errors.fetch_add(1);
        }
        for (size_t index = 0; index < producers.size(); ++index)
        {
            producers[index].join();
        }
        if (consumed != 8000 || errors.load() != 0 || wake.wait(0) || !queue.empty())
        {
            std::printf("FAIL id=47 consumed=%d errors=%d\n", consumed, errors.load());
            return 1;
        }
        std::printf("PASS id=47 four headers; true MPSC Queue; committed=8000 consumed=8000 retry_uncommitted=%d\n",
            retries.load());
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL id=47 %s\n", error.what());
        return 1;
    }
}