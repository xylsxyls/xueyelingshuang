#include "CTaskThreadBenchmark.h"
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
m_next(producers, 0),
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
m_state(state),
m_producer(producer),
m_sequence(sequence),
m_timestamp(timestamp)
{

}

void CTaskBenchmarkItem::doTask()
{
    const size_t index = m_producer * m_state.m_count + m_sequence;
    if (m_state.m_seen[index]++ != 0 ||
        CTaskBenchmarkState::digest(static_cast<uint64_t>(index) + 1, m_state.m_work) != m_state.m_expected[index])
    {
        ++m_state.m_errors;
    }
    if (m_state.m_levels == 1 && m_state.m_next[m_producer]++ != m_sequence)
    {
        ++m_state.m_errors;
    }
    if (m_timestamp != 0)
    {
        m_state.m_latency[index] = CTaskBenchmarkState::now() - m_timestamp;
    }
    if (m_state.m_completed.fetch_add(1, std::memory_order_acq_rel) + 1 == m_state.m_expected.size())
    {
        m_state.m_finishTime.store(CTaskBenchmarkState::now(), std::memory_order_release);
    }
}

CTaskBenchmarkProducer::CTaskBenchmarkProducer(CTaskBenchmarkState& state, const std::shared_ptr<CTaskThread>& target,
    size_t producer, bool lookup) :
m_state(state),
m_target(target),
m_producer(producer),
m_lookup(lookup)
{

}

void CTaskBenchmarkProducer::stopTask()
{
    m_state.m_stop.store(true, std::memory_order_release);
}

void CTaskBenchmarkProducer::doTask()
{
    try
    {
        m_state.m_ready.fetch_add(1, std::memory_order_release);
        while (!m_state.m_start.load(std::memory_order_acquire) && !m_state.m_stop.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
        for (size_t sequence = 0; sequence < m_state.m_count && !m_state.m_stop.load(std::memory_order_acquire); ++sequence)
        {
            const uint64_t timestamp = (sequence & 63) == 0 ? CTaskBenchmarkState::now() : 0;
            if (m_lookup)
            {
                const uint32_t id = m_state.m_targets[(sequence + m_producer) % m_state.m_targets.size()];
                if (!CTaskThreadManager::instance().getThreadInterface(id))
                {
                    ++m_state.m_errors;
                }
                if (timestamp != 0)
                {
                    m_state.m_latency[m_producer * m_state.m_count + sequence] = CTaskBenchmarkState::now() - timestamp;
                }
            }
            else
            {
                std::shared_ptr<CTask> item(new CTaskBenchmarkItem(m_state, m_producer, sequence, timestamp));
                const int32_t level = static_cast<int32_t>(sequence % m_state.m_levels) + 1;
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
                for (;;)
                {
                    const CTaskThreadResult result = m_target->postTask(item, level);
                    if (result.m_committed)
                    {
                        break;
                    }
                    if (result.m_status != CTaskThreadRetry || m_state.m_stop.load())
                    {
                        throw std::runtime_error("benchmark post rejected");
                    }
                    ++m_state.m_retries;
                    std::this_thread::yield();
                }
#else
                m_target->postTask(item, level);
#endif
            }
        }
        m_state.m_accepted.fetch_add(m_state.m_count, std::memory_order_relaxed);
        const uint64_t timestamp = CTaskBenchmarkState::now();
        m_state.submitted(timestamp);
        if (m_lookup && m_state.m_completed.fetch_add(m_state.m_count, std::memory_order_acq_rel) + m_state.m_count == m_state.m_expected.size())
        {
            m_state.m_finishTime.store(CTaskBenchmarkState::now(), std::memory_order_release);
        }
    }
    catch (...)
    {
        ++m_state.m_errors;
        m_state.m_stop = true;
    }
}

/** 有截止时间地等待基准条件。
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

/** 查询进程 CPU 和资源观测。
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

int runTaskThreadBenchmark(int argc, char** argv)
{
    try
    {
        if (argc != 7)
        {
            throw std::runtime_error("usage: throughput|lookup|lifecycle|roundtrip producers count levels work report.csv");
        }
        const std::string mode = argv[1];
        const size_t producers = static_cast<size_t>(std::strtoul(argv[2], nullptr, 10));
        const size_t count = static_cast<size_t>(std::strtoul(argv[3], nullptr, 10));
        const int32_t levels = std::atoi(argv[4]);
        const uint32_t work = static_cast<uint32_t>(std::strtoul(argv[5], nullptr, 10));
        if (producers == 0 || producers > 32 || count == 0 || count > 10000000 / producers || levels < 1 || levels > 64 ||
            work > 100000 || ((mode == "lifecycle" || mode == "roundtrip") && producers != 1) ||
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
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
                if (!target->postTask(item).m_committed)
                {
                    throw std::runtime_error("single outstanding post rejected");
                }
#else
                target->postTask(item);
#endif
                ++state.m_accepted;
                state.submitted(CTaskBenchmarkState::now());
                BenchmarkWait([&state, sequence]()
                {
                    return state.m_completed.load() == sequence + 1;
                }, state);
                if (mode == "lifecycle")
                {
                    manager.waitForEnd(id);
                    target.reset();
                }
                state.m_latency[sequence] = CTaskBenchmarkState::now() - began;
            }
            if (mode == "roundtrip")
            {
                manager.waitForEnd(id);
                target.reset();
            }
            else
            {
                state.m_finishTime = CTaskBenchmarkState::now();
            }
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
                thread->postTask(std::make_shared<CTaskBenchmarkProducer>(state, target, p, mode == "lookup"));
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
                manager.waitForEnd(ids[p]);
            }
            if (targetId != 0)
            {
                manager.waitForEnd(targetId);
            }
            for (size_t i = 0; i < state.m_targets.size(); ++i)
            {
                manager.waitForEnd(state.m_targets[i]);
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
        const double completeMs = static_cast<double>(state.m_finishTime.load() - startTime) / 1000000.0;
        const double submitMs = static_cast<double>(state.m_submitTime.load() - startTime) / 1000000.0;
        std::ofstream output(argv[6], std::ios::binary | std::ios::trunc);
        if (!output)
        {
            throw std::runtime_error("cannot create benchmark report");
        }
#if defined(CTTM_BENCH_BEFORE)
        const char* variant = "before";
#elif defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
        const char* variant = "latest";
#else
        const char* variant = "original";
#endif
        output << "variant,mode,producers,count,levels,work,total,accepted,completed,retries,submit_ms,complete_ms,throughput_ops_s,cpu_ms,p50_us,p95_us,p99_us,peak_working_mb,private_before_mb,private_after_mb,handles_before,handles_after,errors\n";
        output << std::fixed << std::setprecision(6) << variant << ',' << mode << ',' << producers << ',' << count << ',' << levels << ',' << work << ','
               << producers * count << ',' << state.m_accepted.load() << ',' << state.m_completed.load() << ',' << state.m_retries.load() << ','
               << submitMs << ',' << completeMs << ',' << producers * count * 1000.0 / completeMs << ',' << (cpuAfter - cpuBefore) / 1000000.0 << ','
               << latency[latency.size() / 2] / 1000.0 << ',' << latency[(latency.size() - 1) * 95 / 100] / 1000.0 << ','
               << latency[(latency.size() - 1) * 99 / 100] / 1000.0 << ',' << peak / 1048576.0 << ',' << privateBefore / 1048576.0 << ','
               << privateAfter / 1048576.0 << ',' << handlesBefore << ',' << handlesAfter << ',' << state.m_errors.load() << '\n';
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
int main(int argc, char** argv)
{
    return runTaskThreadBenchmark(argc, argv);
}

#endif