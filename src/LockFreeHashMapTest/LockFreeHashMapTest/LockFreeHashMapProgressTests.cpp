#include "LockFreeHashMapProgressTests.h"
#include "LockFreeHashMap.h"
#include "LockFreeHashMapTestThread.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <vector>
#include <string>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

LockFreeHashMapProgressSample::LockFreeHashMapProgressSample() :
m_calls(0),
m_entered(0),
m_completed(0),
m_windowCompleted(0),
m_logicalMisses(0),
m_failures(0),
m_maxCallMs(0),
m_maxProgressGapMs(0),
m_cpuMs(0),
m_startDelayMs(0),
m_firstSuccessMs(-1),
m_cpuCycles(0),
m_priority(0)
{

}

size_t LockFreeHashMapProgressCollision::operator()(uint64_t key) const
{
    (void)key;
    return 0;
}

/** 获取当前线程累计 CPU 时间
@return Windows 累计 CPU 毫秒数；其他平台为 -1，查询失败向入口抛异常
*/
static double ProgressCpuMs()
{
#if defined(_WIN32)
    FILETIME created, exited, kernel, user;
    if (!GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user))
    {
        throw std::runtime_error("GetThreadTimes failed");
    }
    uint64_t total = (uint64_t(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
        (uint64_t(user.dwHighDateTime) << 32) + user.dwLowDateTime;
    return double(total) / 10000.0;
#else
    return -1;
#endif
}

/** 在测试启动、停止等待及负例延迟中让出 CPU，不使用 v120 的 Concurrency 调度等待
@param [in] milliseconds 相对等待毫秒数，不保证实际唤醒时限
*/
static void ProgressSleep(unsigned milliseconds)
{
#if defined(_WIN32)
    Sleep(milliseconds);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}

/** 读取单调时间；v120 的 steady_clock 继承 system_clock，因此 Windows 显式用 QPC
@return 单调秒数，查询失败向测试入口抛异常
*/
static double ProgressNow()
{
#if defined(_WIN32)
    LARGE_INTEGER value, frequency;
    if (!QueryPerformanceCounter(&value) || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
    {
        throw std::runtime_error("progress QPC failed");
    }
    return double(value.QuadPart) / double(frequency.QuadPart);
#else
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

/** 获取当前线程的 CPU 周期观测，不将周期数换算成固定频率的时间
@return Windows 周期计数；其他平台为零，查询失败向测试入口抛异常
*/
static uint64_t ProgressCycles()
{
#if defined(_WIN32)
    ULONG64 cycles = 0;
    if (!QueryThreadCycleTime(GetCurrentThread(), &cycles))
    {
        throw std::runtime_error("QueryThreadCycleTime failed");
    }
    return cycles;
#else
    return 0;
#endif
}

/** 同一 API 的线程共同计量；背景线程只提供数据，不参加公平份额比较
@param [in] operation 受测操作编号，0 至 7；8 为标准分配器对照，9 为不调用 Map 或分配器的调度对照
@param [in] oneCpu 受测组是否共用一个 CPU，背景供给使用另一 CPU
@param [in] loaded 是否使用十二条工作线程及两条同优先级计算干扰线程
@param [in] delayed 仅夹具自检：1 延迟首线程入场，2 延迟首次调用返回，0 不注入
@return 每线程均成功执行、无异常失败且资源回收正确时为 true
*/
template <typename Hash>
static bool ProgressRun(int32_t operation, bool oneCpu, bool loaded, int32_t delayed = 0)
{
    typedef LockFreeHashMap<uint64_t, uint64_t, Hash> Map;
    const size_t workers = loaded ? 12 : 6;
    const char* names[] = { "set_hot", "insert", "erase", "take", "find", "clear", "set_collision", "get", "allocator_control", "scheduler_control" };
    // 阶段记录在计时窗口外，超时时可区分初始化、运行及资源收尾位置。
    std::cout << "PROGRESS_BEGIN api=" << names[operation] << " cpu=" << (oneCpu ? "one" : "all")
        << " loaded=" << loaded << " injected_delay=" << delayed << std::endl;
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 8192;
    Map map(1024, config);
    if (map.status() != LockFreeHashMapOk)
    {
        return false;
    }
    for (uint64_t key = 0; key < 64; ++key)
    {
        if (!map.trySet(key, key + 1).m_committed)
        {
            return false;
        }
    }
    bool background = operation >= 1 && operation <= 3;
    size_t total = workers + (background ? 1 : 0);
    size_t participants = total + (loaded ? 2 : 0);
    // 消费者各有一个不会被其他线程取走的首条目；插入者的首键保持不存在。
    // 后续调用仍争用原有 64 个热点键，不能把 Exists/NotFound 本身认作调度失败。
    if (operation == 2 || operation == 3)
    {
        for (size_t i = 0; i < workers; ++i)
        {
            if (!map.trySet(64 + i, 65 + i).m_committed)
            {
                return false;
            }
        }
    }
    std::vector<LockFreeHashMapProgressSample> samples(total);
    // take 的唯一性记录预留在计时前，分配失败由 main 捕获。
    for (size_t i = 0; i < workers; ++i)
    {
        samples[i].m_taken.reserve(operation == 3 ? 1000000 : 0);
    }
    std::vector<LockFreeHashMapTestThread> threads;
    threads.reserve(participants);
    std::atomic<bool> start(false), stop(false);
    std::atomic<bool> fixtureFailed(false);
    std::atomic<size_t> ready(0);
    // 起点通过 start 的发布获取传递；Windows 用统一事件唤醒，避免定时轮询的唤醒偏差。
#if defined(_WIN32)
    std::unique_ptr<void, decltype(&CloseHandle)> startEvent(CreateEventW(nullptr, TRUE, FALSE, nullptr), &CloseHandle);
    if (!startEvent)
    {
        return false;
    }
#endif
    double windowStart = 0;
    const double windowSeconds = 1.2;
    unsigned long long cpuMask = 0;
    unsigned long long supplyMask = 0;
#if defined(_WIN32)
    DWORD_PTR processMask = 0, systemMask = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask))
    {
        return false;
    }
    cpuMask = static_cast<unsigned long long>(processMask & (~processMask + 1));
    DWORD_PTR remaining = processMask & ~static_cast<DWORD_PTR>(cpuMask);
    supplyMask = static_cast<unsigned long long>(remaining & (~remaining + 1));
    // 受测的六条同 API 线程共用一个 CPU，供给在另一 CPU 持续可运行。
    // 否则几十个槽会在一次时间片中耗尽，后续线程的 NotFound 不能说明调度不公平。
    if (oneCpu && background && supplyMask == 0)
    {
        throw std::runtime_error("supply fixture needs two allowed CPUs");
    }
#else
    if (oneCpu)
    {
        return false;
    }
    (void)cpuMask;
    (void)supplyMask;
#endif
    try
    {
        for (size_t index = 0; index < participants; ++index)
        {
            // 借用夹具直至 join；只写当前线程的样本，异常设置停止标志并由汇总判失败。
            threads.push_back(LockFreeHashMapTestThread([&, index]()
            {
                try
                {
#if defined(_WIN32)
                    if (oneCpu && SetThreadAffinityMask(GetCurrentThread(),
                        static_cast<DWORD_PTR>(background && index == workers ? supplyMask : cpuMask)) == 0)
                    {
                        throw std::runtime_error("SetThreadAffinityMask failed");
                    }
                    int32_t priority = GetThreadPriority(GetCurrentThread());
                    if (priority != THREAD_PRIORITY_NORMAL)
                    {
                        throw std::runtime_error("unexpected worker priority");
                    }
#endif
                    double cpuStart = ProgressCpuMs();
                    uint64_t cyclesStart = ProgressCycles();
                    ready.fetch_add(1);
                    while (!start.load())
                    {
#if defined(_WIN32)
                        DWORD waitResult = WaitForSingleObject(startEvent.get(), 100);
                        if (waitResult != WAIT_OBJECT_0 && waitResult != WAIT_TIMEOUT)
                        {
                            throw std::runtime_error("startup event wait failed");
                        }
#else
                        ProgressSleep(1);
#endif
                    }
                    double deadline = windowStart + windowSeconds;
                    if (index >= total)
                    {
                        // 可控 CPU 干扰不调用 Map，不修改其他进程的亲和性或优先级。
                        volatile uint64_t work = index + 1;
                        while (!stop.load(std::memory_order_relaxed) && ProgressNow() < deadline)
                        {
                            for (size_t i = 0; i < 256; ++i)
                            {
                                work = work * 1664525 + 1013904223;
                            }
                        }
                        return;
                    }
                    LockFreeHashMapProgressSample& sample = samples[index];
#if defined(_WIN32)
                    sample.m_priority = priority;
#endif
                    if (index == 0 && delayed == 1)
                    {
                        ProgressSleep(1300);
                    }
                    sample.m_startDelayMs = (ProgressNow() - windowStart) * 1000;
                    double lastProgress = windowStart;
                    uint64_t sequence = 0;
                    while (!stop.load(std::memory_order_relaxed))
                    {
                        double before = ProgressNow();
                        if (before >= deadline)
                        {
                            break;
                        }
                        uint64_t key = (sequence + index * 11) & 63;
                        if (background && index < workers && sequence == 0)
                        {
                            key = 64 + index;
                        }
                        uint64_t value = (static_cast<uint64_t>(index + 1) << 56) | (++sequence);
                        typename Map::Snapshot snapshot;
                        ++sample.m_entered;
                        if (index == 0 && delayed == 2 && sample.m_entered == 1)
                        {
                            ProgressSleep(1300);
                        }
                        LockFreeHashMapResult result;
                        if (index == workers)
                        {
                            result = operation == 1 ? map.tryErase(key) : map.trySet(key, value);
                        }
                        else if (operation == 0 || operation == 6)
                        {
                            result = map.trySet(operation == 0 ? 0 : key, value);
                        }
                        else if (operation == 1)
                        {
                            result = map.insertIfAbsent(key, value, snapshot);
                        }
                        else if (operation == 2)
                        {
                            result = map.tryErase(key);
                        }
                        else if (operation == 3)
                        {
                            result = map.take(key, snapshot);
                        }
                        else if (operation == 4)
                        {
                            result = map.find(key, snapshot);
                        }
                        else if (operation == 5)
                        {
                            result = map.tryClear();
                        }
                        else if (operation == 9)
                        {
                            // 独立整数运算不借用 Map、堆或用户互斥锁，用于分离系统调度延迟。
                            volatile uint64_t work = value;
                            work = work * 1664525 + 1013904223;
                            if (work != value * 1664525 + 1013904223)
                            {
                                ++sample.m_failures;
                            }
                        }
                        else if (operation == 8)
                        {
                            // 与 find 一样创建拥有副本，计时区间内不调用 Map。
                            snapshot.m_value = std::make_shared<uint64_t>(value);
                            if (!snapshot.m_value || *snapshot.m_value != value)
                            {
                                ++sample.m_failures;
                            }
                        }
                        else
                        {
                            uint64_t readValue = 0;
                            result = LockFreeHashMapResult(map.get(key, readValue) ? LockFreeHashMapOk : LockFreeHashMapNotFound);
                            if (result.m_status == LockFreeHashMapOk && readValue != key + 1)
                            {
                                ++sample.m_failures;
                            }
                        }
                        double after = ProgressNow();
                        ++sample.m_calls;
                        sample.m_maxCallMs = (std::max)(sample.m_maxCallMs, (after - before) * 1000);
                        if (result.m_status == LockFreeHashMapOk)
                        {
                            ++sample.m_completed;
                            if (after <= deadline)
                            {
                                ++sample.m_windowCompleted;
                            }
                            if (sample.m_firstSuccessMs < 0)
                            {
                                sample.m_firstSuccessMs = (after - windowStart) * 1000;
                            }
                            sample.m_maxProgressGapMs = (std::max)(sample.m_maxProgressGapMs, (after - lastProgress) * 1000);
                            lastProgress = after;
                            if (index < workers && operation == 3)
                            {
                                if (!snapshot.m_value)
                                {
                                    ++sample.m_failures;
                                }
                                else
                                {
                                    sample.m_taken.push_back(*snapshot.m_value);
                                }
                            }
                            if (index < workers && operation == 4 && (!snapshot.m_value || *snapshot.m_value != key + 1))
                            {
                                ++sample.m_failures;
                            }
                        }
                        else if (result.m_status == LockFreeHashMapNotFound || result.m_status == LockFreeHashMapExists)
                        {
                            ++sample.m_logicalMisses;
                        }
                        else
                        {
                            ++sample.m_failures;
                        }
                    }
                    sample.m_maxProgressGapMs = (std::max)(sample.m_maxProgressGapMs, (deadline - lastProgress) * 1000);
                    sample.m_cpuMs = cpuStart < 0 ? -1 : ProgressCpuMs() - cpuStart;
                    sample.m_cpuCycles = ProgressCycles() - cyclesStart;
                }
                catch (...)
                {
                    fixtureFailed.store(true);
                    if (index < total)
                    {
                        ++samples[index].m_failures;
                    }
                    stop.store(true);
                }
            }));
        }
        double waiting = ProgressNow();
        while (ready.load() != participants && !stop.load())
        {
            if (ProgressNow() - waiting > 10)
            {
                stop.store(true);
            }
            ProgressSleep(1);
        }
        std::cout << "PROGRESS_STARTED api=" << names[operation] << " cpu=" << (oneCpu ? "one" : "all")
            << " loaded=" << loaded << " injected_delay=" << delayed << std::endl;
        windowStart = ProgressNow();
        start.store(true);
#if defined(_WIN32)
        if (!SetEvent(startEvent.get()))
        {
            throw std::runtime_error("startup event release failed");
        }
#endif
        while (!stop.load() && ProgressNow() < windowStart + windowSeconds)
        {
            ProgressSleep(1);
        }
        stop.store(true);
        std::cout << "PROGRESS_STOPPED api=" << names[operation] << " cpu=" << (oneCpu ? "one" : "all")
            << " loaded=" << loaded << " injected_delay=" << delayed << std::endl;
    }
    catch (...)
    {
        stop.store(true);
        start.store(true);
#if defined(_WIN32)
        // 即使事件唤醒失败，有限等待也会重新检查 start，异常收尾不会永远卡在 join。
        SetEvent(startEvent.get());
#endif
        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].join();
        }
        throw;
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    std::cout << "PROGRESS_JOINED api=" << names[operation] << " cpu=" << (oneCpu ? "one" : "all")
        << " loaded=" << loaded << " injected_delay=" << delayed << std::endl;
    bool passed = !fixtureFailed.load() && LockFreeHashMapTestThread::failures() == 0 && ready.load() == participants;
    uint64_t completed = 0, minimum = UINT64_MAX, maximum = 0, failures = 0;
    double worstCall = 0, worstGap = 0;
    std::vector<uint64_t> taken;
    for (size_t i = 0; i < total; ++i)
    {
        const LockFreeHashMapProgressSample& s = samples[i];
        // 相同 API 的每条线程必须实际成功；逻辑失败不能冒充成功推进。
        bool progressed = s.m_windowCompleted != 0;
        if (delayed != 0 && i == 0)
        {
            // 负例须准确命中注入原因；设施异常或别的线程失败不能冒充检出成功。
            uint64_t expectedCalls = delayed == 1 ? 0 : 1;
            progressed = s.m_windowCompleted == 0 && s.m_entered == expectedCalls &&
                s.m_completed == expectedCalls && (delayed == 1 ? s.m_startDelayMs >= 1200 : s.m_firstSuccessMs >= 1200);
        }
        passed = passed && progressed && s.m_entered == s.m_calls && s.m_failures == 0;
        std::cout << "THREAD api=" << names[operation] << " cpu=" << (oneCpu ? "one" : "all") << " id=" << i
            << " loaded=" << loaded << " injected_delay=" << delayed << " background=" << (background && i == workers)
            << " entered=" << s.m_entered << " calls=" << s.m_calls << " completed=" << s.m_completed
            << " window_completed=" << s.m_windowCompleted << " start_delay_ms=" << s.m_startDelayMs
            << " first_success_ms=" << s.m_firstSuccessMs << " cpu_cycles=" << s.m_cpuCycles
            << " logical_miss=" << s.m_logicalMisses << " failures=" << s.m_failures << " max_call_ms=" << s.m_maxCallMs
            << " max_gap_ms=" << s.m_maxProgressGapMs << " cpu_ms=" << s.m_cpuMs << " priority=" << s.m_priority << '\n';
        failures += s.m_failures;
        if (i == workers)
        {
            continue;
        }
        completed += s.m_completed;
        minimum = (std::min)(minimum, s.m_completed);
        maximum = (std::max)(maximum, s.m_completed);
        worstCall = (std::max)(worstCall, s.m_maxCallMs);
        worstGap = (std::max)(worstGap, s.m_maxProgressGapMs);
        taken.insert(taken.end(), s.m_taken.begin(), s.m_taken.end());
    }
    std::sort(taken.begin(), taken.end());
    passed = passed && std::adjacent_find(taken.begin(), taken.end()) == taken.end();
    map.clear();
    for (int32_t i = 0; i < 256; ++i)
    {
        map.maintain(1024);
    }
    LockFreeHashMapPoolStats stats = {};
    passed = passed && map.poolStats(stats).m_status == LockFreeHashMapOk && stats.occupied == 0 && stats.retired == 0;
    map.clearAndFreeMemory();
    passed = passed && map.poolStats(stats).m_status == LockFreeHashMapOk && stats.capacity == 0;
    std::cout << "PROGRESS api=" << names[operation] << " cpu=" << (oneCpu ? "one" : "all")
        << " loaded=" << loaded << " injected_delay=" << delayed << " workers=" << workers << " window_ms=1200"
        << " completed=" << completed << " min=" << minimum << " max=" << maximum
        << " min_max=" << double(minimum) / double(maximum ? maximum : 1) << " max_call_ms=" << worstCall
        << " max_gap_ms=" << worstGap << " failures=" << failures << " passed=" << passed << std::endl;
    return passed;
}

/** 有界负载不证明数学上的无饥饿；进程超时由外部 C++ 控制器执行
@param [in] argc 参数数量
@param [in] argv 可选 allocator-control 或 scheduler-control，单独运行分配器或纯调度对照
@return 0 全部通过，1 检查失败，2 测试设施异常
*/
int main(int argc, char** argv)
{
    try
    {
        int32_t failed = 0, checks = 0;
        std::cout << std::setprecision(9);
#if defined(_WIN32)
        // 记录进程级调度约束，避免把测试宿主的 CPU 限额误判为 Map 内部竞争。
        BOOL inJob = FALSE;
        BOOL jobKnown = IsProcessInJob(GetCurrentProcess(), nullptr, &inJob);
        JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpuRate = {};
        BOOL rateKnown = QueryInformationJobObject(nullptr, JobObjectCpuRateControlInformation,
            &cpuRate, sizeof(cpuRate), nullptr);
        DWORD rateError = rateKnown ? 0 : GetLastError();
        std::cout << "PROGRESS_ENV process_priority=" << GetPriorityClass(GetCurrentProcess())
            << " job_known=" << jobKnown << " in_job=" << inJob << " rate_known=" << rateKnown
            << " rate_flags=" << cpuRate.ControlFlags << " cpu_rate=" << cpuRate.CpuRate
            << " rate_error=" << rateError << std::endl;
#endif
        bool allocatorOnly = argc == 2 && std::string(argv[1]) == "allocator-control";
        bool schedulerOnly = argc == 2 && std::string(argv[1]) == "scheduler-control";
        bool controlOnly = allocatorOnly || schedulerOnly;
        int32_t controlOperation = schedulerOnly ? 9 : 8;
        for (int32_t delayed = 1; !controlOnly && delayed <= 2; ++delayed)
        {
            // 自检必须拒绝零次入场，以及截止后才成功的样本，不改变实际用例的判定。
            bool rejected = ProgressRun<std::hash<uint64_t>>(0, false, false, delayed);
            ++checks;
            failed += rejected ? 0 : 1;
            std::cout << "FIXTURE_SELFTEST delay=" << delayed << " rejected=" << rejected << std::endl;
        }
#if defined(_WIN32)
        const int32_t cpuModes = 2;
#else
        const int32_t cpuModes = 1;
#endif
        for (int32_t load = 0; load < 2; ++load)
        {
            for (int32_t cpu = 0; cpu < cpuModes; ++cpu)
            {
                for (int32_t operation = controlOnly ? controlOperation : 0;
                    operation < (controlOnly ? controlOperation + 1 : 8); ++operation)
                {
                    bool passed = operation == 6 ? ProgressRun<LockFreeHashMapProgressCollision>(operation, cpu != 0, load != 0) :
                        ProgressRun<std::hash<uint64_t>>(operation, cpu != 0, load != 0);
                    ++checks;
                    if (!passed)
                    {
                        ++failed;
                    }
                }
            }
        }
        std::cout << "MODULE_RESULT checks=" << checks << " failed=" << failed << std::endl;
        return failed ? 1 : 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "progress test exception" << std::endl;
    }
    return 2;
}