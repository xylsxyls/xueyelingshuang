#include "SemaphoreTest.h"
#include "Semaphore.h"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <thread>
#include <vector>

/** 检查真实DLL或静态边界复制的用户回调
*/
int32_t SemaphorePolicyTests()
{
    try
    {
        Semaphore sem;
        std::atomic<int32_t> growCalls(0);
        std::atomic<int32_t> shrinkCalls(0);
        {
            SemaphorePoolConfig config;
            config.initialCapacity = 0;
            config.maxCapacity = 3;
            std::vector<int32_t> capture(512, 7);

            /** 计算本用例期望的新增记录数，异常分支用于验证策略错误反馈
            @param [in] context 容量及增长历史快照
            @return 期望新增记录数，实际容量由库截断
            */
            config.grow = [capture, &growCalls](const SemaphoreGrowContext& context) -> size_t
            {
                if (capture[511] != 7 || context.initialCapacity != 0)
                {
                    throw std::runtime_error("growth copy");
                }
                ++growCalls;
                return 32;
            };
            /** 根据本用例的空闲条件提出缩容要求
            @param [in] context 空闲容量及增长历史快照
            @return 期望缩容量及连续空闲毫秒
            */
            config.shrink = [capture, &shrinkCalls](const SemaphoreShrinkContext& context) -> SemaphoreShrinkDecision
            {
                if (capture[511] != 7 || !context.hasLastGrow)
                {
                    throw std::runtime_error("shrink copy");
                }
                ++shrinkCalls;
                const SemaphoreShrinkDecision decision = {100, 1};
                return decision;
            };
            if (sem.configureEventPool(config).status != SEMAPHORE_OK)
            {
                return 1;
            }
        }
        // 调用方回调及捕获容器已经销毁，库持有独立副本
        if (sem.eventWaitResult(5).status != SEMAPHORE_TIMEOUT || growCalls.load() != 1 ||
            sem.eventPoolStatistics().capacity != 3)
        {
            return 2;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        size_t released = 0;
        if (sem.maintainEventPool(released).status != SEMAPHORE_OK || released != 3 ||
            sem.eventPoolStatistics().capacity != 0 || shrinkCalls.load() != 1)
        {
            return 3;
        }
        SemaphorePoolConfig failing;
        failing.initialCapacity = 0;
        /** 计算本用例期望的新增记录数，异常分支用于验证策略错误反馈
        @param [in] context 容量及增长历史快照
        @return 期望新增记录数，实际容量由库截断
        */
        failing.grow = [](const SemaphoreGrowContext& context) -> size_t
        {
            throw std::runtime_error("caller policy");
        };
        if (sem.configureEventPool(failing).status != SEMAPHORE_OK || sem.eventWaitResult(5).status != SEMAPHORE_POLICY)
        {
            return 4;
        }
        std::printf(
            "PASS id=52 copied callables, clamp, shrink, and caller exception across public library boundary\n");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL id=52 %s\n", error.what());
        return 5;
    }
}