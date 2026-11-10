#include "CTaskFairnessTests.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <thread>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#endif

/** 取得单调毫秒时间
@return steady_clock 毫秒值
*/
static uint64_t FairnessNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

/** 取得当前线程实际 CPU 使用量
@return Windows 用户加内核 CPU 时间，100 纳秒；不支持的平台为零
*/
static uint64_t FairnessCpu()
{
#ifdef _WIN32
    FILETIME created, exited, kernel, user;
    if (!GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user))
    {
        throw std::runtime_error("GetThreadTimes failed");
    }
    return (static_cast<uint64_t>(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
        (static_cast<uint64_t>(user.dwHighDateTime) << 32) + user.dwLowDateTime;
#else
    return 0;
#endif
}

CTaskFairnessCounter::CTaskFairnessCounter() :
m_published(0), m_started(false), m_count(0), m_maxGap(0), m_cpu(0), m_capacity(0), m_resources(0), m_retries(0)
{

}

CTaskFairnessState::CTaskFairnessState(size_t producers, size_t levels) :
m_producers(producers), m_levels(levels), m_sent(new CTaskFairnessCounter[producers]),
m_received(new CTaskFairnessCounter[producers]), m_orders(producers * levels, 0),
m_objects(producers, 0), m_start(false), m_stop(false), m_errors(0), m_begin(0)
{

}

void CTaskFairnessState::consume(size_t producer, size_t ordinal)
{
    const size_t level = ordinal % m_levels;
    const size_t group = (m_objects[producer] - 1 - level) / m_levels + 1;
    uint64_t& order = m_orders[producer * m_levels + level];
    if (order % group != ordinal / m_levels)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
    }
    ++order;
    CTaskFairnessCounter& count = m_received[producer];
    if ((++count.m_count & 63) == 0)
    {
        count.m_published.store(count.m_count, std::memory_order_release);
    }
    if (count.m_count == 1)
    {
#ifdef _WIN32
        if (GetThreadPriority(GetCurrentThread()) != THREAD_PRIORITY_NORMAL)
        {
            m_errors.fetch_add(1, std::memory_order_relaxed);
        }
#endif
    }
}

CTaskFairnessTask::CTaskFairnessTask(CTaskFairnessState& state, size_t producer, size_t ordinal) :
m_state(&state), m_producer(producer), m_ordinal(ordinal)
{

}

void CTaskFairnessTask::doTask()
{
    m_state->consume(m_producer, m_ordinal);
}

int runTaskFairnessTests(int argc, char** argv)
{
    try
    {
        if (argc != 6 && argc != 7)
        {
            throw std::runtime_error("usage: fairness producers levels seconds cpu_count report.csv [queue_producers]");
        }
        const size_t producers = static_cast<size_t>(std::atoi(argv[1]));
        const size_t levels = static_cast<size_t>(std::atoi(argv[2]));
        const uint64_t seconds = static_cast<uint64_t>(std::atoi(argv[3]));
        const size_t cpuCount = static_cast<size_t>(std::atoi(argv[4]));
        const size_t queueProducers = argc == 7 ? static_cast<size_t>(std::atoi(argv[6])) : producers + 8;
        if (producers == 0 || producers > 64 || levels == 0 || levels > 10 || seconds < 3 || seconds > 120 || queueProducers == 0 || queueProducers > 1024)
        {
            throw std::runtime_error("invalid fairness configuration");
        }
#ifdef _WIN32
        if (!SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS) ||
            !SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL))
        {
            throw std::runtime_error("cannot set normal test priority");
        }
        if (cpuCount != 0)
        {
            DWORD_PTR allowed = 0, system = 0, mask = 0;
            if (!GetProcessAffinityMask(GetCurrentProcess(), &allowed, &system))
            {
                throw std::runtime_error("cannot read affinity");
            }
            size_t selected = 0;
            for (size_t bit = 0; bit < sizeof(DWORD_PTR) * 8 && selected < cpuCount; ++bit)
            {
                const DWORD_PTR one = DWORD_PTR(1) << bit;
                if ((allowed & one) != 0)
                {
                    mask |= one;
                    ++selected;
                }
            }
            if (selected != cpuCount || !SetProcessAffinityMask(GetCurrentProcess(), mask))
            {
                throw std::runtime_error("cannot apply test affinity");
            }
        }
#else
        if (cpuCount != 0)
        {
            throw std::runtime_error("restricted CPU case currently requires Windows test backend");
        }
#endif
        CTaskThreadManager& manager = CTaskThreadManager::instance();
        CTaskFairnessState state(producers, levels);
        CTaskThreadConfig config;
        config.m_systemPriority = CTaskPriorityNormal;
        config.m_maximumTasks = 65536;
        config.m_taskMap.initialCapacity = 196608;
        config.m_taskMap.maximumCapacity = 196608;
        config.m_taskMap.protectionRecordCount = producers + 8;
        config.m_queue.initialCapacity = 69632;
        config.m_queue.maximumCapacity = 69632;
        config.m_queue.maximumProducers = queueProducers;
        std::vector<std::vector<std::shared_ptr<CTask>>> tasks(producers);
        for (size_t p = 0; p < producers; ++p)
        {
            state.m_objects[p] = 2000 / producers + (p < 2000 % producers ? 1 : 0);
            for (size_t n = 0; n < state.m_objects[p]; ++n)
            {
                tasks[p].push_back(std::make_shared<CTaskFairnessTask>(state, p, n));
            }
        }
        std::vector<std::thread> threads;
        threads.reserve(producers);
        std::vector<uint64_t> last(producers, 0), previous(producers, 0);
        const uint32_t id = manager.init(config);
        const std::shared_ptr<CTaskThread> target = manager.getThreadInterface(id);
        if (!target)
        {
            manager.uninit(id);
            throw std::runtime_error("fairness target initialization failed");
        }
        uint64_t receiveGap = 0;
        try
        {
            for (size_t p = 0; p < producers; ++p)
            {
                // p 对应唯一生产计数；外部预建任务、只读配置和 target 保持到 join 之后。
                threads.push_back(std::thread([&state, &tasks, target, p]()
                {
                    try
                    {
#ifdef _WIN32
                        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL))
                        {
                            throw std::runtime_error("producer priority failed");
                        }
#endif
                        while (!state.m_start.load(std::memory_order_acquire) && !state.m_stop.load())
                        {
                            std::this_thread::yield();
                        }
                        // 后四分之一生产者晚加入，检验既有成功者不会永久占据机会。
                        if (p >= state.m_producers * 3 / 4 && state.m_producers >= 4)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(500));
                        }
                        CTaskFairnessCounter& count = state.m_sent[p];
                        const uint64_t cpu = FairnessCpu();
                        count.m_started.store(true, std::memory_order_release);
                        while (!state.m_stop.load(std::memory_order_acquire))
                        {
                            const size_t index = static_cast<size_t>(count.m_count % tasks[p].size());
                            const CTaskThreadResult result = target->postTask(tasks[p][index], static_cast<int32_t>(index % state.m_levels + 1));
                            if (result.m_committed)
                            {
                                ++count.m_count;
                                if ((count.m_count & 63) == 0)
                                {
                                    count.m_published.store(count.m_count, std::memory_order_release);
                                }
                                if (result.m_status != CTaskThreadOk)
                                {
                                    ++state.m_errors;
                                }
                            }
                            else if (result.m_status == CTaskThreadCapacity || result.m_status == CTaskThreadRetry)
                            {
                                count.m_capacity += result.m_status == CTaskThreadCapacity ? 1 : 0;
                                count.m_retries += result.m_status == CTaskThreadRetry ? 1 : 0;
                                std::this_thread::yield();
                            }
#ifndef CTTM_LEGACY_API
                            else if (result.m_status == CTaskThreadResourceUnavailable)
                            {
                                ++count.m_resources;
                                std::this_thread::yield();
                            }
#endif
                            else
                            {
                                ++state.m_errors;
                                state.m_stop = true;
                            }
                        }
                        count.m_cpu = FairnessCpu() - cpu;
                        count.m_published.store(count.m_count, std::memory_order_release);
                    }
                    catch (...)
                    {
                        ++state.m_errors;
                        state.m_stop = true;
                    }
                }));
            }
            state.m_begin = FairnessNow();
            std::fill(last.begin(), last.end(), state.m_begin);
            uint64_t receiveLast = state.m_begin, receivePrevious = 0;
            state.m_start.store(true, std::memory_order_release);
            while (FairnessNow() - state.m_begin < seconds * 1000 && !state.m_stop.load())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                const uint64_t now = FairnessNow();
                uint64_t sent = 0, received = 0;
                for (size_t p = 0; p < producers; ++p)
                {
                    const uint64_t current = state.m_sent[p].m_published.load(std::memory_order_acquire);
                    sent += current;
                    received += state.m_received[p].m_published.load(std::memory_order_acquire);
                    if (!state.m_sent[p].m_started.load(std::memory_order_acquire))
                    {
                        last[p] = now;
                    }
                    else
                    {
                        state.m_sent[p].m_maxGap = (std::max)(state.m_sent[p].m_maxGap, now - last[p]);
                        if (current != previous[p])
                        {
                            previous[p] = current;
                            last[p] = now;
                        }
                    }
                }
                if (sent > received)
                {
                    receiveGap = (std::max)(receiveGap, now - receiveLast);
                }
                if (received != receivePrevious || sent <= received)
                {
                    receivePrevious = received;
                    receiveLast = now;
                }
            }
        }
        catch (...)
        {
            state.m_stop = true;
            for (size_t p = 0; p < threads.size(); ++p)
            {
                threads[p].join();
            }
            manager.uninit(id);
            throw;
        }
        state.m_stop = true;
        for (size_t p = 0; p < threads.size(); ++p)
        {
            threads[p].join();
        }
        const uint64_t submitEnd = FairnessNow();
        const CTaskThreadResult closed = manager.waitForEnd(id);
        const uint64_t ended = FairnessNow();
        std::ofstream out(argv[5], std::ios::binary | std::ios::trunc);
        if (!out)
        {
            throw std::runtime_error("cannot open fairness report");
        }
        out << "producer,producers,levels,cpus,queue_producers,submitted,consumed,max_submit_gap_ms,consumer_max_gap_ms,producer_cpu_ms,capacity_failures,external_retries,submit_ms,complete_ms,errors,resource_failures\n";
        bool valid = closed.m_status == CTaskThreadOk && state.m_errors == 0 && target->statistics().m_errors == 0 && receiveGap < 2000;
        for (size_t p = 0; p < producers; ++p)
        {
            const CTaskFairnessCounter& sent = state.m_sent[p];
            const CTaskFairnessCounter& received = state.m_received[p];
            valid = valid && sent.m_count == received.m_count && sent.m_count >= 1000 && sent.m_maxGap < 2000;
#ifndef CTTM_LEGACY_API
            valid = valid && sent.m_retries == 0;
#endif
            out << p << ',' << producers << ',' << levels << ',' << cpuCount << ',' << queueProducers << ',' << sent.m_count << ',' << received.m_count << ','
                << sent.m_maxGap << ',' << receiveGap << ',' << sent.m_cpu / 10000.0 << ',' << sent.m_capacity << ',' << sent.m_retries << ','
                << submitEnd - state.m_begin << ',' << ended - state.m_begin << ',' << state.m_errors.load() << ',' << sent.m_resources << '\n';
        }
        std::cout << "fairness " << (valid ? "PASS" : "FAIL") << " producers=" << producers << " levels=" << levels << " cpus=" << cpuCount
            << " consumer_gap_ms=" << receiveGap << " errors=" << state.m_errors.load() << "\n";
        return valid ? 0 : 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "unknown fairness test exception\n";
        return 1;
    }
}

#ifdef CTTM_FAIRNESS_MAIN
int main(int argc, char** argv)
{
    return runTaskFairnessTests(argc, argv);
}
#endif