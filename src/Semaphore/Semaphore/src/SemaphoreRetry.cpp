#include "SemaphoreRetry.h"
#include "SemaphoreConfig.h"
#include "SemaphoreWake.h"
#include <chrono>
#include <thread>
#include <system_error>
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#endif

SemaphoreResult SemaphoreRetry::capacity(uint32_t& attempts, uint32_t fastAttempts, uint32_t sleepMilliseconds)
{
    if (attempts < fastAttempts)
    {
        ++attempts;
        return SemaphoreResult();
    }
    if (!SemaphoreWake::canBlock())
    {
        return SemaphoreResult(SEMAPHORE_UNSUPPORTED);
    }
    try
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMilliseconds));
        return SemaphoreResult();
    }
    catch (const std::system_error& error)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR, false, error.code().value());
    }
    catch (...)
    {
        return SemaphoreResult(SEMAPHORE_SYSTEM_ERROR);
    }
}

void SemaphoreRetry::contention(uint32_t& attempts, const SemaphoreWaitPolicy& policy)
{
    // CAS竞争失败后短退避，减少同时争抢同一缓存行。
    // 这里只退让竞争失败，不用处理器指令等待许可；无许可仍进入系统阻塞。
#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
    const uint32_t requested = policy.m_initialPauses << (attempts < 10 ? attempts : 10);
    const uint32_t pauses = requested < policy.m_maximumPauses ? requested : policy.m_maximumPauses;
    for (uint32_t i = 0; i < pauses; ++i)
    {
#ifdef _MSC_VER
        _mm_pause();
#else
        __builtin_ia32_pause();
#endif
    }
#endif
    if (++attempts == policy.m_contentionYieldInterval)
    {
        attempts = 0;
        std::this_thread::yield();
    }
}