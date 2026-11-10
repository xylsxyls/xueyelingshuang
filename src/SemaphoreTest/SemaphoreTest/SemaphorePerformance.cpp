#include "SemaphorePerformance.h"
#include "Semaphore.h"
#ifdef SEMAPHORE_TEST_NATIVE_CONTROL
#include "SemaphoreNativeIpc.h"
// 只在原生诊断配置中替换测量对象，正式库及其他测试不受影响
#define Semaphore SemaphoreNativeIpc
#endif
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string>

// 计时程序不启用分配器覆盖或故障钩子；申请统计使用独立配置
#ifdef SEMAPHORE_PERF_ALLOCATION
static std::atomic<bool> s_trackAllocations(false);
static std::atomic<size_t> s_allocationCalls(0);
static std::atomic<size_t> s_allocationBytes(0);

void* operator new(size_t bytes)
{
    void* value = std::malloc(bytes == 0 ? 1 : bytes);
    if (value == nullptr)
    {
        throw std::bad_alloc();
    }
    if (s_trackAllocations.load())
    {
        s_allocationCalls.fetch_add(1);
        s_allocationBytes.fetch_add(bytes);
    }
    return value;
}

void operator delete(void* value) throw()
{
    std::free(value);
}

void* operator new[](size_t bytes)
{
    return ::operator new(bytes);
}

void operator delete[](void* value) throw()
{
    ::operator delete(value);
}
#endif

// 线程启动前初始化，全部计时工作量运行期间保持只读
static double s_microsecondsPerTick = 0;
// 一轮开始前确定接口模式，工作线程启动后保持只读
static bool s_combined = false;
// 析构无法返回的系统清理错误，由整轮结果统一检查
static std::atomic<int32_t> s_cleanupFailures(0);

SemaphorePerformanceThreads::SemaphorePerformanceThreads() :
m_gate(::CreateEventW(nullptr, TRUE, FALSE, nullptr)),
m_cancelled(false),
m_errors(0)
{
    SemaphorePerformance::require(m_gate != nullptr, "create start gate");
    try
    {
        m_threads.reserve(16);
    }
    catch (...)
    {
        if (!::CloseHandle(m_gate))
        {
            s_cleanupFailures.fetch_add(1);
        }
        throw;
    }
}

SemaphorePerformanceThreads::~SemaphorePerformanceThreads()
{
    m_cancelled.store(true);
    if (!::SetEvent(m_gate))
    {
        s_cleanupFailures.fetch_add(1);
    }
    for (size_t i = 0; i < m_threads.size(); ++i)
    {
        if (m_threads[i].joinable())
        {
            m_threads[i].join();
        }
    }
    if (!::CloseHandle(m_gate))
    {
        s_cleanupFailures.fetch_add(1);
    }
}

void SemaphorePerformanceThreads::add(const std::function<void()>& task)
{
    m_threads.emplace_back(
        /** 执行当前测试的线程步骤，捕获状态必须活到线程回收
        */
        [this, task]()
        {
            try
            {
                SemaphorePerformance::require(
                    ::WaitForSingleObject(m_gate, 10000) == WAIT_OBJECT_0, "start gate timeout");
                if (!m_cancelled.load())
                {
                    task();
                }
            }
            catch (...)
            {
                m_errors.fetch_add(1);
                m_cancelled.store(true);
            }
        });
}

void SemaphorePerformanceThreads::start()
{
    SemaphorePerformance::require(::SetEvent(m_gate) != FALSE, "start gate release");
}

void SemaphorePerformanceThreads::join()
{
    for (size_t i = 0; i < m_threads.size(); ++i)
    {
        if (m_threads[i].joinable())
        {
            m_threads[i].join();
        }
    }
    SemaphorePerformance::require(m_errors.load() == 0, "worker failed");
}

bool SemaphorePerformanceThreads::cancelled() const
{
    return m_cancelled.load();
}

void SemaphorePerformance::require(bool value, const char* message)
{
    if (!value)
    {
        throw std::runtime_error(message);
    }
}

__declspec(noinline) uint32_t SemaphorePerformance::stackAlignment(uint32_t depth)
{
    Semaphore sem;
    const uintptr_t address = reinterpret_cast<uintptr_t>(&sem);
    uint32_t failures = address % sizeof(uint64_t) == 0 ? 0 : 1;
#if !defined(SEMAPHORE_TEST_BASELINE) && defined(STATIC_LIB)
    SemaphorePoolConfig config;
    config.initialCapacity = 0;
    config.maxCapacity = 0;
    SemaphoreEventPool pool(config);
    const uintptr_t poolAddress = reinterpret_cast<uintptr_t>(&pool);
    failures += poolAddress % sizeof(uint64_t) == 0 ? 0 : 1;
    std::printf("LAYOUT pool depth=%u address=%p mod8=%u\n", depth, &pool,
        static_cast<unsigned>(poolAddress % 8));
#endif
    std::printf("LAYOUT depth=%u address=%p mod8=%u mod64=%u\n", depth, &sem,
        static_cast<unsigned>(address % 8), static_cast<unsigned>(address % 64));
    if (depth != 0)
    {
        failures += stackAlignment(depth - 1);
    }
    // 递归返回后仍使用本层实例，防止尾递归或生命周期合并掩盖真实布局
    sem.signal();
    require(sem.wait(0) && !sem.wait(0), "alignment count exact");
    return failures;
}

void SemaphorePerformance::alignment()
{
    uint32_t failures = stackAlignment(15);
    std::atomic<uint32_t> workerFailures(0);
    SemaphorePerformanceThreads threads;
    for (uint32_t i = 0; i < 4; ++i)
    {
        threads.add(
            /** 检查当前工作线程的16层栈，失败数只用于独立测试判定
            */
            [&]()
            {
                workerFailures.fetch_add(stackAlignment(15));
            });
    }
    threads.start();
    threads.join();
    failures += workerFailures.load();
    // DLL只消费公开Semaphore接口；内部池的真实栈对齐由静态和白盒配置覆盖。
#if !defined(SEMAPHORE_TEST_BASELINE) && defined(STATIC_LIB)
    const uint32_t objects = 160;
#else
    const uint32_t objects = 80;
#endif
    std::printf("METRIC {\"name\":\"stack_alignment\",\"objects\":%u,\"misaligned\":%u}\n", objects, failures);
    require(failures == 0, "64-bit atomic stack alignment");
}

double SemaphorePerformance::now()
{
    LARGE_INTEGER ticks;
    require(::QueryPerformanceCounter(&ticks) != FALSE, "QPC");
    return ticks.QuadPart * s_microsecondsPerTick;
}

double SemaphorePerformance::cpu()
{
    FILETIME created, ended, kernel, user;
    require(::GetProcessTimes(::GetCurrentProcess(), &created, &ended, &kernel, &user) != FALSE, "process CPU");
    const uint64_t k = (uint64_t(kernel.dwHighDateTime) << 32) | kernel.dwLowDateTime;
    const uint64_t u = (uint64_t(user.dwHighDateTime) << 32) | user.dwLowDateTime;
    return (k + u) / 10000.0;
}

void SemaphorePerformance::latency(const char* name, std::vector<double>& samples)
{
    require(!samples.empty(), "latency sample set");
    std::printf("SAMPLES %s [", name);
    for (size_t i = 0; i < samples.size(); ++i)
    {
        std::printf("%s%.3f", i == 0 ? "" : ",", samples[i]);
    }
    std::printf("]\n");
    std::sort(samples.begin(), samples.end());
    const size_t n = samples.size();
    std::printf(
        "METRIC {\"name\":\"%s\",\"samples\":%u,\"p50_us\":%.3f,\"p95_us\":%.3f,\"p99_us\":%.3f,\"max_us\":%.3f}\n",
        name, static_cast<unsigned>(n), samples[(n * 50 + 99) / 100 - 1], samples[(n * 95 + 99) / 100 - 1],
        samples[(n * 99 + 99) / 100 - 1], samples.back());
    std::fflush(stdout);
}

void SemaphorePerformance::bulk()
{
    Semaphore sem;
    std::printf("LAYOUT bulk address=%p mod8=%u mod64=%u\n", &sem,
        static_cast<unsigned>(reinterpret_cast<uintptr_t>(&sem) % 8),
        static_cast<unsigned>(reinterpret_cast<uintptr_t>(&sem) % 64));
    const int32_t count = 1000000;
    for (int32_t i = 0; i < 10000; ++i)
    {
        sem.signal();
        require(sem.wait(0), "bulk warmup");
    }
    const double started = now();
    for (int32_t i = 0; i < count; ++i)
    {
        sem.signal();
    }
    for (int32_t i = 0; i < count; ++i)
    {
        require(sem.wait(0), "bulk permit");
    }
    const double elapsed = now() - started;
    require(!sem.wait(0), "bulk no surplus");
    std::printf("METRIC {\"name\":\"bulk\",\"permits\":%d,\"permits_per_second\":%.3f,\"elapsed_ms\":%.3f}\n", count,
        count * 1000000.0 / elapsed, elapsed / 1000);
}

void SemaphorePerformance::contention(const char* name, int32_t producers, int32_t consumers)
{
    Semaphore sem;
    std::printf("LAYOUT %s address=%p mod8=%u mod64=%u\n", name, &sem,
        static_cast<unsigned>(reinterpret_cast<uintptr_t>(&sem) % 8),
        static_cast<unsigned>(reinterpret_cast<uintptr_t>(&sem) % 64));
    std::atomic<int32_t> consumed(0);
    SemaphorePerformanceThreads threads;
    const int32_t count = 400000;
    require(producers > 0 && consumers > 0 && count % producers == 0 && count % consumers == 0,
        "contention workload division");
    for (int32_t p = 0; p < producers; ++p)
    {
        threads.add(
            /** 执行当前测试的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                for (int32_t i = 0; i < count / producers && !threads.cancelled(); ++i)
                {
                    sem.signal();
                }
            });
    }
    for (int32_t c = 0; c < consumers; ++c)
    {
        threads.add(
            /** 执行当前测试的线程步骤，捕获状态必须活到线程回收
            */
            [&]()
            {
                int32_t local = 0;
                for (; local < count / consumers && !threads.cancelled(); ++local)
                {
                    require(sem.wait(5000), "contended wait");
                }
                consumed.fetch_add(local);
            });
    }
    const double cpuBefore = cpu();
    const double started = now();
    threads.start();
    threads.join();
    const double elapsed = now() - started;
    const double cpuUsed = cpu() - cpuBefore;
    require(consumed.load() == count && !sem.wait(0), "contended count exact");
    std::printf(
        "METRIC {\"name\":\"%s\",\"permits\":%d,\"permits_per_second\":%.3f,\"elapsed_ms\":%.3f,\"cpu_ms\":%.3f}\n",
        name, count, count * 1000000.0 / elapsed, elapsed / 1000, cpuUsed);
}

void SemaphorePerformance::handoff(bool cold)
{
    Semaphore request, reply, ready;
#ifndef SEMAPHORE_TEST_BASELINE
    if (s_combined)
    {
        require(request.prepareSignalAndWait().status == SEMAPHORE_OK &&
                reply.prepareSignalAndWait().status == SEMAPHORE_OK,
            "local handoff preparation");
    }
#endif
    const int32_t measured = cold ? 300 : 20000;
    const int32_t warmup = cold ? 10 : 1000;
    std::vector<double> samples(measured);
    double observed = 0;
    SemaphorePerformanceThreads threads;
    threads.add(
        /** 执行当前测试的线程步骤，捕获状态必须活到线程回收
        */
        [&]()
        {
            for (int32_t i = 0; i < measured + warmup && !threads.cancelled(); ++i)
            {
                if (cold)
                {
                    ready.signal();
                }
                require(request.wait(5000), "handoff request");
                if (cold)
                {
                    observed = now();
                }
                reply.signal();
            }
        });
    threads.start();
    for (int32_t i = 0; i < measured + warmup; ++i)
    {
        if (cold)
        {
            require(ready.wait(5000), "cold ready");
            ::Sleep(5);
        }
        const double started = now();
#ifndef SEMAPHORE_TEST_BASELINE
        if (s_combined)
        {
            const SemaphoreSignalWaitResult result = request.signalAndWaitResult(reply, 5000);
            require(result.status == SEMAPHORE_OK && result.signalState == SEMAPHORE_COMMIT_DONE &&
                    result.waitState == SEMAPHORE_COMMIT_DONE,
                "local handoff reply");
        }
        else
#endif
        {
            request.signal();
            require(reply.wait(5000), "handoff reply");
        }
        const double elapsed = (cold ? observed : now()) - started;
        if (i >= warmup)
        {
            samples[i - warmup] = elapsed;
        }
    }
    threads.join();
    latency(cold ? "cold_publish_to_observe" : "warm_roundtrip", samples);
}

void SemaphorePerformance::events(int32_t count, bool prepared)
{
    Semaphore event, ready, reply;
#ifndef SEMAPHORE_TEST_BASELINE
    if (prepared)
    {
        require(event.prepareSignalAndWait().status == SEMAPHORE_OK, "event backend preparation");
    }
#endif
    std::atomic<int32_t> generation(0);
    std::atomic<int32_t> rescueTimeouts(0);
    const int32_t measured = 200, warmup = 10;
    std::vector<double> observations(count), samples(measured);
    SemaphorePerformanceThreads threads;
    for (int32_t w = 0; w < count; ++w)
    {
        threads.add(
            /** 执行当前测试的线程步骤，捕获状态必须活到线程回收
            */
            [&, w]()
            {
                for (int32_t round = 1; round <= measured + warmup && !threads.cancelled(); ++round)
                {
                    ready.signal();
                    const double limit = now() + 5000000;
                    while (generation.load() < round && !threads.cancelled())
                    {
                        if (!event.eventWait(100))
                        {
                            rescueTimeouts.fetch_add(1);
                        }
                        require(now() < limit, "event generation deadline");
                    }
                    observations[w] = now();
                    reply.signal();
                }
            });
    }
    threads.start();
    for (int32_t round = 1; round <= measured + warmup; ++round)
    {
        for (int32_t w = 0; w < count; ++w)
        {
            require(ready.wait(5000), "event ready");
        }
        ::Sleep(5);
        const double started = now();
        generation.store(round);
        if (count == 1)
        {
            event.event();
        }
        else
        {
            event.eventAll();
        }
        for (int32_t w = 0; w < count; ++w)
        {
            require(reply.wait(5000), "event reply");
        }
        const double elapsed = *std::max_element(observations.begin(), observations.end()) - started;
        if (round > warmup)
        {
            samples[round - warmup - 1] = elapsed;
        }
    }
    threads.join();
    latency(count == 1 ? "event_one_observed" : "event_all_8_observed", samples);
    std::printf("METRIC {\"name\":\"%s\",\"rescue_timeouts\":%d}\n",
        count == 1 ? "event_one_boundary" : "event_all_boundary", rescueTimeouts.load());
}

void SemaphorePerformance::idle()
{
    Semaphore sem;
    const double cpuBefore = cpu(), started = now();
    require(!sem.wait(2000), "idle timeout");
    const double elapsed = now() - started, used = cpu() - cpuBefore;
    require(elapsed >= 1900000 && elapsed < 4000000, "idle budget");
    std::printf("METRIC {\"name\":\"idle\",\"wall_ms\":%.3f,\"cpu_ms\":%.3f,\"one_core_percent\":%.6f}\n",
        elapsed / 1000, used, used * 100000.0 / elapsed);
}

void SemaphorePerformance::lifecycle()
{
    const int32_t count = 100000;
    for (int32_t i = 0; i < 1000; ++i)
    {
        Semaphore sem;
        sem.signal();
        require(sem.wait(0), "lifecycle warmup");
    }
    DWORD handlesBefore = 0;
    require(::GetProcessHandleCount(::GetCurrentProcess(), &handlesBefore) != FALSE, "lifecycle handles before");
    const double started = now();
    for (int32_t i = 0; i < count; ++i)
    {
        Semaphore sem;
        sem.signal();
        require(sem.wait(0), "lifecycle permit");
    }
    const double elapsed = now() - started;
    DWORD handlesAfter = 0;
    require(::GetProcessHandleCount(::GetCurrentProcess(), &handlesAfter) != FALSE, "lifecycle handles after");
    require(handlesAfter == handlesBefore, "lifecycle handles returned");
    std::printf("METRIC {\"name\":\"lifecycle\",\"iterations\":%d,\"cycle_us\":%.3f,\"object_bytes\":%u}\n", count,
        elapsed / count, static_cast<unsigned>(sizeof(Semaphore)));
    std::printf("METRIC {\"name\":\"lifecycle_resources\",\"handles_before\":%lu,\"handles_after\":%lu}\n",
        handlesBefore, handlesAfter);
}

void SemaphorePerformance::allocations()
{
#ifdef SEMAPHORE_PERF_ALLOCATION
    s_allocationCalls.store(0);
    s_allocationBytes.store(0);
    s_trackAllocations.store(true);
    try
    {
        Semaphore sem;
        s_trackAllocations.store(false);
        std::printf("METRIC {\"name\":\"allocation_constructor\",\"calls\":%u,\"bytes\":%u,\"object_bytes\":%u}\n",
            static_cast<unsigned>(s_allocationCalls.load()), static_cast<unsigned>(s_allocationBytes.load()),
            static_cast<unsigned>(sizeof(Semaphore)));
        const char* phases[] = {"allocation_count_hot", "allocation_event_first", "allocation_event_reuse"};
        for (int32_t phase = 0; phase < 3; ++phase)
        {
            s_allocationCalls.store(0);
            s_allocationBytes.store(0);
            s_trackAllocations.store(true);
            if (phase == 0)
            {
                for (int32_t i = 0; i < 10000; ++i)
                {
                    sem.signal();
                    require(sem.wait(0), "allocation count permit");
                }
            }
            else
            {
                // 事件允许虚假唤醒；这里只统计申请，不把返回false作为测试成功条件
                (void)sem.eventWait(1);
            }
            s_trackAllocations.store(false);
            std::printf("METRIC {\"name\":\"%s\",\"calls\":%u,\"bytes\":%u}\n", phases[phase],
                static_cast<unsigned>(s_allocationCalls.load()), static_cast<unsigned>(s_allocationBytes.load()));
        }
#ifndef SEMAPHORE_TEST_BASELINE
        const SemaphorePoolStats stats = sem.eventPoolStatistics();
        std::printf("METRIC {\"name\":\"event_pool_retained\",\"capacity\":%u,\"free\":%u,\"retired\":%u,\"bytes\":%u}\n",
            static_cast<unsigned>(stats.capacity), static_cast<unsigned>(stats.freeCapacity),
            static_cast<unsigned>(stats.retiredCapacity), static_cast<unsigned>(stats.retainedBytes));
#endif
    }
    catch (...)
    {
        s_trackAllocations.store(false);
        throw;
    }
    s_trackAllocations.store(false);
#endif
}

void SemaphorePerformance::poolCycles(size_t count)
{
#ifdef SEMAPHORE_TEST_POOL_PERFORMANCE
    require(count == 1 || count == 8 || count == 64, "pool cycle batch size");
    SemaphorePoolConfig config;
    config.initialCapacity = count;
    config.maxCapacity = count;
    SemaphoreEventPool pool(config);
    const size_t repetitions = 1000 / count;
    std::vector<double> samples(300);
    for (size_t batch = 0; batch < samples.size() + 30; ++batch)
    {
        const double start = now();
        for (size_t i = 0; i < repetitions; ++i)
        {
            SemaphoreEventRecord* records[64] = {};
            for (size_t n = 0; n < count; ++n)
            {
                require(pool.begin(records[n]).status == SEMAPHORE_OK, "pool cycle registration");
            }
            require(pool.notify(true).committed, "pool cycle notification");
            for (size_t n = count; n != 0; --n)
            {
                require(pool.finish(records[n - 1]), "pool cycle notified record release");
            }
        }
        const double duration = now() - start;
        if (batch >= 30)
        {
            samples[batch - 30] = duration / (repetitions * count);
        }
    }
    const SemaphorePoolStats stats = pool.statistics();
    require(stats.capacity == count && stats.freeCapacity == count && stats.retiredCapacity == 0 &&
            stats.reservedCapacity == 0 && stats.successfulGrowCount == 0,
        "pool cycle capacity and references return without allocation");
    std::printf("POOL record_bytes=%u retained_bytes=%u records_per_sample=%u batch_records=%u\n",
        static_cast<unsigned>(sizeof(SemaphoreEventRecord)), static_cast<unsigned>(stats.retainedBytes),
        static_cast<unsigned>(repetitions * count), static_cast<unsigned>(count));
    latency(count == 1 ? "event_pool_cycle" : (count == 8 ? "event_pool_batch_8" : "event_pool_batch_64"), samples);
#else
    (void)count;
#endif
}

int32_t SemaphorePerformance::peer(const char* name, bool combined)
{
    try
    {
        Semaphore request, reply;
        require(request.tryOpenProcessSemaphore(name), "peer open request");
        require(reply.tryOpenProcessSemaphore(std::string(name) + "r"), "peer open reply");
        require(reply.tryProcessSignal() == 0, "peer ready");
        require(request.processWait(5000), "peer first request");
        for (int32_t i = 0; i < 2100; ++i)
        {
#ifndef SEMAPHORE_TEST_BASELINE
            if (combined && i != 2099)
            {
                const SemaphoreSignalWaitResult result = reply.processSignalAndWaitResult(request, 5000);
                require(result.status == SEMAPHORE_OK && result.signalState == SEMAPHORE_COMMIT_DONE &&
                        result.waitState == SEMAPHORE_COMMIT_DONE,
                    "peer reply and next request");
            }
            else
#endif
            {
                require(reply.tryProcessSignal() == 0, "peer reply");
                if (i != 2099)
                {
                    require(request.processWait(5000), "peer request");
                }
            }
        }
        return 0;
    }
    catch (...)
    {
        return 1;
    }
}

void SemaphorePerformance::ipc()
{
    Semaphore request, reply;
    char name[48];
    std::sprintf(name, "SPPerf%lu", ::GetCurrentProcessId());
    request.createProcessSemaphore(name, 2);
    reply.createProcessSemaphore(std::string(name) + "r", 2);
    char executable[32768];
    require(::GetModuleFileNameA(nullptr, executable, sizeof(executable)) != 0, "self path");
    const std::string command = std::string("\"") + executable +
        (s_combined ? "\" --performance-peer-combined " : "\" --performance-peer ") + name;
    std::vector<char> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(0);
    std::vector<double> samples(2000);
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process = {};
    require(::CreateProcessA(nullptr, &mutableCommand[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
                &startup, &process) != FALSE,
        "spawn IPC peer");
    try
    {
        require(reply.processWait(5000), "IPC ready");
        for (int32_t i = 0; i < 2100; ++i)
        {
            const double started = now();
#ifndef SEMAPHORE_TEST_BASELINE
            if (s_combined)
            {
                const SemaphoreSignalWaitResult result = request.processSignalAndWaitResult(reply, 5000);
                require(result.status == SEMAPHORE_OK && result.signalState == SEMAPHORE_COMMIT_DONE &&
                        result.waitState == SEMAPHORE_COMMIT_DONE,
                    "IPC roundtrip");
            }
            else
#endif
            {
                require(request.tryProcessSignal() == 0 && reply.processWait(5000), "IPC roundtrip");
            }
            const double elapsed = now() - started;
            if (i >= 100)
            {
                samples[i - 100] = elapsed;
            }
        }
        require(::WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0, "IPC peer exit");
        DWORD status = 1;
        require(::GetExitCodeProcess(process.hProcess, &status) != FALSE && status == 0, "IPC peer status");
    }
    catch (...)
    {
        if (::WaitForSingleObject(process.hProcess, 0) == WAIT_TIMEOUT)
        {
            ::TerminateProcess(process.hProcess, 91);
            ::WaitForSingleObject(process.hProcess, 5000);
        }
        if (!::CloseHandle(process.hThread))
        {
            s_cleanupFailures.fetch_add(1);
        }
        if (!::CloseHandle(process.hProcess))
        {
            s_cleanupFailures.fetch_add(1);
        }
        throw;
    }
    const bool threadClosed = ::CloseHandle(process.hThread) != FALSE;
    const bool processClosed = ::CloseHandle(process.hProcess) != FALSE;
    require(threadClosed && processClosed, "IPC process handles closed");
    latency("ipc_roundtrip", samples);
}

int32_t SemaphorePerformance::run(int32_t id, bool combined)
{
    try
    {
        LARGE_INTEGER frequency;
        require(::QueryPerformanceFrequency(&frequency) != FALSE && frequency.QuadPart > 0, "QPC frequency");
        s_microsecondsPerTick = 1000000.0 / frequency.QuadPart;
        s_combined = combined;
        std::printf("ENV pid=%lu logical_processors=%u sizeof_semaphore=%u\n", ::GetCurrentProcessId(),
            std::thread::hardware_concurrency(), static_cast<unsigned>(sizeof(Semaphore)));
#ifdef SEMAPHORE_PERF_ALLOCATION
        allocations();
#else
        if (id == 79)
        {
#ifdef SEMAPHORE_TEST_POOL_PERFORMANCE
            poolCycles(1);
            poolCycles(8);
            poolCycles(64);
#else
            return 77;
#endif
        }
        else if (id == 71)
        {
            alignment();
        }
        else if (id == 72)
        {
            events(1, combined);
            events(8, combined);
        }
        else if (id == 55 || id == 73)
        {
            bulk();
            contention("spsc_1p1c", 1, 1);
            contention("mpsc_4p1c", 4, 1);
            contention("mpmc_4p4c", 4, 4);
            contention("mpmc_8p8c", 8, 8);
            contention("mpmc_16p16c", 16, 16);
            if (id == 55)
            {
                handoff(false);
                handoff(true);
                events(1);
                events(8);
                ipc();
                idle();
            }
            lifecycle();
        }
        else if (id == 57 || id == 58 || id == 74)
        {
            ipc();
        }
        else
        {
            handoff(false);
            handoff(true);
            ipc();
            idle();
        }
#endif
#ifdef SEMAPHORE_TEST_NATIVE_CONTROL
        require(SemaphoreNativeIpc::nativeCleanupFailures() == 0, "native cleanup errors");
#endif
        require(s_cleanupFailures.load() == 0, "benchmark cleanup errors");
        std::printf("PASS id=%d all performance workload invariants\n", id);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL id=%d %s\n", id, error.what());
        return 1;
    }
}