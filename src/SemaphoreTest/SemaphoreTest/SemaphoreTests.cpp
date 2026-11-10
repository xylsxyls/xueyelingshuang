#include "SemaphoreTests.h"
#include "SemaphoreTestHooks.h"
#include "SemaphoreTestProcess.h"
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
#include "SemaphoreCount.h"
#endif
#if defined(_WIN32) && defined(PROCESS_WORK_PORTABLE_NOTIFY)
#include "SemaphoreTestProcessMemory.h"
#else
#include "SemaphoreProcessMemory.h"
#endif
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <system_error>
#include <limits>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <signal.h>
#include <spawn.h>
extern char** environ;
#endif

static std::atomic<SemaphoreTestGate*> s_gate(nullptr);
static std::atomic<int32_t> s_faultPoint(0);
static std::atomic<int32_t> s_faultCountdown(0);
static std::atomic<int32_t> s_registrations(0);
static Semaphore* s_seedObject = nullptr;
static uint64_t s_seedValue = 0;

void SemaphoreTestBackend(bool addressWait)
{
    static std::atomic<bool> s_reported(false);
    if (!s_reported.exchange(true))
    {
        std::printf("BACKEND %s\n", addressWait ? "WaitOnAddress" : "manual-reset-event");
    }
}

#ifdef SEMAPHORE_TEST_INSTRUMENTATION
static std::atomic<int32_t> s_allocationCountdown(0);
static std::atomic<int32_t> s_allocationCalls(0);
// 真实成功申请减释放次数，用于反复构造、池销毁和映射收尾的回落检查
static std::atomic<int32_t> s_liveAllocations(0);

void* operator new(size_t size)
{
    s_allocationCalls.fetch_add(1);
    if (s_allocationCountdown.load() > 0 && s_allocationCountdown.fetch_sub(1) == 1)
    {
        throw std::bad_alloc();
    }
    void* result = std::malloc(size == 0 ? 1 : size);
    if (result == nullptr)
    {
        throw std::bad_alloc();
    }
    s_liveAllocations.fetch_add(1);
    return result;
}

void operator delete(void* value) throw()
{
    if (value != nullptr)
    {
        s_liveAllocations.fetch_sub(1);
    }
    std::free(value);
}

void* operator new[](size_t size)
{
    return ::operator new(size);
}

void operator delete[](void* value) throw()
{
    ::operator delete(value);
}
#endif

int32_t SemaphoreTestFault(int32_t point)
{
    if (s_faultPoint.load() == point && s_faultCountdown.fetch_sub(1) == 1)
    {
        return 123;
    }
    return 0;
}

void SemaphoreTestHook(int32_t point, Semaphore* object)
{
    if (point == 1)
    {
        s_registrations.fetch_add(1);
    }
    SemaphoreTestGate* gate = s_gate.load();
    if (gate != nullptr)
    {
        gate->hit(point);
    }
}

void SemaphoreTestPoolHook(int32_t point, SemaphoreEventPool* pool)
{
    SemaphoreTestGate* gate = s_gate.load();
    if (gate != nullptr)
    {
        gate->hit(point + 100);
    }
}

void SemaphoreTestWakeHook()
{
    SemaphoreTestGate* gate = s_gate.load();
    if (gate != nullptr)
    {
        gate->hit(6);
    }
}

bool SemaphoreTestCountSeed(Semaphore* object, uint64_t& value)
{
    value = s_seedValue;
    return object == s_seedObject;
}

SemaphoreTestThreads::SemaphoreTestThreads() :
m_errors(0)
{
    m_threads.reserve(32);
}

SemaphoreTestThreads::~SemaphoreTestThreads()
{
    for (size_t index = 0; index < m_threads.size(); ++index)
    {
        if (m_threads[index].joinable())
        {
            m_threads[index].join();
        }
    }
}

void SemaphoreTestThreads::start(const std::function<void()>& task)
{
    m_threads.emplace_back(
        /** 执行当前测试的线程步骤，捕获状态必须活到线程回收
        */
        [this, task]()
        {
            try
            {
                task();
            }
            catch (const std::exception& error)
            {
                std::fprintf(stderr, "WORKER: %s\n", error.what());
                m_errors.fetch_add(1);
            }
            catch (...)
            {
                m_errors.fetch_add(1);
            }
        });
}

void SemaphoreTestThreads::join()
{
    for (size_t index = 0; index < m_threads.size(); ++index)
    {
        if (m_threads[index].joinable())
        {
            m_threads[index].join();
        }
    }
    SemaphoreTests::check(m_errors.load() == 0, "worker exception");
}

SemaphoreTestGate::SemaphoreTestGate(int32_t point) :
m_point(point),
m_arrived(false),
m_released(false)
{
    s_gate.store(this);
}

SemaphoreTestGate::~SemaphoreTestGate()
{
    release();
    s_gate.store(nullptr);
}

void SemaphoreTestGate::hit(int32_t point)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (point != m_point || m_arrived || m_released)
    {
        return;
    }
    m_arrived = true;
    m_changed.notify_all();
    m_changed.wait_for(lock, std::chrono::seconds(5),
        /** 检查暂停门的受保护状态
                       @return 本轮等待条件是否满足
                       */
        [this]()
        {
            return m_released;
        });
}

bool SemaphoreTestGate::reached()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_changed.wait_for(lock, std::chrono::seconds(5),
        /** 检查暂停门的受保护状态
                              @return 本轮等待条件是否满足
                              */
        [this]()
        {
            return m_arrived;
        });
}

void SemaphoreTestGate::release()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_released = true;
    m_changed.notify_all();
}

void SemaphoreTests::check(bool ok, const char* text)
{
    if (!ok)
    {
        throw std::runtime_error(text);
    }
}

double SemaphoreTests::now()
{
#ifdef _WIN32
    LARGE_INTEGER ticks;
    LARGE_INTEGER frequency;
    if (!::QueryPerformanceCounter(&ticks) || !::QueryPerformanceFrequency(&frequency))
    {
        throw std::runtime_error("QPC unavailable");
    }
    return static_cast<double>(ticks.QuadPart) * 1000.0 / frequency.QuadPart;
#else
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

void SemaphoreTests::pause(int32_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

std::string SemaphoreTests::name(int32_t suffix)
{
    char buffer[64];
#ifdef _WIN32
    const unsigned long pid = ::GetCurrentProcessId();
#else
    const unsigned long pid = static_cast<unsigned long>(::getpid());
#endif
    std::sprintf(buffer, "SPLF%lu_%d", pid, suffix);
    return buffer;
}

int32_t SemaphoreTests::peer(const std::string& logicalName, bool exchange)
{
    try
    {
        Semaphore request;
        Semaphore response;
        check(request.openProcessResult(logicalName).status == SEMAPHORE_OK, "child open request");
        check(response.openProcessResult(logicalName + "r").status == SEMAPHORE_OK, "child open response");
        check(response.processSignalResult().committed, "child ready");
        check(request.processWaitResult(-1).committed, "child infinite request");
        if (exchange)
        {
            for (int32_t i = 0; i < 199; ++i)
            {
                const SemaphoreSignalWaitResult result = response.processSignalAndWaitResult(request, 5000);
                check(result.status == SEMAPHORE_OK && result.signalState == SEMAPHORE_COMMIT_DONE &&
                        result.waitState == SEMAPHORE_COMMIT_DONE,
                    "child combined reply and next request");
            }
        }
        check(response.processSignalResult().committed, "child reply");
        check(request.closeProcessResult().status == SEMAPHORE_OK, "child close request");
        check(response.closeProcessResult().status == SEMAPHORE_OK, "child close reply");
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "PEER: %s\n", error.what());
        return 1;
    }
}

bool SemaphoreTests::runCase(int32_t id)
{
    if (id >= 82 && id <= 86 && id != 85)
    {
        return fairness(id);
    }
    if (id == 85)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        const int32_t liveBefore = s_liveAllocations.load();
        {
            for (int32_t boundary = 0; boundary < 2; ++boundary)
            {
                SemaphoreCount pool;
                SemaphoreWake wake;
                std::atomic<uint64_t> count(boundary == 0 ? 0 : (UINT64_MAX >> 1));
                s_faultPoint.store(2);
                s_faultCountdown.store(1);
                const SemaphoreResult published = pool.signal(count, wake);
                s_faultPoint.store(0);
                check(published.status == SEMAPHORE_SYSTEM_ERROR && published.committed,
                    "slow count accumulation still notifies competitive waiters and preserves commit on hint failure");
                SemaphoreEventRecord* unused = nullptr;
                check(pool.begin(count, unused, true).committed && unused == nullptr,
                    "hint failure leaves the accumulated permit consumable");
                pool.clear(count);
            }
            Semaphore sem;
            s_seedObject = &sem;
            s_seedValue = (UINT64_MAX >> 1) - 1;
            sem.reset();
            s_seedObject = nullptr;
            check(sem.trySignal().committed && sem.trySignal().committed, "direct-to-large count transition");
            check(sem.wait(0) && sem.wait(0), "large-to-direct count transition");
            s_seedObject = &sem;
            s_seedValue = UINT64_MAX;
            sem.reset();
            s_seedObject = nullptr;
            check(sem.trySignal().status == SEMAPHORE_OVERFLOW, "large count cannot overflow");
            check(sem.wait(0) && sem.trySignal().committed, "full range remains usable after consume");
            sem.reset();
            size_t released = 0;
            check(sem.maintainCountPool(released).status == SEMAPHORE_OK, "large record maintenance");
            const SemaphorePoolStats empty = sem.countPoolStatistics();
            check(empty.freeCapacity == empty.capacity && empty.retiredCapacity == 0, "large records reclaimed");
            SemaphorePoolConfig blocked;
            blocked.initialCapacity = 0;
            blocked.maxCapacity = 0;
            check(sem.configureCountPool(blocked).status == SEMAPHORE_OK, "bounded count pool configuration");
            s_seedObject = &sem;
            s_seedValue = UINT64_MAX >> 1;
            sem.reset();
            s_seedObject = nullptr;
            const SemaphoreResult denied = sem.trySignal();
            check(denied.status == SEMAPHORE_CAPACITY && !denied.committed && sem.wait(0),
                "large transition failure preserves existing count");
            sem.reset();
            Semaphore allocation;
            s_allocationCountdown.store(1);
            const SemaphoreResult shortWait = allocation.waitResult(1);
            const SemaphoreResult failed = allocation.waitResult(100);
            s_allocationCountdown.store(0);
            check(shortWait.status == SEMAPHORE_TIMEOUT && failed.status == SEMAPHORE_NO_MEMORY &&
                !failed.committed && allocation.countPoolStatistics().capacity == 0,
                "count pool allocation failure observable without partial publication");
            check(allocation.waitResult(100).status == SEMAPHORE_TIMEOUT && allocation.countPoolStatistics().capacity == 16,
                "count pool allocation retry after sustained waiting");
        }
        check(s_liveAllocations.load() == liveBefore, "count records and failed candidates release every allocation");
        return true;
#else
        return false;
#endif
    }
    if (id == 25)
    {
        Semaphore sem;
        check(sem.waitResult(0).status == SEMAPHORE_TIMEOUT, "empty probe");
        for (int32_t index = 0; index < 1000; ++index)
        {
            sem.signal();
        }
        for (int32_t index = 0; index < 1000; ++index)
        {
            check(sem.wait(0), "stored permit");
        }
        check(!sem.wait(0), "no extra permit");
        sem.signal();
        sem.reset();
        check(!sem.wait(0), "reset");
        check(!sem.eventWait(0) && !sem.eventWait(-1), "event nonpositive is immediate");
    }
    else if (id == 26)
    {
        Semaphore sem;
        SemaphoreTestThreads threads;
        threads.start(
            /** 执行固定超时预算的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                for (int32_t i = 0; i < 250; ++i)
                {
                    sem.eventAll();
                    pause(1);
                }
            });
        const double started = now();
        check(!sem.wait(120), "irrelevant events cannot consume count");
        const double elapsed = now() - started;
        threads.join();
        std::printf("METRIC budget_ms=%.3f\n", elapsed);
        check(elapsed >= 100 && elapsed < 600, "fixed timeout budget");
    }
    else if (id == 27)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        Semaphore sem;
        s_seedObject = &sem;
        s_seedValue = UINT64_MAX;
        sem.reset();
        check(sem.trySignal().status == SEMAPHORE_OVERFLOW, "64-bit overflow result");
        bool threw = false;
        try
        {
            sem.signal();
        }
        catch (const std::overflow_error&)
        {
            threw = true;
        }
        check(threw, "legacy overflow exception");
        check(sem.wait(0) && sem.trySignal().committed, "overflow unchanged count");
        s_seedValue = uint64_t(1) << 32;
        sem.reset();
        check(sem.wait(0), "count exceeds 32 bits");
        s_seedValue = 0;
        sem.reset();
        s_seedObject = nullptr;
#else
        return false;
#endif
    }
    else if (id == 28)
    {
        Semaphore sem;
        std::atomic<int32_t> consumed(0);
        SemaphoreTestThreads threads;
        const double started = now();
        for (int32_t i = 0; i < 4; ++i)
        {
            threads.start(
                /** 执行多生产多消费的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    for (int32_t n = 0; n < 5000; ++n)
                    {
                        check(sem.wait(5000), "MPMC lost permit");
                        consumed.fetch_add(1);
                    }
                });
            threads.start(
                /** 执行多生产多消费的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    for (int32_t n = 0; n < 5000; ++n)
                    {
                        sem.signal();
                    }
                });
        }
        threads.join();
        check(consumed.load() == 20000 && !sem.wait(0), "MPMC exact accounting");
        std::printf("METRIC permits_per_second=%.0f\n", 20000000.0 / (now() - started));

        // 每人只消费一次，确保连续通知能覆盖全部接收者，退出通知不能只靠同一线程反复消费。
        Semaphore ready;
        Semaphore oneShot;
        std::atomic<int32_t> completed(0);
        SemaphoreTestThreads receivers;
        for (int32_t i = 0; i < 16; ++i)
        {
            receivers.start(
                /** 消费本线程的一次退出通知；有限等待保证测试失败也可回收线程
                */
                [&]()
                {
                    ready.signal();
                    check(oneShot.wait(2000), "one-shot receiver permit");
                    completed.fetch_add(1);
                });
        }
        for (int32_t i = 0; i < 16; ++i)
        {
            check(ready.wait(2000), "one-shot receiver ready");
        }
        pause(5);
        for (int32_t i = 0; i < 16; ++i)
        {
            oneShot.signal();
        }
        receivers.join();
        check(completed.load() == 16 && !oneShot.wait(0), "one notification for each receiver");
    }
    else if (id == 29)
    {
        Semaphore request;
        Semaphore reply;
        int32_t payload = 0;
        std::vector<double> latencies;
        latencies.reserve(1000);
        SemaphoreTestThreads threads;
        threads.start(
            /** 执行发布获取可见性的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                for (int32_t n = 1; n <= 1000; ++n)
                {
                    check(request.wait(5000), "publication wait");
                    check(payload == n, "ordinary payload visibility");
                    reply.signal();
                }
            });
        for (int32_t n = 1; n <= 1000; ++n)
        {
            payload = n;
            const double started = now();
            request.signal();
            check(reply.wait(5000), "reply");
            latencies.push_back((now() - started) * 1000);
        }
        threads.join();
        std::sort(latencies.begin(), latencies.end());
        std::printf("METRIC roundtrip_us p50=%.3f p95=%.3f p99=%.3f\n", latencies[500], latencies[950], latencies[990]);
    }
    else if (id == 30 || id == 63)
    {
        Semaphore sem;
        if (id == 63)
        {
            check(sem.prepareSignalAndWait().status == SEMAPHORE_OK, "prepare preserves event contract");
        }
        sem.event();
        sem.eventAll();
        check(!sem.eventWait(15), "early events never accumulate");
        std::atomic<int32_t> done(0);
        SemaphoreTestThreads threads;
        for (int32_t i = 0; i < 4; ++i)
        {
            threads.start(
                /** 执行预备之后事件兼容的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    if (sem.eventWait(2000))
                    {
                        done.fetch_add(1);
                    }
                });
        }
        pause(100);
        sem.event();
        pause(80);
        const int32_t single = done.load();
        sem.eventAll();
        threads.join();
        check(single == 1 && done.load() == 4, "event one and eventAll remaining");
        check(!sem.eventWait(15) && !sem.wait(0), "events leave no late permit");
    }
    else if (id == 31)
    {
        Semaphore sem;
        sem.signal();
        check(!sem.eventWait(15), "count not consumed by event waiter");
        check(sem.wait(0), "count retained");
        sem.eventAll();
        check(!sem.wait(15), "event not a count permit");
    }
    else if (id == 32)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        for (int32_t point = 1; point <= 7; ++point)
        {
            Semaphore sem;
            SemaphoreTestGate gate(point);
            std::atomic<bool> ok(false);
            SemaphoreTestThreads threads;
            if (point == 4)
            {
                threads.start(
                    /** 执行登记睡眠边界的线程步骤，捕获状态必须活到线程回收
                    */
                    [&]()
                    {
                        sem.signal();
                    });
                const bool arrived = gate.reached();
                ok.store(sem.wait(100));
                gate.release();
                threads.join();
                check(arrived && ok.load() && !sem.wait(0), "paused publisher after commit");
            }
            else
            {
                threads.start(
                    /** 执行登记睡眠边界的线程步骤，捕获状态必须活到线程回收
                    */
                    [&]()
                    {
                        ok.store(sem.wait(point == 5 ? 0 : 500));
                    });
                const bool arrived = gate.reached();
                sem.signal();
                gate.release();
                threads.join();
                check(arrived && ok.load() && !sem.wait(0), "check/sleep/timeout boundary");
            }
        }
#else
        return false;
#endif
    }
    else if (id == 33)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        SemaphorePoolConfig config;
        SemaphoreEventPool pool(config);
        SemaphoreEventRecord* early = nullptr;
        SemaphoreEventRecord* late = nullptr;
        check(pool.begin(early).status == SEMAPHORE_OK, "early registration");
        SemaphoreTestGate gate(101);
        SemaphoreTestThreads threads;
        threads.start(
            /** 执行广播快照集合的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                pool.notify(true);
            });
        const bool arrived = gate.reached();
        check(pool.begin(late).status == SEMAPHORE_OK, "late registration");
        gate.release();
        threads.join();
        check(arrived && pool.finish(early) && !pool.finish(late), "broadcast excludes late registration");
        check(pool.begin(early).status == SEMAPHORE_OK, "first event waiter");
        check(pool.notify(false).committed, "first event");
        check(pool.begin(late).status == SEMAPHORE_OK, "second event waiter");
        check(pool.notify(false).committed, "second event");
        check(pool.finish(late) && pool.finish(early), "late cannot steal old event");
#else
        return false;
#endif
    }
    else if (id == 34)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        SemaphorePoolConfig config;
        config.initialCapacity = 1;
        config.maxCapacity = 3;
        std::atomic<int32_t> calls(0);

        /** 计算本用例期望的新增记录数，异常分支用于验证策略错误反馈
        @param [in] context 容量及增长历史快照
        @return 期望新增记录数，实际容量由库截断
        */
        config.grow = [&](const SemaphoreGrowContext& context) -> size_t
        {
            ++calls;
            check(context.initialCapacity == 1, "growth context");
            return 10;
        };
        SemaphoreEventPool pool(config);
        SemaphoreEventRecord* a = nullptr;
        SemaphoreEventRecord* b = nullptr;
        SemaphoreEventRecord* c = nullptr;
        SemaphoreEventRecord* rejected = nullptr;
        check(pool.begin(a).status == SEMAPHORE_OK && calls.load() == 0, "threshold zero last node");
        check(pool.begin(b).status == SEMAPHORE_OK && calls.load() == 1, "next demand grows");
        check(pool.begin(c).status == SEMAPHORE_OK && pool.statistics().capacity == 3, "growth clamped");
        check(pool.begin(rejected).status == SEMAPHORE_CAPACITY, "finite full rejection");
        pool.finish(a);
        pool.finish(b);
        pool.finish(c);
        check(pool.begin(a).status == SEMAPHORE_OK, "full capacity still reusable");
        pool.finish(a);
        config.freeThreshold = 1;
        /** 计算本用例期望的新增记录数，异常分支用于验证策略错误反馈
        @param [in] context 容量及增长历史快照
        @return 期望新增记录数，实际容量由库截断
        */
        config.grow = [](const SemaphoreGrowContext& context) -> size_t
        {
            throw std::runtime_error("policy");
        };
        SemaphoreEventPool failing(config);
        check(failing.begin(a).status == SEMAPHORE_OK, "pre-grow error retains acquired node");
        check(failing.statistics().lastGrowStatus == SEMAPHORE_POLICY, "pre-grow error observable");
        check(failing.begin(b).status == SEMAPHORE_POLICY, "empty policy error");
        failing.finish(a);
        Semaphore sem;
        config.maxCapacity = -2;
        check(sem.configureEventPool(config).status == SEMAPHORE_INVALID, "invalid negative maximum");
        config.maxCapacity = 0;
        check(sem.configureEventPool(config).status == SEMAPHORE_INVALID, "maximum below initial");
        config.maxCapacity = -1;
        config.initialCapacity = (std::numeric_limits<size_t>::max)();
        check(sem.configureEventPool(config).status == SEMAPHORE_INVALID, "capacity byte overflow");
#else
        return false;
#endif
    }
    else if (id == 35)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        Semaphore sem;
        SemaphorePoolConfig config;
        config.initialCapacity = 4;
        check(sem.configureEventPool(SemaphorePoolConfig()).status == SEMAPHORE_OK, "prepare prior pool for rollback");
        for (int32_t failure = 1; failure <= 4; ++failure)
        {
            s_faultCountdown.store(failure);
            s_faultPoint.store(4);
            const SemaphoreResult result = sem.configureEventPool(config);
            s_faultPoint.store(0);
            check(result.status == SEMAPHORE_NO_MEMORY, "each initial record allocation failure");
            check(sem.eventPoolStatistics().capacity == 16, "failed configure preserves old pool");
        }
        config.initialCapacity = 0;
        config.maxCapacity = 4;
        SemaphoreEventPool pool(config);
        for (int32_t failure = 1; failure <= 4; ++failure)
        {
            SemaphoreEventRecord* record = nullptr;
            s_faultCountdown.store(failure);
            s_faultPoint.store(4);
            const SemaphoreResult result = pool.begin(record);
            s_faultPoint.store(0);
            check(result.status == SEMAPHORE_NO_MEMORY && record == nullptr, "partial batch rollback");
            const SemaphorePoolStats stats = pool.statistics();
            check(
                stats.capacity == 0 && stats.reservedCapacity == 0 && stats.freeCapacity == 0, "reservation refunded");
        }
#else
        return false;
#endif
    }
    else if (id == 36)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        SemaphorePoolConfig config;
        config.initialCapacity = 2;
        config.maxCapacity = 40;
        std::atomic<uint64_t> idle(0);

        /** 计算本用例期望的新增记录数，异常分支用于验证策略错误反馈
        @param [in] context 容量及增长历史快照
        @return 期望新增记录数，实际容量由库截断
        */
        config.grow = [](const SemaphoreGrowContext& context) -> size_t
        {
            return 5;
        };
        /** 根据本用例的空闲条件提出缩容要求
        @param [in] context 空闲容量及增长历史快照
        @return 期望缩容量及连续空闲毫秒
        */
        config.shrink = [&](const SemaphoreShrinkContext& context) -> SemaphoreShrinkDecision
        {
            check(context.initialCapacity == 2, "shrink context");
            SemaphoreShrinkDecision result = {100, idle.load()};
            return result;
        };
        SemaphoreEventPool pool(config);
        SemaphoreEventRecord* records[32] = {};
        for (int32_t i = 0; i < 32; ++i)
        {
            check(pool.begin(records[i]).status == SEMAPHORE_OK, "multiple batches");
        }
        for (int32_t i = 0; i < 32; ++i)
        {
            pool.finish(records[i]);
        }
        size_t released = 0;
        check(pool.maintain(released).status == SEMAPHORE_OK && released == 0, "idle zero disables shrink");
        const uint64_t grows = pool.statistics().successfulGrowCount;
        idle.store(20);
        pause(50);
        SemaphoreTestThreads threads;
        for (int32_t i = 0; i < 4; ++i)
        {
            threads.start(
                /** 执行动态缩容的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    for (int32_t n = 0; n < 50; ++n)
                    {
                        size_t freed;
                        check(pool.maintain(freed).status == SEMAPHORE_OK, "concurrent maintenance");
                    }
                });
        }
        threads.join();
        pool.maintain(released);
        check(pool.statistics().capacity == 2 && pool.statistics().successfulGrowCount == grows,
            "shrink all batches to initial floor");
        for (int32_t i = 0; i < 3; ++i)
        {
            check(pool.begin(records[i]).status == SEMAPHORE_OK, "regrow");
        }
        for (int32_t i = 0; i < 3; ++i)
        {
            pool.finish(records[i]);
        }
        check(pool.statistics().successfulGrowCount == grows + 1, "growth history survives shrink");
#else
        return false;
#endif
    }
    else if (id == 37)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        SemaphorePoolConfig config;
        config.initialCapacity = 1;
        config.maxCapacity = 1;
        SemaphoreEventPool pool(config);
        SemaphoreEventRecord* old = nullptr;
        SemaphoreEventRecord* next = nullptr;
        check(pool.begin(old).status == SEMAPHORE_OK, "old register");
        SemaphoreTestGate gate(102);
        SemaphoreTestThreads threads;
        threads.start(
            /** 执行暂停读者回收保护的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                pool.notify(true);
            });
        const bool arrived = gate.reached();
        check(!pool.finish(old), "timeout wins while scanner paused");
        const SemaphorePoolStats paused = pool.statistics();
        const SemaphoreResult full = pool.begin(next);
        gate.release();
        threads.join();
        check(arrived && paused.retiredCapacity == 1 && full.status == SEMAPHORE_CAPACITY,
            "borrowed retirement counts in limit");
        check(pool.begin(next).status == SEMAPHORE_OK, "reclaim after scanner leaves");
        pool.finish(next);
#else
        return false;
#endif
    }
    else if (id == 38)
    {
        Semaphore sem;
        SemaphorePoolConfig config;
        config.initialCapacity = 4;
        config.maxCapacity = 64;
        check(sem.configureEventPool(config).status == SEMAPHORE_OK, "stress config");
        std::atomic<int32_t> completed(0);
        SemaphoreTestThreads threads;
        for (int32_t i = 0; i < 6; ++i)
        {
            threads.start(
                /** 执行事件池并发压力的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    for (int32_t n = 0; n < 2000; ++n)
                    {
                        const SemaphoreResult result = sem.eventWaitResult(1);
                        check(result.status == SEMAPHORE_OK || result.status == SEMAPHORE_TIMEOUT ||
                                result.status == SEMAPHORE_CAPACITY || result.status == SEMAPHORE_RETRY,
                            "stress wait result");
                        ++completed;
                    }
                });
        }
        threads.start(
            /** 执行事件池并发压力的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                const double deadline = now() + 15000;
                while (completed.load() < 12000 && now() < deadline)
                {
                    sem.event();
                    sem.eventAll();
                    pause(1);
                }
            });
        threads.join();
        size_t released;
        sem.maintainEventPool(released);
        const SemaphorePoolStats stats = sem.eventPoolStatistics();
        check(stats.capacity <= 64 && stats.freeCapacity == stats.capacity && stats.retiredCapacity == 0,
            "stress no lost records");
    }
    else if (id == 39)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        s_faultCountdown.store(1);
        s_faultPoint.store(1);
        bool threw = false;
        try
        {
            Semaphore sem;
        }
        catch (const std::system_error&)
        {
            threw = true;
        }
        s_faultPoint.store(0);
        check(threw, "system construction failure observable");
        Semaphore sem;
        {
            // 无等待者的计数发布不调用系统提示；在真实登记后验证交付成功但提示失败。
            SemaphoreTestGate countGate(8);
            SemaphoreTestThreads countThreads;
            SemaphoreResult consumed;
            countThreads.start([&]() { consumed = sem.waitResult(1000); });
            const bool registered = countGate.reached();
            s_faultCountdown.store(1);
            s_faultPoint.store(2);
            const SemaphoreResult published = sem.trySignal();
            s_faultPoint.store(0);
            countGate.release();
            countThreads.join();
            check(registered && published.status == SEMAPHORE_SYSTEM_ERROR && published.committed &&
                    published.nativeError == 123, "postcommit wake failure");
            check(consumed.status == SEMAPHORE_OK && consumed.committed && !sem.wait(0), "postcommit must not retry");
        }
        // 兼容void入口也必须说明许可已经提交；分别覆盖普通竞争与优先登记两条路径。
        for (int32_t point = 7; point <= 8; ++point)
        {
            SemaphoreTestGate countGate(point);
            SemaphoreTestThreads countThreads;
            SemaphoreResult consumed;
            countThreads.start([&]() { consumed = sem.waitResult(1000); });
            const bool registered = countGate.reached();
            s_faultCountdown.store(1);
            s_faultPoint.store(2);
            bool reported = false;
            try
            {
                sem.signal();
            }
            catch (const std::system_error& error)
            {
                reported = error.code().value() == 123 && std::strstr(error.what(), "notification committed") != nullptr;
            }
            catch (...)
            {
                s_faultPoint.store(0);
                countGate.release();
                countThreads.join();
                throw;
            }
            s_faultPoint.store(0);
            countGate.release();
            countThreads.join();
            check(registered && reported, "legacy signal exception retains committed notification");
            check(consumed.status == SEMAPHORE_OK && consumed.committed && !sem.wait(0),
                "legacy signal wake failure retains exactly one permit");
        }
        s_faultCountdown.store(1);
        s_faultPoint.store(3);
        const SemaphoreResult waited = sem.eventWaitResult(100);
        s_faultPoint.store(0);
        check(waited.status == SEMAPHORE_SYSTEM_ERROR && !waited.committed, "wait error observable");
        check(sem.eventPoolStatistics().freeCapacity == 16, "wait error unregisters record");
        // 在登记后、系统睡眠前注入通知错误；已提交事件必须仍能消费，不能重发。
        SemaphoreResult received;
        SemaphoreTestGate gate(1);
        SemaphoreTestThreads threads;
        threads.start([&]() { received = sem.eventWaitResult(1000); });
        const bool arrived = gate.reached();
        s_faultCountdown.store(1);
        s_faultPoint.store(2);
        const SemaphoreResult event = sem.tryEvent();
        s_faultPoint.store(0);
        gate.release();
        threads.join();
        check(arrived && event.status == SEMAPHORE_SYSTEM_ERROR && event.committed && event.nativeError == 123,
            "event wake failure retains committed result");
        check(received.status == SEMAPHORE_OK && received.committed &&
                sem.eventPoolStatistics().freeCapacity == 16, "committed event survives failed native hint");
#else
        return false;
#endif
    }
    else if (id == 40)
    {
        Semaphore owner;
        Semaphore joiner;
        Semaphore opener;
        check(owner.processWaitResult(-2).status == SEMAPHORE_INVALID, "invalid process timeout");
        check(owner.processSignalResult().status == SEMAPHORE_NOT_OPEN, "unopened signal");
        check(owner.createProcessResult("bad/name").status == SEMAPHORE_INVALID, "slash name");
        check(owner.createProcessResult(std::string(65536, 'a')).status != SEMAPHORE_OK,
            "system/backend rejects excessive name");
#ifdef SEMAPHORE_SHARED_COUNTER
        check(
            owner.createProcessResult(std::string(31, 'a')).status == SEMAPHORE_INVALID, "mobile protocol name length");
#endif
        check(owner.createProcessResult(std::string("a\0b", 3)).status == SEMAPHORE_INVALID, "embedded NUL");
        check(!opener.tryOpenProcessSemaphore(name(999)), "open missing does not create");
        check(owner.createProcessResult(name(id), 2).status == SEMAPHORE_OK, "create IPC");
        check(owner.processSignalResult().committed, "IPC first count");
        check(joiner.createProcessResult(name(id), 1).status == SEMAPHORE_OK, "join existing creator");
        check(joiner.processWait(0), "join does not reset");
        check(opener.openProcessResult(name(id)).status == SEMAPHORE_OK, "open IPC");
        check(owner.processSignalResult().committed && owner.processSignalResult().committed, "IPC two permits");
#if (defined(_WIN32) && !defined(SEMAPHORE_TEST_POSIX_NAME)) || defined(SEMAPHORE_SHARED_COUNTER)
        check(owner.processSignalResult().status == SEMAPHORE_OVERFLOW, "IPC full");
#endif
        check(opener.processWait(0) && joiner.processWait(0) && !owner.processWait(0), "shared counter exact");
        const double started = now();
        check(!owner.processWait(80), "IPC timed timeout");
        check(now() - started >= 60 && now() - started < 500, "IPC total budget");
        check(joiner.closeProcessResult().status == SEMAPHORE_OK, "joiner close");
        check(owner.closeProcessResult().status == SEMAPHORE_OK, "owner close");
        check(opener.processSignalResult().committed && opener.processWait(0), "opened reference survives owner close");
        check(opener.closeProcessResult().status == SEMAPHORE_OK && opener.closeProcessResult().status == SEMAPHORE_OK,
            "repeat close");
        check(!joiner.tryOpenProcessSemaphore(name(id)), "last reference/name cleanup");
    }
    else if (id == 41 || id == 60)
    {
#if defined(__APPLE__) && TARGET_OS_IPHONE
        std::printf(
            "NOT_RUN_DETAIL id=%d iOS requires two host applications sharing an App Group; fork is unavailable\n", id);
        return false;
#else
        Semaphore request;
        Semaphore response;
        const std::string logicalName = name(id);
        check(request.createProcessResult(logicalName).status == SEMAPHORE_OK, "parent create request");
        check(response.createProcessResult(logicalName + "r").status == SEMAPHORE_OK, "parent create response");
#ifdef _WIN32
        char executable[32768];
        check(::GetModuleFileNameA(nullptr, executable, sizeof(executable)) != 0, "self path");
        std::string command =
            std::string("\"") + executable + (id == 60 ? "\" --exchange-peer " : "\" --peer ") + logicalName;
        std::vector<char> mutableCommand(command.begin(), command.end());
        mutableCommand.push_back(0);
        STARTUPINFOA startup = {};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process = {};
        check(::CreateProcessA(nullptr, &mutableCommand[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
                  &startup, &process) != FALSE,
            "spawn independent process");
        const bool ready = response.processWait(5000);
        bool sent = true, replied = true;
        for (int32_t i = 0; i < (id == 60 ? 200 : 1); ++i)
        {
            if (id == 60)
            {
                const SemaphoreSignalWaitResult result = request.processSignalAndWaitResult(response, 5000);
                sent = sent && result.signalState == SEMAPHORE_COMMIT_DONE;
                replied = replied && result.status == SEMAPHORE_OK && result.waitState == SEMAPHORE_COMMIT_DONE;
            }
            else
            {
                sent = request.processSignalResult().committed;
                replied = response.processWait(5000);
            }
            if (!sent || !replied)
            {
                break;
            }
        }
        const DWORD waited = ::WaitForSingleObject(process.hProcess, 5000);
        DWORD code = 1;
        if (waited != WAIT_OBJECT_0)
        {
            ::TerminateProcess(process.hProcess, 90);
            ::WaitForSingleObject(process.hProcess, 5000);
        }
        ::GetExitCodeProcess(process.hProcess, &code);
        ::CloseHandle(process.hThread);
        ::CloseHandle(process.hProcess);
        check(ready && sent && replied && waited == WAIT_OBJECT_0 && code == 0, "true IPC handshake and child exit");
#else
        // 子程序从全新地址空间打开对象，不能在fork后调用继承的库实例
        const std::string executable = SemaphoreTestProcess::executable();
        char* arguments[] = {const_cast<char*>(executable.c_str()),
            const_cast<char*>(id == 60 ? "--exchange-peer" : "--peer"), const_cast<char*>(logicalName.c_str()),
            nullptr};
        pid_t child = -1;
        check(::posix_spawn(&child, executable.c_str(), nullptr, nullptr, arguments, environ) == 0,
            "spawn independent peer");
        const bool ready = response.processWait(5000);
        bool sent = true, replied = true;
        for (int32_t i = 0; i < (id == 60 ? 200 : 1); ++i)
        {
            if (id == 60)
            {
                const SemaphoreSignalWaitResult result = request.processSignalAndWaitResult(response, 5000);
                sent = sent && result.signalState == SEMAPHORE_COMMIT_DONE;
                replied = replied && result.status == SEMAPHORE_OK && result.waitState == SEMAPHORE_COMMIT_DONE;
            }
            else
            {
                sent = request.processSignalResult().committed;
                replied = response.processWait(5000);
            }
            if (!sent || !replied)
            {
                break;
            }
        }
        int32_t status = 0;
        pid_t joined = 0;
        const double deadline = now() + 5000;
        while (joined == 0 && now() < deadline)
        {
            joined = ::waitpid(child, &status, WNOHANG);
            if (joined == 0)
            {
                pause(1);
            }
        }
        if (joined == 0)
        {
            ::kill(child, SIGKILL);
            ::waitpid(child, &status, 0);
        }
        check(ready && sent && replied && joined == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
            "true IPC handshake");
#endif
#endif
    }
    else if (id == 42)
    {
        Semaphore sem;
        std::atomic<bool> stopping(false);
        std::atomic<int32_t> exited(0);
        SemaphoreTestThreads threads;
        try
        {
            for (int32_t i = 0; i < 8; ++i)
            {
                threads.start(
                    /** 执行无限等待退出唤醒的线程步骤，捕获状态必须活到线程回收
                    */
                    [&]()
                    {
                        sem.wait();
                        check(stopping.load(), "exit predicate");
                        ++exited;
                    });
            }
        }
        catch (...)
        {
            stopping.store(true);
            for (int32_t i = 0; i < 8; ++i)
            {
                sem.trySignal();
            }
            throw;
        }
        pause(30);
        stopping.store(true);
        for (int32_t i = 0; i < 8; ++i)
        {
            sem.signal();
        }
        threads.join();
        check(exited.load() == 8, "all infinite count waiters exit before destructor");
    }
    else if (id == 43)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        const int32_t liveBefore = s_liveAllocations.load();
        for (int32_t cycle = 0; cycle < 100; ++cycle)
        {
            Semaphore sem;
            SemaphorePoolConfig config;
            config.initialCapacity = 8;
            check(sem.configureEventPool(config).status == SEMAPHORE_OK, "repeated pool construction");
            check(sem.configureEventPool(config).status == SEMAPHORE_OK, "repeated pool replacement");
        }
        check(s_liveAllocations.load() == liveBefore, "all event pool allocations return to baseline");
        std::printf("METRIC heap_live_before=%d after=%d pool_cycles=100\n", liveBefore, s_liveAllocations.load());
#endif
#ifdef _WIN32
        DWORD before = 0, after = 0;
        check(::GetProcessHandleCount(::GetCurrentProcess(), &before) != FALSE, "read initial handles");
        for (int32_t i = 0; i < 1000; ++i)
        {
            Semaphore sem;
            sem.signal();
            check(sem.wait(0), "resource cycle");
        }
        check(::GetProcessHandleCount(::GetCurrentProcess(), &after) != FALSE, "read final handles");
        check(after <= before + 2, "handle trend");
        FILETIME create, exit, kernel0, user0, kernel1, user1;
        check(::GetProcessTimes(::GetCurrentProcess(), &create, &exit, &kernel0, &user0) != FALSE, "read initial CPU");
        Semaphore sem;
        const double started = now();
        check(!sem.wait(1200), "idle blocks");
        const double elapsed = now() - started;
        check(::GetProcessTimes(::GetCurrentProcess(), &create, &exit, &kernel1, &user1) != FALSE, "read final CPU");
        const uint64_t k0 = (uint64_t(kernel0.dwHighDateTime) << 32) | kernel0.dwLowDateTime;
        const uint64_t k1 = (uint64_t(kernel1.dwHighDateTime) << 32) | kernel1.dwLowDateTime;
        const uint64_t u0 = (uint64_t(user0.dwHighDateTime) << 32) | user0.dwLowDateTime;
        const uint64_t u1 = (uint64_t(user1.dwHighDateTime) << 32) | user1.dwLowDateTime;
        const double cpu = double(k1 - k0 + u1 - u0) / 10000;
        std::printf("METRIC idle_wall_ms=%.3f cpu_ms=%.3f handles_before=%lu after=%lu\n", elapsed, cpu, before, after);
        check(cpu < elapsed * 0.20, "no busy waiting");
#else
        Semaphore sem;
        check(!sem.wait(1200), "idle wait");
#endif
        check(Semaphore::cleanupFailures() == 0, "cleanup diagnostics");
    }
    else if (id == 44)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        const int32_t points[] = {103, 1, 3, 5};
        for (int32_t index = 0; index < 4; ++index)
        {
            Semaphore sem;
            SemaphoreTestGate gate(points[index]);
            std::atomic<bool> received(false);
            SemaphoreTestThreads threads;
            threads.start(
                /** 执行事件记录复用的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    received.store(sem.eventWait(100));
                });
            const bool arrived = gate.reached();
            sem.eventAll();
            gate.release();
            threads.join();
            check(arrived && received.load() == (points[index] != 103), "event registration/sleep/timeout boundary");
            check(!sem.eventWait(1), "no old event after record reuse");
        }
#else
        return false;
#endif
    }
    else if (id == 45)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        const int32_t before = s_allocationCalls.load();
        delete new Semaphore;
        const int32_t allocationSites = s_allocationCalls.load() - before;
        check(allocationSites >= 1, "real object allocation sites observed");
        for (int32_t failure = 1; failure <= allocationSites; ++failure)
        {
            s_allocationCountdown.store(failure);
            bool failed = false;
            try
            {
                delete new Semaphore;
            }
            catch (const std::bad_alloc&)
            {
                failed = true;
            }
            s_allocationCountdown.store(0);
            check(failed, "real allocation failure propagated");
            Semaphore retry;
            retry.signal();
            check(retry.wait(0), "construction retry after failure");
        }
        Semaphore prepared;
        const int32_t beforeEvent = s_allocationCalls.load();
        check(prepared.eventWaitResult(1).status == SEMAPHORE_TIMEOUT, "initial event allocation probe");
        const int32_t eventSites = s_allocationCalls.load() - beforeEvent;
        check(eventSites >= 17, "real lazy pool allocations observed");
        for (int32_t failure = 1; failure <= eventSites; ++failure)
        {
            Semaphore fresh;
            s_allocationCountdown.store(failure);
            const SemaphoreResult result = fresh.eventWaitResult(1);
            s_allocationCountdown.store(0);
            check(result.status == SEMAPHORE_NO_MEMORY && fresh.eventPoolStatistics().capacity == 0,
                "failed lazy pool leaves no partial publication");
            check(fresh.eventWaitResult(1).status == SEMAPHORE_TIMEOUT, "lazy pool retry after allocation failure");
        }
        Semaphore sem;
        const int32_t quiet = s_allocationCalls.load();
        sem.signal();
        sem.event();
        sem.eventAll();
        check(s_allocationCalls.load() == quiet, "signal/event paths allocate nothing");
        std::printf(
            "METRIC constructor_allocation_sites=%d first_event_allocation_sites=%d\n", allocationSites, eventSites);
#else
        return false;
#endif
    }
    else if (id == 46)
    {
#if defined(_WIN32) && !defined(PROCESS_WORK_PORTABLE_NOTIFY)
        const std::string logicalName = name(id);
        HANDLE incompatible = ::CreateEventA(nullptr, TRUE, FALSE, logicalName.c_str());
        check(incompatible != nullptr, "create incompatible named object");
        Semaphore sem;
        const SemaphoreResult wrong = sem.createProcessResult(logicalName);
        const SemaphoreResult wrongOpen = sem.openProcessResult(logicalName);
        const bool closed = ::CloseHandle(incompatible) != FALSE;
        check(closed && wrong.status == SEMAPHORE_SYSTEM_ERROR && wrongOpen.status == SEMAPHORE_SYSTEM_ERROR,
            "wrong object type is observable");
        check(sem.createProcessResult(logicalName).status == SEMAPHORE_OK, "retry after wrong object removed");
#else
        return false;
#endif
    }
    else if (id == 50 || id == 70)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        SemaphorePoolConfig config;
        config.initialCapacity = 0;
        config.maxCapacity = 2;
        SemaphoreEventPool pool(config);
        SemaphoreEventRecord* first = nullptr;
        SemaphoreEventRecord* other = nullptr;
        SemaphoreTestGate gate(id == 50 ? 104 : 105);
        SemaphoreTestThreads threads;
        threads.start(
            /** 执行并发扩容预算的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                check(pool.begin(first).status == SEMAPHORE_OK, "reserved batch publishes");
            });
        const bool arrived = gate.reached();
        const SemaphorePoolStats snapshot = pool.statistics();
        const SemaphoreResult competing = pool.begin(other);
        gate.release();
        threads.join();
        check(
            arrived && snapshot.reservedCapacity == 2 && snapshot.capacity == 0 && competing.status == SEMAPHORE_RETRY,
            "pending allocation remains inside shared maximum");
        check(pool.statistics().capacity == 2, "no concurrent oversubscription");
        pool.finish(first);
#else
        return false;
#endif
    }
    else if (id == 51)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        Semaphore sem;
        SemaphoreTestGate gate(1);
        std::atomic<bool> first(false);
        SemaphoreTestThreads threads;
        threads.start(
            /** 执行暂停等待者不私占许可的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                first.store(sem.wait(150));
            });
        const bool arrived = gate.reached();
        sem.signal();
        const bool other = sem.wait(100);
        gate.release();
        threads.join();
        check(arrived && other && !first.load(), "paused waiter cannot reserve a count permit");
#else
        return false;
#endif
    }
    else if (id == 53)
    {
        // 原接口接受这些字节名称，验证未擅自改为仅允许ASCII
        const std::string compatible = name(id) + " space\xc3\xa9";
        Semaphore owner;
        Semaphore peer;
        check(owner.createProcessResult(compatible).status == SEMAPHORE_OK, "preserve space/non-ASCII byte names");
        check(peer.openProcessResult(compatible).status == SEMAPHORE_OK, "open compatible name");
        check(owner.processSignalResult().committed && peer.processWait(0), "compatible name communicates");
#if defined(_WIN32) && !defined(SEMAPHORE_SHARED_COUNTER)
        Semaphore local;
        Semaphore localPeer;
        const std::string nativeName = "Local\\" + name(id) + std::string(40, 'x');
        check(local.createProcessResult(nativeName).status == SEMAPHORE_OK,
            "preserve native namespace and native length");
        check(localPeer.openProcessResult(nativeName).status == SEMAPHORE_OK, "open native namespace");
        check(local.processSignalResult().committed && localPeer.processWait(0), "native namespace communicates");
#endif
    }
    else if (id == 54)
    {
#if defined(SEMAPHORE_TEST_INSTRUMENTATION) && defined(_WIN32) && defined(SEMAPHORE_SHARED_COUNTER)
        Semaphore owner;
        Semaphore probe;
        const std::string original = name(id);
        check(owner.createProcessResult(original).status == SEMAPHORE_OK, "partial close fixture");
        s_faultCountdown.store(1);
        s_faultPoint.store(10);
        const SemaphoreResult closed = owner.closeProcessResult();
        s_faultPoint.store(0);
        check(closed.status == SEMAPHORE_SYSTEM_ERROR && closed.nativeError == 123,
            "mapping handle close failure visible");
        check(owner.createProcessResult(original + "new").status == SEMAPHORE_INVALID &&
                owner.openProcessResult(original).status == SEMAPHORE_INVALID,
            "pending resource ownership cannot be overwritten");
        check(probe.openProcessResult(original).status == SEMAPHORE_OK, "pending handle is still owned");
        check(probe.closeProcessResult().status == SEMAPHORE_OK && owner.closeProcessResult().status == SEMAPHORE_OK,
            "close retry releases all references");
        check(!probe.tryOpenProcessSemaphore(original), "no leaked named mapping after retry");
        check(owner.createProcessResult(original + "new").status == SEMAPHORE_OK, "recreate only after cleanup");
#else
        return false;
#endif
    }
    else if (id == 56)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        Semaphore sem;
        check(sem.eventPoolStatistics().capacity == 0, "count-only instance has no event pool");
        const int32_t quiet = s_allocationCalls.load();
        sem.signal();
        check(sem.wait(0), "count-only fast permit");
        sem.event();
        sem.eventAll();
        check(s_allocationCalls.load() == quiet && sem.eventPoolStatistics().retainedBytes == 0,
            "count and early event notifications allocate no pool");
        SemaphoreTestGate gate(104);
        std::atomic<int32_t> first(SEMAPHORE_INVALID);
        SemaphoreTestThreads threads;
        threads.start(
            /** 执行首次事件池并发创建的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                first.store(sem.eventWaitResult(80).status);
            });
        const bool paused = gate.reached();
        const SemaphoreResult second = sem.eventWaitResult(1);
        gate.release();
        threads.join();
        const SemaphorePoolStats stats = sem.eventPoolStatistics();
        check(paused && second.status == SEMAPHORE_TIMEOUT && first.load() == SEMAPHORE_TIMEOUT,
            "paused first initializer cannot block another event waiter or retain an early event");
        check(stats.capacity == 16 && stats.freeCapacity == 16 && stats.reservedCapacity == 0,
            "one stable pool owns all published records after initialization race");
#else
        return false;
#endif
    }
    else if (id == 59)
    {
        Semaphore request, reply, unopened;
        check(request.createProcessResult(name(id), 1).status == SEMAPHORE_OK &&
                reply.createProcessResult(name(id) + "r", 1).status == SEMAPHORE_OK,
            "exchange named resources");
        SemaphoreSignalWaitResult result = request.processSignalAndWaitResult(unopened, 0);
        check(result.status == SEMAPHORE_NOT_OPEN && result.signalState == SEMAPHORE_COMMIT_NONE &&
                !request.processWait(0),
            "missing wait target rejects before publication");
        result = request.processSignalAndWaitResult(reply, -2);
        check(result.status == SEMAPHORE_INVALID && result.signalState == SEMAPHORE_COMMIT_NONE &&
                !request.processWait(0),
            "invalid exchange timeout has no side effect");
        const double start = now();
        result = request.processSignalAndWaitResult(reply, 25);
        const double elapsed = now() - start;
        check(result.status == SEMAPHORE_TIMEOUT && result.signalState == SEMAPHORE_COMMIT_DONE &&
                result.waitState == SEMAPHORE_COMMIT_NONE && elapsed >= 15 && elapsed < 300,
            "exchange timeout retains published request and one total budget");
        check(request.processWait(0) && !request.processWait(0), "timeout publishes exactly once");
        check(reply.tryProcessSignal() == 0, "prepare exchange reply");
        result = request.processSignalAndWaitResult(reply, 0);
        check(result.status == SEMAPHORE_OK && result.signalState == SEMAPHORE_COMMIT_DONE &&
                result.waitState == SEMAPHORE_COMMIT_DONE && request.processWait(0) && !reply.processWait(0),
            "exchange zero probe consumes prepared reply exactly once");
#if defined(_WIN32) || defined(SEMAPHORE_SHARED_COUNTER)
        check(request.tryProcessSignal() == 0 && reply.tryProcessSignal() == 0, "fill named limits");
        result = request.processSignalAndWaitResult(reply, 0);
        check(result.status == SEMAPHORE_OVERFLOW && result.signalState == SEMAPHORE_COMMIT_NONE &&
                result.waitState == SEMAPHORE_COMMIT_NONE && request.processWait(0) && reply.processWait(0),
            "overflow neither publishes nor consumes reply");
#endif
        result = request.processSignalAndWaitResult(request, 0);
        check(result.status == SEMAPHORE_OK && result.signalState == SEMAPHORE_COMMIT_DONE &&
                result.waitState == SEMAPHORE_COMMIT_DONE && !request.processWait(0),
            "same-object exchange is exact");
        SemaphoreTestThreads threads;
        threads.start(
            /** 执行IPC组合接口边界的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                check(request.processWait(5000), "infinite exchange request");
                check(reply.tryProcessSignal() == 0, "infinite exchange reply");
            });
        result = request.processSignalAndWaitResult(reply, -1);
        threads.join();
        check(result.status == SEMAPHORE_OK && result.waitState == SEMAPHORE_COMMIT_DONE && !request.processWait(0) &&
                !reply.processWait(0),
            "infinite named exchange completes without surplus");
    }
    else if (id == 61)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        Semaphore request, reply;
        check(request.createProcessResult(name(id), 2).status == SEMAPHORE_OK &&
                reply.createProcessResult(name(id) + "r", 2).status == SEMAPHORE_OK,
            "exchange fault resources");
        for (int32_t point = 11; point <= 12; ++point)
        {
            s_faultCountdown.store(1);
            s_faultPoint.store(point);
            const SemaphoreSignalWaitResult result = request.processSignalAndWaitResult(reply, 0);
            s_faultPoint.store(0);
            check(result.status == SEMAPHORE_SYSTEM_ERROR && result.nativeError == 123 &&
                    result.signalState == (point == 11 ? SEMAPHORE_COMMIT_NONE : SEMAPHORE_COMMIT_DONE) &&
                    result.waitState == SEMAPHORE_COMMIT_NONE,
                "known exchange phase failure");
            check(request.processWait(0) == (point == 12) && !request.processWait(0) && !reply.processWait(0),
                "fault side effects agree with result");
        }
#if defined(_WIN32) && !defined(SEMAPHORE_SHARED_COUNTER)
        check(reply.tryProcessSignal() == 0, "ambiguous phase prepared reply");
        s_faultCountdown.store(1);
        s_faultPoint.store(13);
        const SemaphoreSignalWaitResult result = request.processSignalAndWaitResult(reply, 0);
        s_faultPoint.store(0);
        check(result.status == SEMAPHORE_SYSTEM_ERROR && result.nativeError == 123 &&
                result.signalState == SEMAPHORE_COMMIT_UNKNOWN && result.waitState == SEMAPHORE_COMMIT_UNKNOWN,
            "ambiguous native failure cannot authorize a resend or double consume");
        check(request.processWait(0) && !request.processWait(0) && !reply.processWait(0),
            "ambiguous failure preserves actual native side effects");
#endif
#else
        return false;
#endif
    }
    else if (id == 62)
    {
        for (int32_t prepared = 0; prepared <= 3; ++prepared)
        {
            Semaphore request, reply;
            if ((prepared & 1) != 0)
            {
                check(request.prepareSignalAndWait().status == SEMAPHORE_OK &&
                        request.prepareSignalAndWait().status == SEMAPHORE_OK,
                    "idempotent local exchange preparation");
            }
            if ((prepared & 2) != 0)
            {
                check(reply.prepareSignalAndWait().status == SEMAPHORE_OK &&
                        reply.prepareSignalAndWait().status == SEMAPHORE_OK,
                    "idempotent reply preparation");
            }
            reply.signal();
            SemaphoreSignalWaitResult result = request.signalAndWaitResult(reply, 0);
            check(result.status == SEMAPHORE_OK && result.signalState == SEMAPHORE_COMMIT_DONE &&
                    result.waitState == SEMAPHORE_COMMIT_DONE && request.wait(0) && !reply.wait(0),
                "local exchange zero probe");
            const double start = now();
            result = request.signalAndWaitResult(reply, 25);
            check(result.status == SEMAPHORE_TIMEOUT && result.signalState == SEMAPHORE_COMMIT_DONE &&
                    result.waitState == SEMAPHORE_COMMIT_NONE && now() - start >= 15 && now() - start < 300,
                "local exchange deadline and committed request");
            check(request.wait(0) && !request.wait(0), "local exchange timeout publishes once");
            result = request.signalAndWaitResult(request, 0);
            check(result.status == SEMAPHORE_OK && !request.wait(0), "local same-object exchange");
            int32_t payload = 0;
            SemaphoreTestThreads threads;
            threads.start(
                /** 执行本地组合准备状态的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    for (int32_t i = 1; i <= 200; ++i)
                    {
                        check(request.wait(5000) && payload == i, "local combined ordinary payload publication");
                        reply.signal();
                    }
                });
            for (int32_t i = 1; i <= 200; ++i)
            {
                payload = i;
                result = i == 1 ? request.signalAndWaitResult(reply) : request.signalAndWaitResult(reply, 5000);
                check(result.status == SEMAPHORE_OK && result.signalState == SEMAPHORE_COMMIT_DONE &&
                        result.waitState == SEMAPHORE_COMMIT_DONE,
                    "local combined reply");
            }
            threads.join();
            check(!request.wait(0) && !reply.wait(0), "local combined exact round count");
        }
    }
    else if (id == 64)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        for (int32_t point = 4; point <= 6; point += 2)
        {
            Semaphore request, reply;
            check(request.prepareSignalAndWait().status == SEMAPHORE_OK &&
                    reply.prepareSignalAndWait().status == SEMAPHORE_OK,
                "boundary prepare");
            SemaphoreTestGate gate(point);
            std::atomic<int32_t> status(SEMAPHORE_INVALID);
            SemaphoreTestThreads threads;
            threads.start(
                /** 执行本地组合提交资源边界的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    status.store(request.signalAndWaitResult(reply, 500).status);
                });
            const bool reached = gate.reached();
            const bool sent = request.wait(0);
            reply.signal();
            gate.release();
            threads.join();
            check(reached && sent && status.load() == SEMAPHORE_OK && !request.wait(0) && !reply.wait(0),
                "reply at publication/reset/sleep boundary cannot be lost or repeated");
        }
        Semaphore request, reply;
        s_faultCountdown.store(1);
        s_faultPoint.store(14);
        const SemaphoreResult failed = request.prepareSignalAndWait();
        s_faultPoint.store(0);
        check(failed.status == SEMAPHORE_SYSTEM_ERROR && failed.nativeError == 123, "prepare failure is observable");
        request.signal();
        check(request.wait(0), "old backend survives failed preparation");
        check(request.prepareSignalAndWait().status == SEMAPHORE_OK &&
                reply.prepareSignalAndWait().status == SEMAPHORE_OK,
            "prepare retry succeeds");
        s_seedObject = &request;
        s_seedValue = UINT64_MAX;
        request.reset();
        s_seedObject = nullptr;
        reply.signal();
        SemaphoreSignalWaitResult result = request.signalAndWaitResult(reply, 0);
        check(result.status == SEMAPHORE_OVERFLOW && result.signalState == SEMAPHORE_COMMIT_NONE &&
                result.waitState == SEMAPHORE_COMMIT_NONE && reply.wait(0),
            "combined local preserves full 64-bit overflow");
        s_seedObject = &request;
        s_seedValue = UINT64_MAX - 1;
        request.reset();
        s_seedObject = nullptr;
        result = request.signalAndWaitResult(reply, 0);
        check(result.status == SEMAPHORE_TIMEOUT && result.signalState == SEMAPHORE_COMMIT_DONE &&
                request.trySignal().status == SEMAPHORE_OVERFLOW,
            "combined publication retains all 64 counter bits");
        request.reset();
        s_faultCountdown.store(1);
        s_faultPoint.store(2);
        result = request.signalAndWaitResult(reply, 50);
        s_faultPoint.store(0);
        check(result.status == SEMAPHORE_SYSTEM_ERROR && result.signalState == SEMAPHORE_COMMIT_DONE &&
                result.waitState == SEMAPHORE_COMMIT_NONE && request.wait(0) && !request.wait(0),
            "local native hint failure cannot hide published request");
        reply.signal();
        s_faultCountdown.store(1);
        s_faultPoint.store(2);
        result = request.signalAndWaitResult(reply, 0);
        s_faultPoint.store(0);
#ifdef _WIN32
        check(result.status == SEMAPHORE_SYSTEM_ERROR && result.signalState == SEMAPHORE_COMMIT_DONE &&
                result.waitState == SEMAPHORE_COMMIT_DONE && request.wait(0) && !reply.wait(0),
            "fast reply consumption remains visible when source hint fails");
#else
        check(result.status == SEMAPHORE_SYSTEM_ERROR && result.signalState == SEMAPHORE_COMMIT_DONE &&
                result.waitState == SEMAPHORE_COMMIT_NONE && request.wait(0) && reply.wait(0),
            "sequential fallback stops before consuming after failed hint");
#endif
        const int32_t allocationCalls = s_allocationCalls.load();
        result = request.signalAndWaitResult(reply, 5);
        check(result.status == SEMAPHORE_TIMEOUT && request.wait(0) && s_allocationCalls.load() == allocationCalls,
            "prepared combination has no C++ allocation in notify/wait path");
#ifdef _WIN32
        DWORD before = 0, after = 0;
        check(::GetProcessHandleCount(::GetCurrentProcess(), &before) != FALSE, "handle baseline");
        for (int32_t i = 0; i < 1000; ++i)
        {
            Semaphore temporary;
            check(temporary.prepareSignalAndWait().status == SEMAPHORE_OK, "repeated preparation");
        }
        check(::GetProcessHandleCount(::GetCurrentProcess(), &after) != FALSE && after == before,
            "prepared native handles return to baseline");
#endif
#else
        return false;
#endif
    }
    else if (id == 75)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        SemaphoreWake wake;
        if (!wake.canWaitAddress())
        {
            return false;
        }
        SemaphorePoolConfig config;
        config.initialCapacity = 1;
        config.maxCapacity = 1;
        SemaphoreEventPool pool(config);
        SemaphoreEventRecord* old = nullptr;
        SemaphoreEventRecord* next = nullptr;
        check(pool.begin(old).status == SEMAPHORE_OK, "direct event register");
        SemaphoreResult notified;
        SemaphoreTestGate gate(106);
        SemaphoreTestThreads threads;
        threads.start([&]() { notified = pool.notify(true, &wake); });
        const bool arrived = gate.reached();
        // 发布者持有读者保护，等待者可独立完成，但记录不能提前重用给下一代。
        const bool received = pool.finish(old);
        const SemaphorePoolStats paused = pool.statistics();
        const SemaphoreResult full = pool.begin(next);
        gate.release();
        threads.join();
        check(arrived && received && paused.retiredCapacity == 1 && full.status == SEMAPHORE_CAPACITY,
            "pending native hint protects retired record address");
        check(notified.status == SEMAPHORE_OK && notified.committed, "direct event publisher completes");
        check(pool.begin(next).status == SEMAPHORE_OK && next == old && next->m_state.load() == 0,
            "record reusable only after native hint returns");
        check(!pool.finish(next) && pool.statistics().freeCapacity == 1,
            "old address hint cannot notify a new generation");
#else
        return false;
#endif
    }
    else if (id == 76)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        SemaphorePoolConfig config;
        config.initialCapacity = 256;
        config.maxCapacity = 256;
        SemaphoreEventPool pool(config);
        SemaphoreEventRecord* records[256] = {};
        for (size_t i = 0; i < 256; ++i)
        {
            check(pool.begin(records[i]).status == SEMAPHORE_OK, "prepare retirement backlog");
        }
        {
            SemaphoreTestGate gate(101);
            SemaphoreTestThreads threads;
            // 保留扫描者的读者引用，迫使结束的记录进入退役链，不能被提前复用。
            threads.start([&]() { pool.notify(true); });
            const bool arrived = gate.reached();
            for (size_t i = 0; i < 256; ++i)
            {
                pool.finish(records[i]);
            }
            size_t released = 0;
            const SemaphoreResult maintained = pool.maintain(released);
            const SemaphorePoolStats paused = pool.statistics();
            gate.release();
            threads.join();
            check(arrived && maintained.status == SEMAPHORE_OK && released == 0 &&
                    paused.retiredCapacity == 256 && paused.freeCapacity == 0,
                "paused reader keeps complete retired budget");
        }
        for (size_t round = 1; round <= 4; ++round)
        {
            size_t released = 0;
            check(pool.maintain(released).status == SEMAPHORE_OK && released == 0,
                "recycle is not physical release");
            const SemaphorePoolStats stats = pool.statistics();
            check(stats.capacity == 256 && stats.freeCapacity == round * 64 &&
                    stats.retiredCapacity == 256 - round * 64,
                "one maintenance call processes at most 64 retired records");
        }
        std::printf("BUDGET records=256 rounds=4 max_recycled_per_call=64\n");
        // 同一轮先回收再缩容：两处回收共用预算，不能各自再处理64条。
        bool shrinking = true;
        uint64_t idleLimit = UINT64_MAX;
        config.initialCapacity = 0;
        // 固定新增256条，容量仍受池自身的最大预算约束。
        config.grow = [](const SemaphoreGrowContext& context) -> size_t
        {
            (void)context;
            return 256;
        };
        // 捕获值只由已停止并发操作的主测试线程修改，用于切换空闲期和禁用缩容。
        config.shrink = [&](const SemaphoreShrinkContext& context) -> SemaphoreShrinkDecision
        {
            (void)context;
            const SemaphoreShrinkDecision decision = {64, shrinking ? idleLimit : 0};
            return decision;
        };
        SemaphoreEventPool sharedBudget(config);
        for (size_t i = 0; i < 256; ++i)
        {
            check(sharedBudget.begin(records[i]).status == SEMAPHORE_OK, "prepare shared reclaim budget");
        }
        {
            SemaphoreTestGate gate(101);
            SemaphoreTestThreads threads;
            threads.start([&]() { sharedBudget.notify(true); });
            const bool arrived = gate.reached();
            for (size_t i = 0; i < 256; ++i)
            {
                sharedBudget.finish(records[i]);
            }
            gate.release();
            threads.join();
            check(arrived && sharedBudget.statistics().retiredCapacity == 256, "prepare second retired backlog");
        }
        size_t released = 0;
        check(sharedBudget.maintain(released).status == SEMAPHORE_OK && released == 0 &&
                sharedBudget.statistics().retiredCapacity == 256 && sharedBudget.statistics().freeCapacity == 0,
            "second reclaim cannot exceed the shared 64-attempt budget");
        shrinking = false;
        for (size_t i = 0; i < 4; ++i)
        {
            check(sharedBudget.maintain(released).status == SEMAPHORE_OK, "drain shared budget backlog");
        }
        check(sharedBudget.statistics().freeCapacity == 256, "entire shared budget backlog returns");
        shrinking = true;
        idleLimit = 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        for (size_t round = 1; round <= 4; ++round)
        {
            check(sharedBudget.maintain(released).status == SEMAPHORE_OK && released == 64 &&
                    sharedBudget.statistics().capacity == 256 - round * 64,
                "physical releases remain bounded and eventually reach initial capacity");
        }
#else
        return false;
#endif
    }
    else if (id == 77)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        for (int32_t point = 107; point <= 108; ++point)
        {
            for (int32_t repeat = 0; repeat < 8; ++repeat)
            {
                SemaphorePoolConfig config;
                config.initialCapacity = 2;
                config.maxCapacity = 2;
                SemaphoreEventPool pool(config);
                SemaphoreEventRecord* a = nullptr;
                SemaphoreEventRecord* b = nullptr;
                check(pool.begin(a).status == SEMAPHORE_OK && pool.begin(b).status == SEMAPHORE_OK,
                    "prepare two retired candidates");
                {
                    SemaphoreTestGate gate(101);
                    SemaphoreTestThreads threads;
                    threads.start([&]() { pool.notify(true); });
                    const bool arrived = gate.reached();
                    pool.finish(a);
                    pool.finish(b);
                    gate.release();
                    threads.join();
                    check(arrived && pool.statistics().retiredCapacity == 2, "two records quarantined");
                }
                {
                    SemaphoreTestGate gate(point);
                    SemaphoreTestThreads threads;
                    threads.start([&]()
                    {
                        size_t released = 0;
                        check(pool.maintain(released).status == SEMAPHORE_OK, "paused reclaimer finishes");
                    });
                    const bool arrived = gate.reached();
                    size_t released = 0;
                    const SemaphoreResult maintained = pool.maintain(released);
                    const SemaphorePoolStats paused = pool.statistics();
                    gate.release();
                    threads.join();
                    check(arrived && maintained.status == SEMAPHORE_OK && released == 0 &&
                            paused.freeCapacity == 0 && paused.retiredCapacity == 2,
                        "borrowed retired head cannot be reused during concurrent pop and requeue");
                }
                size_t released = 0;
                pool.maintain(released);
                check(pool.statistics().freeCapacity == 2 && pool.statistics().retiredCapacity == 0,
                    "all borrowed records eventually return");
                check(pool.begin(a).status == SEMAPHORE_OK && pool.begin(b).status == SEMAPHORE_OK && a != b,
                    "no duplicate retired node after ABA boundary");
                pool.finish(a);
                pool.finish(b);
            }
        }
#else
        return false;
#endif
    }
    else if (id == 81)
    {
#if defined(SEMAPHORE_SHARED_COUNTER) && !defined(_WIN32) && !defined(__EMSCRIPTEN__)
        const std::string logicalName = name(id);
        const std::string path = SemaphoreProcessMemory::directory() + "/m_" + logicalName + ".s";
        check(::mkfifo(path.c_str(), 0600) == 0, "create wrong-type shared name");
        bool rejected = false;
        bool preserved = false;
        double elapsed = 0;
        try
        {
            Semaphore owner, opener;
            const double started = now();
            rejected = owner.createProcessResult(logicalName, 2).status != SEMAPHORE_OK &&
                opener.openProcessResult(logicalName).status != SEMAPHORE_OK;
            elapsed = now() - started;
            struct stat info = {};
            preserved = ::lstat(path.c_str(), &info) == 0 && S_ISFIFO(info.st_mode);
        }
        catch (...)
        {
            // 即使异常也尝试移除本用例创建的文件，清理错误单独报告。
            if (::unlink(path.c_str()) != 0)
            {
                std::fprintf(stderr, "FIFO cleanup failed errno=%d\n", errno);
            }
            throw;
        }
        const bool removed = ::unlink(path.c_str()) == 0;
        check(rejected && preserved && removed && elapsed < 1000,
            "wrong-type open does not block or remove another owner's name");
#else
        return false;
#endif
    }
    else if (id == 80)
    {
#ifdef SEMAPHORE_TEST_INSTRUMENTATION
        for (size_t capacity = 1; capacity <= 2; ++capacity)
        {
            SemaphorePoolConfig config;
            config.initialCapacity = capacity;
            config.maxCapacity = static_cast<int64_t>(capacity);
            SemaphoreEventPool pool(config);
            SemaphoreEventRecord* a = nullptr;
            SemaphoreEventRecord* b = nullptr;
            check(pool.begin(a).status == SEMAPHORE_OK, "register first waiter");
            if (capacity == 2)
            {
                check(pool.begin(b).status == SEMAPHORE_OK, "register second waiter before paused finish");
            }
            bool notified = true;
            {
                SemaphoreTestGate gate(109);
                SemaphoreTestThreads threads;
                threads.start([&]() { notified = pool.finish(a); });
                const bool arrived = gate.reached();
                SemaphoreEventRecord* candidate = nullptr;
                const SemaphoreResult acquired = pool.begin(candidate);
                size_t prematureReleased = 0;
                const SemaphoreResult maintained = pool.maintain(prematureReleased);
                const SemaphorePoolStats paused = pool.statistics();
                const SemaphoreResult broadcast = pool.notify(true);
                // 先放行和回收线程，再判定，避免失败异常绕过门闩释放而卡住析构。
                gate.release();
                threads.join();
                check(arrived && acquired.status == SEMAPHORE_CAPACITY && candidate == nullptr &&
                        maintained.status == SEMAPHORE_OK && prematureReleased == 0 &&
                        paused.freeCapacity == 0 && paused.retiredCapacity == 1,
                    "marked waiter remains protected after helper removes it");
                check(!notified && broadcast.committed == (capacity == 2),
                    "cancelled waiter cannot consume later broadcast");
            }
            if (b != nullptr)
            {
                check(pool.finish(b), "remaining waiter received broadcast");
            }
            size_t released = 0;
            pool.maintain(released);
            check(pool.statistics().freeCapacity == capacity && pool.statistics().retiredCapacity == 0,
                "all records recovered after paused waiter leaves");
            check(pool.begin(a).status == SEMAPHORE_OK, "first record reusable");
            if (capacity == 2)
            {
                check(pool.begin(b).status == SEMAPHORE_OK && a != b, "second reusable record is distinct");
                pool.finish(b);
            }
            pool.finish(a);
        }
#else
        return false;
#endif
    }
    else if (id == 78)
    {
#if (!defined(_WIN32) && !defined(__EMSCRIPTEN__)) || defined(SEMAPHORE_TEST_POSIX_NAME)
#ifdef SEMAPHORE_TEST_POSIX_NAME
        DWORD handlesBefore = 0;
        check(SemaphoreTestPosixNameHandles() == 0, "name fixture initially empty after runtime warmup");
        check(::GetProcessHandleCount(::GetCurrentProcess(), &handlesBefore) != FALSE, "name fixture initial handles");
#endif
        Semaphore owner, opener, joiner, replacement, fresh;
        const std::string logicalName = name(id);
        check(owner.createProcessResult(logicalName, 2).status == SEMAPHORE_OK &&
                opener.openProcessResult(logicalName).status == SEMAPHORE_OK &&
                joiner.createProcessResult(logicalName, 2).status == SEMAPHORE_OK,
            "create owner and non-owning references");
        check(!opener.unlinkProcessName() && !joiner.unlinkProcessName(), "only exclusive creator can unlink");
        check(owner.unlinkProcessName() && !owner.unlinkProcessName(), "unlink consumes owner right exactly once");
        check(replacement.createProcessResult(logicalName, 2).status == SEMAPHORE_OK,
            "new generation reuses detached name");
        check(!opener.unlinkProcessName() && !joiner.unlinkProcessName() && !owner.unlinkProcessName(),
            "old references cannot unlink replacement generation");
        check(fresh.openProcessResult(logicalName).status == SEMAPHORE_OK, "replacement name remains discoverable");
        check(owner.processSignalResult().committed && opener.processWait(0) && !fresh.processWait(0),
            "old references retain old counter");
        check(replacement.processSignalResult().committed && fresh.processWait(0) && !opener.processWait(0),
            "replacement counter stays separate");
        check(owner.closeProcessResult().status == SEMAPHORE_OK &&
                opener.closeProcessResult().status == SEMAPHORE_OK && joiner.closeProcessResult().status == SEMAPHORE_OK,
            "old generation close cannot remove new name");
        check(owner.openProcessResult(logicalName).status == SEMAPHORE_OK, "new name survives old close");
        check(replacement.closeProcessResult().status == SEMAPHORE_OK &&
                !opener.tryOpenProcessSemaphore(logicalName),
            "new owner alone removes new name");
        check(fresh.processSignalResult().committed && owner.processWait(0), "unlinked live references remain usable");
        check(owner.closeProcessResult().status == SEMAPHORE_OK && fresh.closeProcessResult().status == SEMAPHORE_OK,
            "all generations release resources");
#ifdef SEMAPHORE_TEST_POSIX_NAME
        DWORD handlesAfter = 0;
        check(SemaphoreTestPosixNameHandles() == 0, "all fixture owned native handles returned");
        check(::GetProcessHandleCount(::GetCurrentProcess(), &handlesAfter) != FALSE, "name fixture final handles");
        std::printf("NAME_HANDLES before=%lu after=%lu fixture=0\n", handlesBefore, handlesAfter);
        check(::GetProcessHandleCount(::GetCurrentProcess(), &handlesAfter) != FALSE && handlesAfter == handlesBefore,
            "all name fixture handles returned");
#endif
#else
        return false;
#endif
    }
    else if (id == 65)
    {
        Semaphore request, reply;
        check(request.prepareSignalAndWait().status == SEMAPHORE_OK &&
                reply.prepareSignalAndWait().status == SEMAPHORE_OK,
            "contended combined preparation");
        std::atomic<int32_t> accepted(0), replied(0);
        SemaphoreTestThreads threads;
        for (int32_t i = 0; i < 4; ++i)
        {
            threads.start(
                /** 执行并发组合计数的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    for (int32_t n = 0; n < 500; ++n)
                    {
                        check(request.wait(5000), "contended request");
                        accepted.fetch_add(1);
                        reply.signal();
                    }
                });
            threads.start(
                /** 执行并发组合计数的线程步骤，捕获状态必须活到线程回收
                */
                [&]()
                {
                    for (int32_t n = 0; n < 500; ++n)
                    {
                        const SemaphoreSignalWaitResult result = request.signalAndWaitResult(reply, 5000);
                        check(result.status == SEMAPHORE_OK && result.waitState == SEMAPHORE_COMMIT_DONE,
                            "contended combined reply");
                        replied.fetch_add(1);
                    }
                });
        }
        threads.join();
        check(accepted.load() == 2000 && replied.load() == 2000 && !request.wait(0) && !reply.wait(0),
            "combined concurrency neither loses nor duplicates permits");
    }
    return true;
}