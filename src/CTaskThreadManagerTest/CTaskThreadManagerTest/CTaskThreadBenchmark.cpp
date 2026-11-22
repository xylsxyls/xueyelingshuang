#include "CTaskThreadBenchmark.h"
#include "PlatformTest/PlatformTestAPI.h"
#include "BlackBoxTestSupport.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#endif

CTaskBenchmarkState::CTaskBenchmarkState(size_t producers, size_t count, int32_t levels, uint32_t work) :
m_producers(producers),
m_count(count),
m_levels(levels),
m_work(work),
m_expected(producers * count),
m_seen(producers * count, 0),
m_next(producers * static_cast<size_t>(levels), 0),
m_latency(producers * count, UINT64_MAX),
m_start(false),
m_stop(false),
m_ready(0),
m_accepted(0),
m_completed(0),
m_retries(0),
m_errors(0),
m_submitTime(0),
m_finishTime(0)
{
    for (size_t i = 0; i < m_expected.size(); ++i)
    {
        m_expected[i] = digest(static_cast<uint64_t>(i) + 1, m_work);
    }
}

CTaskBenchmarkState::~CTaskBenchmarkState()
{
    m_stop.store(true, std::memory_order_release);
    CTaskThreadManager::instance().uninitAll();
}

uint64_t CTaskBenchmarkState::digest(uint64_t seed, uint32_t work)
{
    for (uint32_t i = 0; i < work; ++i)
    {
        seed = (seed ^ (seed >> 17)) * UINT64_C(11400714819323198485) + i;
    }
    return seed;
}

uint64_t CTaskBenchmarkState::now()
{
#ifdef _WIN32
    LARGE_INTEGER value, frequency;
    if (!QueryPerformanceCounter(&value) || !QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0)
    {
        throw std::runtime_error("high resolution clock unavailable");
    }
    const uint64_t ticks = static_cast<uint64_t>(value.QuadPart);
    const uint64_t rate = static_cast<uint64_t>(frequency.QuadPart);
    return ticks / rate * UINT64_C(1000000000) + (ticks % rate) * UINT64_C(1000000000) / rate;
#else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch())
            .count());
#endif
}

void CTaskBenchmarkState::submitted(uint64_t timestamp)
{
    uint64_t previous = m_submitTime.load(std::memory_order_relaxed);
    while (previous < timestamp && !m_submitTime.compare_exchange_weak(previous, timestamp))
    {

    }
}

CTaskBenchmarkItem::CTaskBenchmarkItem(CTaskBenchmarkState& state, size_t producer, size_t sequence, uint64_t timestamp) :
m_state(&state),
m_producer(producer),
m_sequence(sequence),
m_timestamp(timestamp)
{

}

void CTaskBenchmarkItem::doTask()
{
    const size_t index = m_producer * m_state->m_count + m_sequence;
    if (m_state->m_seen[index]++ != 0 ||
        CTaskBenchmarkState::digest(static_cast<uint64_t>(index) + 1, m_state->m_work) != m_state->m_expected[index])
    {
        ++m_state->m_errors;
    }
    const size_t levels = static_cast<size_t>(m_state->m_levels);
    const size_t stream = m_producer * levels + m_sequence % levels;
    if (m_state->m_next[stream]++ != m_sequence / levels)
    {
        ++m_state->m_errors;
    }
    if (m_timestamp != 0)
    {
        m_state->m_latency[index] = CTaskBenchmarkState::now() - m_timestamp;
    }
    if (m_state->m_completed.fetch_add(1, std::memory_order_acq_rel) + 1 == m_state->m_expected.size())
    {
        m_state->m_finishTime.store(CTaskBenchmarkState::now(), std::memory_order_release);
    }
}

CTaskBenchmarkProducer::CTaskBenchmarkProducer(CTaskBenchmarkState& state, const std::shared_ptr<CTaskThread>& target,
    size_t producer, bool lookup) :
m_state(&state),
m_target(target),
m_producer(producer),
m_lookup(lookup)
{

}

void CTaskBenchmarkProducer::stopTask()
{
    m_state->m_stop.store(true, std::memory_order_release);
}

void CTaskBenchmarkProducer::doTask()
{
    try
    {
        m_state->m_ready.fetch_add(1, std::memory_order_release);
        while (!m_state->m_start.load(std::memory_order_acquire) && !m_state->m_stop.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
        size_t sequence = 0;
        for (; sequence < m_state->m_count && !m_state->m_stop.load(std::memory_order_acquire); ++sequence)
        {
            const uint64_t timestamp = (sequence & 63) == 0 ? CTaskBenchmarkState::now() : 0;
            if (m_lookup)
            {
                const uint32_t id = m_state->m_targets[(sequence + m_producer) % m_state->m_targets.size()];
                const std::shared_ptr<CTaskThread> found = CTaskThreadManager::instance().getThreadInterface(id);
                if (!found || found->threadId() != id)
                {
                    ++m_state->m_errors;
                }
                if (timestamp != 0)
                {
                    m_state->m_latency[m_producer * m_state->m_count + sequence] = CTaskBenchmarkState::now() - timestamp;
                }
            }
            else
            {
                std::shared_ptr<CTask> item(new CTaskBenchmarkItem(*m_state, m_producer, sequence, timestamp));
                const int32_t level = static_cast<int32_t>(sequence % m_state->m_levels) + 1;
                const CTaskThreadResult result = m_target->postTaskResult(item, level);
                if (!result.m_committed || result.m_status != CTaskThreadOk)
                {
                    throw std::runtime_error("ordinary benchmark post failed; external retries would hide a contract failure");
                }
            }
        }
        m_state->m_accepted.fetch_add(sequence, std::memory_order_relaxed);
        if (sequence != m_state->m_count)
        {
            // 提前终止不能把计划数量记为实际接纳，也不能让查询模式伪造完整完成数。
            ++m_state->m_errors;
            m_state->m_stop.store(true, std::memory_order_release);
            return;
        }
        const uint64_t timestamp = CTaskBenchmarkState::now();
        m_state->submitted(timestamp);
        if (m_lookup && m_state->m_completed.fetch_add(m_state->m_count, std::memory_order_acq_rel) + m_state->m_count == m_state->m_expected.size())
        {
            m_state->m_finishTime.store(CTaskBenchmarkState::now(), std::memory_order_release);
        }
    }
    catch (...)
    {
        ++m_state->m_errors;
        m_state->m_stop = true;
    }
}

/** 有截止时间地等待基准条件
@param [in] condition 成功条件
@param [in,out] state 错误及停止状态
*/
static void BenchmarkWait(const std::function<bool()>& condition, CTaskBenchmarkState& state)
{
    const uint64_t deadline = CTaskBenchmarkState::now() + UINT64_C(120000000000);
    while (!condition())
    {
        if (state.m_errors.load() != 0 || CTaskBenchmarkState::now() >= deadline)
        {
            state.m_stop = true;
            throw std::runtime_error("benchmark error or timeout");
        }
        std::this_thread::yield();
    }
}

/** 排空并注销被测线程，验证当前接口的完成结果
@param [in,out] manager 被测管理器
@param [in] id 本次基准拥有的线程编号
*/
static void BenchmarkDrain(CTaskThreadManager& manager, uint32_t id)
{
    const std::shared_ptr<CTaskThread> thread = manager.getThreadInterface(id);
    if (!thread || manager.waitForEnd(id).m_status != CTaskThreadOk)
    {
        throw std::runtime_error("benchmark thread drain failed");
    }
    if (thread->statistics().m_errors != 0)
    {
        throw std::runtime_error("benchmark thread reported internal errors");
    }
}

/** 查询进程 CPU 和资源观测
@param [out] cpuNanoseconds CPU 时间
@param [out] privateBytes 私有字节
@param [out] peakWorkingBytes 历史峰值工作集
@param [out] handles 进程句柄数
*/
static void BenchmarkResources(uint64_t& cpuNanoseconds, uint64_t& privateBytes, uint64_t& peakWorkingBytes, uint64_t& handles)
{
    cpuNanoseconds = privateBytes = peakWorkingBytes = handles = 0;
#ifdef _WIN32
    FILETIME creation, exit, kernel, user;
    PROCESS_MEMORY_COUNTERS_EX memory = {};
    memory.cb = sizeof(memory);
    DWORD count = 0;
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user) ||
        !GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)) ||
        !GetProcessHandleCount(GetCurrentProcess(), &count))
    {
        throw std::runtime_error("process resource query failed");
    }
    cpuNanoseconds = ((static_cast<uint64_t>(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
                         (static_cast<uint64_t>(user.dwHighDateTime) << 32) + user.dwLowDateTime) *
        100;
    privateBytes = memory.PrivateUsage;
    peakWorkingBytes = memory.PeakWorkingSetSize;
    handles = count;
#endif
}

/** 测量公开线程池初始化和排空关闭，各阶段单独计时；预热及状态检查在计时区间外
@param [in] rounds 重建次数，至少一次
@param [in] path 本轮 CSV 输出路径
*/
static void BenchmarkPoolLifecycle(size_t rounds, const char* path)
{
    CTaskThreadManager& manager = CTaskThreadManager::instance();
    CTaskThreadPoolConfig config;
    config.m_initialThreads = 1;
    config.m_maximumThreads = 1;
    uint64_t initNanoseconds = 0;
    uint64_t shutdownNanoseconds = 0;
    for (size_t pass = 0; pass <= rounds; ++pass)
    {
        CTaskThreadPool pool;
        const uint64_t start = CTaskBenchmarkState::now();
        const CTaskThreadResult opened = pool.initialize(config);
        const uint64_t initialized = CTaskBenchmarkState::now();
        bbCheck(opened.m_status == CTaskThreadOk && pool.statistics().m_threads == 1, "pool startup failed");
        const uint64_t closing = CTaskBenchmarkState::now();
        const CTaskThreadResult closed = pool.shutdown(true);
        const uint64_t stopped = CTaskBenchmarkState::now();
        bbCheck(closed.m_status == CTaskThreadOk && manager.count() == 0, "pool failed to release its threads");
        if (pass != 0)
        {
            initNanoseconds += initialized - start;
            shutdownNanoseconds += stopped - closing;
        }
    }
    std::ofstream report(path, std::ios::binary | std::ios::trunc);
    report << "iterations,init_us,shutdown_us\n" << std::fixed << std::setprecision(6) << rounds << ','
        << static_cast<double>(initNanoseconds) / rounds / 1000.0 << ','
        << static_cast<double>(shutdownNanoseconds) / rounds / 1000.0 << '\n';
    report.close();
    bbCheck(static_cast<bool>(report), "cannot write pool lifecycle benchmark");
}

int runTaskThreadBenchmark(int argc, char** argv)
{
    try
    {
        if (argc == 1)
        {
            // 预先建立唯一目标并持有它，计时只覆盖按 ID 查询、释放局部引用及结果累计。
            CTaskThreadPool pool;
            CTaskThreadPoolConfig config;
            config.m_initialThreads = 1;
            config.m_maximumThreads = 1;
            bbCheck(pool.initialize(config).m_status == CTaskThreadOk, "lookup fixture failed");
            CTaskThreadAllocator allocator(pool);
            uint32_t id = 0;
            std::shared_ptr<CTaskThread> expected;
            bbCheck(allocator.select(id, expected).m_status == CTaskThreadOk && expected, "lookup selection failed");
            CTaskThreadManager& manager = CTaskThreadManager::instance();
            LibraryBenchmark::run("getThreadInterface.single", [&](uint32_t count) -> uint64_t
            {
                uint64_t matches = 0;
                for (uint32_t i = 0; i < count; ++i)
                {
                    matches += manager.getThreadInterface(id) == expected;
                }
                bbCheck(matches == count, "lookup returned unexpected target");
                return matches;
            });
            bbCheck(pool.shutdown(true).m_status == CTaskThreadOk, "lookup fixture did not close");
            return 0;
        }
        if (argc != 7)
        {
            throw std::runtime_error("usage: --benchmark mode producers count levels work report.csv (see CTaskThreadManagerTest.h)");
        }
        const std::string mode = argv[1];
        const size_t producers = static_cast<size_t>(bbUnsigned(argv[2], 64));
        const size_t count = static_cast<size_t>(bbUnsigned(argv[3], 1000000000));
        const int32_t levels = static_cast<int32_t>(bbUnsigned(argv[4], 64));
        const uint32_t work = static_cast<uint32_t>(bbUnsigned(argv[5], UINT32_MAX));
        if (mode == "pool-lifecycle")
        {
            bbCheck(producers == 1 && count > 0 && count <= 10000000 && levels == 1 && work == 0,
                "pool-lifecycle requires 1 producer, 1 level and work=0");
            BenchmarkPoolLifecycle(count, argv[6]);
            return 0;
        }
        if (producers == 0 || producers > 32 || count == 0 || count > 10000000 / producers || levels < 1 || levels > 64 ||
            work > 100000 || ((mode == "lifecycle" || mode == "roundtrip") && (producers != 1 || levels != 1)) ||
            (mode == "lookup" && work != 0) ||
            (mode != "throughput" && mode != "lookup" && mode != "lifecycle" && mode != "roundtrip"))
        {
            throw std::runtime_error("invalid benchmark input");
        }
        CTaskBenchmarkState state(producers, count, levels, work);
        CTaskThreadManager& manager = CTaskThreadManager::instance();
        uint64_t cpuBefore = 0, privateBefore = 0, peak = 0, handlesBefore = 0;
        BenchmarkResources(cpuBefore, privateBefore, peak, handlesBefore);
        uint64_t startTime = 0;
        if (mode == "lifecycle" || mode == "roundtrip")
        {
            uint32_t id = 0;
            std::shared_ptr<CTaskThread> target;
            if (mode == "roundtrip")
            {
                id = manager.init();
                target = manager.getThreadInterface(id);
                if (!target)
                {
                    throw std::runtime_error("roundtrip initialization failed");
                }
            }
            uint64_t observedEnd = 0;
            startTime = CTaskBenchmarkState::now();
            for (size_t sequence = 0; sequence < count; ++sequence)
            {
                const uint64_t began = CTaskBenchmarkState::now();
                if (mode == "lifecycle")
                {
                    id = manager.init();
                    target = manager.getThreadInterface(id);
                    if (!target)
                    {
                        throw std::runtime_error("lifecycle initialization failed");
                    }
                }
                std::shared_ptr<CTask> item(new CTaskBenchmarkItem(state, 0, sequence, 0));
                const CTaskThreadResult result = target->postTaskResult(item, 1);
                if (!result.m_committed || result.m_status != CTaskThreadOk)
                {
                    throw std::runtime_error("single outstanding post rejected");
                }
                ++state.m_accepted;
                state.submitted(CTaskBenchmarkState::now());
                BenchmarkWait([&state, sequence]()
                {
                    return state.m_completed.load() == sequence + 1;
                }, state);
                if (mode == "lifecycle")
                {
                    BenchmarkDrain(manager, id);
                    target.reset();
                }
                observedEnd = CTaskBenchmarkState::now();
                state.m_latency[sequence] = observedEnd - began;
            }
            if (mode == "roundtrip")
            {
                BenchmarkDrain(manager, id);
                target.reset();
            }
            // 往返以调用者观察完成为终点，生命周期还包含注销；join 后写入避免消费者覆盖。
            state.m_finishTime.store(observedEnd, std::memory_order_release);
        }
        else
        {
            uint32_t targetId = 0;
            std::shared_ptr<CTaskThread> target;
            if (mode == "lookup")
            {
                for (size_t i = 0; i < 32; ++i)
                {
                    const uint32_t id = manager.init();
                    if (id == 0)
                    {
                        throw std::runtime_error("lookup target initialization failed");
                    }
                    state.m_targets.push_back(id);
                }
            }
            else
            {
                targetId = manager.init();
                target = manager.getThreadInterface(targetId);
                if (!target)
                {
                    throw std::runtime_error("target initialization failed");
                }
            }
            std::vector<uint32_t> ids;
            for (size_t p = 0; p < producers; ++p)
            {
                const uint32_t id = manager.init();
                std::shared_ptr<CTaskThread> thread = manager.getThreadInterface(id);
                if (!thread)
                {
                    throw std::runtime_error("producer initialization failed");
                }
                ids.push_back(id);
                const std::shared_ptr<CTask> producer(new CTaskBenchmarkProducer(state, target, p, mode == "lookup"));
                const CTaskThreadResult result = thread->postTaskResult(producer, 1);
                if (!result.m_committed || result.m_status != CTaskThreadOk)
                {
                    throw std::runtime_error("benchmark producer start failed");
                }
            }
            BenchmarkWait([&state, producers]()
            {
                return state.m_ready.load() == producers;
            }, state);
            BenchmarkResources(cpuBefore, privateBefore, peak, handlesBefore);
            startTime = CTaskBenchmarkState::now();
            state.m_start = true;
            BenchmarkWait([&state]()
            {
                return state.m_finishTime.load(std::memory_order_acquire) != 0;
            }, state);
            for (size_t p = 0; p < ids.size(); ++p)
            {
                BenchmarkDrain(manager, ids[p]);
            }
            if (targetId != 0)
            {
                BenchmarkDrain(manager, targetId);
            }
            for (size_t i = 0; i < state.m_targets.size(); ++i)
            {
                BenchmarkDrain(manager, state.m_targets[i]);
            }
            target.reset();
        }
        if (state.m_errors != 0 || state.m_completed != producers * count || state.m_accepted != producers * count || manager.count() != 0)
        {
            throw std::runtime_error("benchmark failed independent count/content/FIFO verification");
        }
        if (mode != "lookup")
        {
            for (size_t i = 0; i < state.m_seen.size(); ++i)
            {
                if (state.m_seen[i] != 1)
                {
                    throw std::runtime_error("benchmark missing/duplicate task");
                }
            }
        }
        uint64_t cpuAfter = 0, privateAfter = 0, handlesAfter = 0;
        BenchmarkResources(cpuAfter, privateAfter, peak, handlesAfter);
        std::vector<uint64_t> latency;
        for (size_t i = 0; i < state.m_latency.size(); ++i)
        {
            if (state.m_latency[i] != UINT64_MAX)
            {
                latency.push_back(state.m_latency[i]);
            }
        }
        std::sort(latency.begin(), latency.end());
        const size_t expectedSamples = (mode == "lifecycle" || mode == "roundtrip") ? count : producers * ((count + 63) / 64);
        if (latency.size() != expectedSamples)
        {
            throw std::runtime_error("benchmark latency sample count mismatch");
        }
        if (state.m_finishTime.load() <= startTime || state.m_submitTime.load() <= startTime)
        {
            throw std::runtime_error("benchmark duration must be positive");
        }
        const double completeMs = static_cast<double>(state.m_finishTime.load() - startTime) / 1000000.0;
        const double submitMs = static_cast<double>(state.m_submitTime.load() - startTime) / 1000000.0;
        std::ofstream output(argv[6], std::ios::binary | std::ios::trunc);
        if (!output)
        {
            throw std::runtime_error("cannot create benchmark report");
        }
        const char* variant = "latest";
        output << "variant,mode,producers,count,levels,work,total,accepted,completed,retries,submit_ms,complete_ms,throughput_ops_s,cpu_ms,p50_us,p95_us,p99_us,peak_working_mb,private_before_mb,private_after_mb,handles_before,handles_after,errors\n";
        output << std::fixed << std::setprecision(6) << variant << ',' << mode << ',' << producers << ',' << count << ',' << levels << ',' << work << ','
               << producers * count << ',' << state.m_accepted.load() << ',' << state.m_completed.load() << ',' << state.m_retries.load() << ','
               << submitMs << ',' << completeMs << ',' << producers * count * 1000.0 / completeMs << ',';
#ifdef _WIN32
        output << (cpuAfter - cpuBefore) / 1000000.0;
#else
        output << "NA";
#endif
        output << ','
               << latency[(latency.size() + 1) / 2 - 1] / 1000.0 << ',' << latency[(latency.size() * 95 + 99) / 100 - 1] / 1000.0 << ','
               << latency[(latency.size() * 99 + 99) / 100 - 1] / 1000.0 << ',';
#ifdef _WIN32
        output << peak / 1048576.0 << ',' << privateBefore / 1048576.0 << ','
               << privateAfter / 1048576.0 << ',' << handlesBefore << ',' << handlesAfter;
#else
        output << "NA,NA,NA,NA,NA";
#endif
        output << ',' << state.m_errors.load() << '\n';
        output.close();
        if (!output)
        {
            throw std::runtime_error("benchmark report write failed");
        }
        std::cout << "PASS " << variant << ' ' << mode << " total=" << state.m_completed.load() << " complete_ms=" << completeMs << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL benchmark: " << error.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "FAIL benchmark: unknown exception\n";
        return 1;
    }
}

#ifdef CTTM_BENCHMARK_MAIN
/** 独立构建时转入辅助性能测试入口
@param [in] argc 命令行参数数量
@param [in] argv 当前入口的命令行参数数组
@return 0表示所选检查成功，非零表示失败或当前组合不适用
*/
int main(int argc, char** argv)
{
    return runTaskThreadBenchmark(argc, argv);
}

#endif