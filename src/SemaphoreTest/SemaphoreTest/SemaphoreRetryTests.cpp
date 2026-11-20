#include "SemaphoreRetryTests.h"
#include "Semaphore.h"
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
#include "SemaphoreRetry.h"
#include "SemaphoreEventPool.h"
#include "SemaphoreTestHooks.h"
#endif
#include <atomic>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <vector>
#ifdef _WIN32
#include <Windows.h>
#else
#include "SemaphoreProcessMemory.h"
#include <unistd.h>
#endif

#ifdef SEMAPHORE_TEST_INSTRUMENTATION
// 发布者记录首次退避的发布序号，再用休眠时间通知观察线程。
static std::atomic<uint32_t> s_firstSleepAttempt(0);
static std::atomic<uint32_t> s_firstSleepMilliseconds(0);

void SemaphoreTestObserveCapacitySleep(uint32_t milliseconds)
{
    if (s_firstSleepMilliseconds.load() == 0)
    {
        s_firstSleepAttempt.store(SemaphoreTestPublishAttempts());
        s_firstSleepMilliseconds.store(milliseconds);
    }
}
#endif

SemaphoreRetryJoin::SemaphoreRetryJoin(std::thread& thread) :
m_thread(&thread)
{

}

SemaphoreRetryJoin::~SemaphoreRetryJoin()
{
    if (m_thread->joinable())
    {
        try
        {
            m_thread->join();
        }
        catch (...)
        {
            // 测试调用方只能拥有一次join；保留错误给外层看门狗报告。
        }
    }
}

bool SemaphoreRetryTests::capacity(int32_t mode)
{
#if (defined(_WIN32) && !defined(SEMAPHORE_TEST_POSIX_NAME)) || defined(SEMAPHORE_SHARED_COUNTER)
    std::ostringstream name;
#ifdef _WIN32
    const unsigned long pid = ::GetCurrentProcessId();
#else
    const unsigned long pid = static_cast<unsigned long>(::getpid());
#endif
    name << "sem-retry-" << pid << '-' << mode;
    SemaphoreConfig config;
    if (mode == 4)
    {
        SemaphoreWaitPolicy policy;
        policy.m_capacityAttempts = 3;
        policy.m_capacitySleep = 7;
        if (config.setWaitPolicy(policy).status != SEMAPHORE_OK)
        {
            return false;
        }
    }
    Semaphore semaphore(config);
    if (semaphore.createProcessResult(name.str(), 1).status != SEMAPHORE_OK ||
        !semaphore.tryProcessSignalResult().committed)
    {
        return false;
    }
    const bool waitCapacity = mode == 4 || (mode & 2) != 0;
    if (!waitCapacity)
    {
        const SemaphoreResult full = semaphore.tryProcessSignalResult((mode & 1) != 0, false);
        return full.status == SEMAPHORE_OVERFLOW && !full.committed &&
            semaphore.processWait(0) && !semaphore.processWait(0);
    }
    // 普通发布还须穿过实例配置的快速重试次数，进入首次休眠后才释放容量。
    std::atomic<bool> started(false);
    std::atomic<bool> done(false);
    std::atomic<bool> threw(false);
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
    SemaphoreTestPublishAttempts(true);
    s_firstSleepMilliseconds.store(0);
    s_firstSleepAttempt.store(0);
#endif
    SemaphoreResult result(SEMAPHORE_INVALID);
    /** 在满载时按当前模式发布，记录正常结果或异常并最终发布完成标记
    */
    std::thread producer([&]()
    {
        started.store(true);
        try
        {
            result = mode == 4 ? SemaphoreResult(SEMAPHORE_OK, semaphore.processSignal()) :
                semaphore.tryProcessSignalResult((mode & 1) != 0, waitCapacity);
        }
        catch (...)
        {
            threw.store(true);
        }
        done.store(true);
    });
    SemaphoreRetryJoin join(producer);
    while (!started.load())
    {
        std::this_thread::yield();
    }
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!done.load() && std::chrono::steady_clock::now() < deadline &&
        (mode == 4 ? s_firstSleepMilliseconds.load() == 0 : SemaphoreTestPublishAttempts() < 2))
    {
        std::this_thread::yield();
    }
    const bool retried = mode == 4 ? s_firstSleepMilliseconds.load() == 7 && s_firstSleepAttempt.load() == 4 :
        SemaphoreTestPublishAttempts() >= 2;
#else
    const bool retried = true; // DLL只验证公开入口完成；内部重试次数由单元构建验证。
#endif
    const bool early = done.load();
    const bool consumed = semaphore.processWait(0);
    producer.join();
    return !threw.load() && retried && consumed && !early && result.status == SEMAPHORE_OK && result.committed &&
        semaphore.processWait(0) && !semaphore.processWait(0);
#else
    (void)mode;
    return true;
#endif
}

bool SemaphoreRetryTests::run()
{
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
    uint32_t attempts = 0;
    for (uint32_t i = 0; i < 4096; ++i)
    {
        if (SemaphoreRetry::capacity(attempts, 4096, 1).status != SEMAPHORE_OK || attempts != i + 1)
        {
            return false;
        }
    }
    for (int32_t i = 0; i < 8; ++i)
    {
        const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        if (SemaphoreRetry::capacity(attempts, 4096, 1).status != SEMAPHORE_OK || attempts != 4096 ||
            std::chrono::steady_clock::now() - begin < std::chrono::milliseconds(1))
        {
            return false;
        }
    }
#endif
    for (int32_t mode = 0; mode < 5; ++mode)
    {
        if (!capacity(mode))
        {
            return false;
        }
    }
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
    SemaphorePoolConfig poolConfig;
    poolConfig.initialCapacity = 1;
    poolConfig.freeThreshold = 0;
    /** 保留当前容量，拒绝新增记录但允许归还后复用
    @param [in] context 本次扩容前的容量快照
    @return 0不增加容量
    */
    poolConfig.grow = [](const SemaphoreGrowContext& context) -> size_t
    {
        (void)context;
        return 0;
    };
    SemaphoreEventPool pool(poolConfig);
    SemaphoreEventRecord* held = nullptr;
    SemaphoreEventRecord* next = nullptr;
    if (pool.begin(held).status != SEMAPHORE_OK)
    {
        return false;
    }
    const SemaphoreResult full = pool.begin(next);
    pool.finish(held);
    if (full.status != SEMAPHORE_CAPACITY || next != nullptr ||
        pool.begin(next).status != SEMAPHORE_OK)
    {
        return false;
    }
    pool.finish(next);
#endif
    Semaphore closed;
    if (closed.tryProcessSignalResult(true, true).status != SEMAPHORE_NOT_OPEN)
    {
        return false;
    }
    Semaphore count;
    std::atomic<bool> failed(false);
    std::vector<std::thread> producers;
    try
    {
        producers.reserve(4);
        for (int32_t p = 0; p < 4; ++p)
        {
            /** 发布一万个许可，任何失败都记录到共享标记；捕获对象活到线程回收
            */
            producers.push_back(std::thread([&]()
            {
                try
                {
                    for (int32_t i = 0; i < 10000; ++i)
                    {
                        if (!count.trySignal(true, true).committed)
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
            }));
        }
        for (int32_t i = 0; i < 40000; ++i)
        {
            if (!count.wait(5000))
            {
                failed.store(true);
                break;
            }
        }
    }
    catch (...)
    {
        failed.store(true);
    }
    for (size_t i = 0; i < producers.size(); ++i)
    {
        if (producers[i].joinable())
        {
            producers[i].join();
        }
    }
    return !failed.load() && !count.wait(0);
}