#include "CTaskPreparedBenchmark.h"
#include "CTaskPreparedProfiler.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#endif

CTaskPreparedSample::CTaskPreparedSample() :
m_begin(0),
m_returned(0),
m_entered(0),
m_retries(0),
m_level(0)
{

}

CTaskPreparedProducerData::CTaskPreparedProducerData() :
m_published(0),
m_done(false),
m_end(0),
m_consumedAtEnd(0),
m_retries(0)
{

}

CTaskPreparedCompletion::CTaskPreparedCompletion() :
m_count(0)
{

}

CTaskPreparedState::CTaskPreparedState(size_t producers, size_t total, int32_t levels, bool latency, bool separateAllocation,
    bool reuseTasks, bool countOnly, size_t window) :
m_producers(producers),
m_total(total),
m_levels(levels),
m_latency(latency),
m_window(window),
m_windowCompletions(window != 0 ? new CTaskPreparedCompletion[producers] : nullptr),
m_taskOffsets(producers + 1, 0),
m_samples(latency ? producers * ((total / producers + 1008) / 1009) : 0),
m_expected(producers * static_cast<size_t>(levels), 0),
m_repeatNext(producers * static_cast<size_t>(levels), 0),
m_progress(new CTaskPreparedProducerData[producers]),
m_ready(0),
m_start(false),
m_stop(false),
m_completed(0),
m_observed(0),
m_finish(0),
m_errors(0)
{
#ifdef CTTM_BENCH_CPU_TRACE
    std::fill(m_cpuSamples, m_cpuSamples + 64, UINT64_C(0));
    std::fill(m_cpuTrace, m_cpuTrace + sizeof(m_cpuTrace) / sizeof(m_cpuTrace[0]), uint8_t(255));
#endif
#ifdef CTTM_BENCH_SAMPLER
    m_consumerId.store(0, std::memory_order_relaxed);
#endif
    if (reuseTasks && (2000 % levels != 0 || static_cast<size_t>(2000 / levels) < producers))
    {
        throw std::runtime_error("reused task layout cannot preserve fixed per-level identities");
    }
    if (window > 1 || (window != 0 && (!latency || !reuseTasks || countOnly)))
    {
        throw std::runtime_error("windowed latency requires sampled reusable tasks and a one-item window");
    }
    m_tasks.reserve(reuseTasks ? 2000 : total);
    const size_t count = total / producers;
    const size_t samples = (count + 1008) / 1009;
    for (size_t p = 0; p < producers; ++p)
    {
        if (latency)
        {
            for (size_t i = 0; i < samples; ++i)
            {
                m_samples[p * samples + i].m_level = static_cast<int32_t>((i * 1009) % levels) + 1;
            }
        }
        // 以完整优先级周期分配对象；全部生产者合计恰好 2000 个，不在计时内构造。
        const size_t groups = reuseTasks ? 2000 / static_cast<size_t>(levels) : 0;
        const size_t objects = reuseTasks ? (groups / producers + (p < groups % producers ? 1 : 0)) * levels : count;
        m_taskOffsets[p] = m_tasks.size();
        for (size_t sequence = 0; sequence < objects; ++sequence)
        {
            if (countOnly)
            {
                m_tasks.push_back(std::make_shared<CTaskPreparedCountedItem>(*this));
                continue;
            }
            if (latency && reuseTasks)
            {
                m_tasks.push_back(std::make_shared<CTaskPreparedSampledItem>(*this, p, sequence, objects / levels));
                continue;
            }
            CTaskPreparedSample* sample = latency && sequence % 1009 == 0 ? &m_samples[p * samples + sequence / 1009] : nullptr;
            if (sample != nullptr)
            {
                sample->m_level = static_cast<int32_t>(sequence % levels) + 1;
            }
            uint64_t* repeatNext = reuseTasks ? &m_repeatNext[p * levels + sequence % levels] : nullptr;
            const uint64_t cycle = reuseTasks ? objects / levels : 0;
            if (separateAllocation)
            {
                m_tasks.push_back(std::shared_ptr<CTask>(new CTaskPreparedItem(*this, sample,
                    m_expected[p * levels + sequence % levels], sequence / levels, repeatNext, cycle)));
            }
            else
            {
                m_tasks.push_back(std::make_shared<CTaskPreparedItem>(*this, sample,
                    m_expected[p * levels + sequence % levels], sequence / levels, repeatNext, cycle));
            }
        }
    }
    m_taskOffsets[producers] = m_tasks.size();
}

uint64_t CTaskPreparedState::now()
{
#ifdef _WIN32
    LARGE_INTEGER value;
    if (!QueryPerformanceCounter(&value))
    {
        throw std::runtime_error("QueryPerformanceCounter failed");
    }
    return static_cast<uint64_t>(value.QuadPart);
#else
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}

uint64_t CTaskPreparedState::frequency()
{
#ifdef _WIN32
    LARGE_INTEGER value;
    if (!QueryPerformanceFrequency(&value) || value.QuadPart <= 0)
    {
        throw std::runtime_error("QueryPerformanceFrequency failed");
    }
    return static_cast<uint64_t>(value.QuadPart);
#else
    return 1000000000;
#endif
}

CTaskPreparedItem::CTaskPreparedItem(CTaskPreparedState& state, CTaskPreparedSample* sample, uint64_t& expected, uint64_t sequence,
    uint64_t* repeatNext, uint64_t cycle) :
m_sequence(sequence),
m_expected(&expected),
m_repeatNext(repeatNext),
m_cycle(cycle),
m_state(&state),
m_sample(sample)
{

}

void CTaskPreparedItem::doTask()
{
#if defined(CTTM_BENCH_FIXED_CPU) && defined(_WIN32)
    if (m_state->m_completed == 0 && SetThreadAffinityMask(GetCurrentThread(), DWORD_PTR(1) << 2) == 0)
    {
        throw std::runtime_error("diagnostic consumer affinity failed");
    }
#endif
    if (m_sample != nullptr)
    {
        m_sample->m_entered = CTaskPreparedState::now();
    }
    // 只有消费者更新小型序号表；每项必须恰好是本生产者/优先级的下一项。
    // 不向每个 Task 写执行结果，避免测试自身与生产者的 shared_ptr 引用计数争用。
    uint64_t& expected = m_repeatNext != nullptr ? *m_repeatNext : *m_expected;
    if (expected != m_sequence)
    {
        m_state->m_errors.fetch_add(1, std::memory_order_relaxed);
    }
    ++expected;
    if (m_repeatNext != nullptr)
    {
        if (expected == m_cycle)
        {
            expected = 0;
        }
        ++*m_expected;
    }
    const uint64_t order = ++m_state->m_completed;
#ifdef CTTM_BENCH_SAMPLER
    if (order == 1)
    {
        m_state->m_consumerId.store(GetCurrentThreadId(), std::memory_order_release);
    }
#endif
    if ((order & 1023) == 0)
    {
#if defined(CTTM_BENCH_CPU_TRACE) && defined(_WIN32)
        const DWORD cpu = GetCurrentProcessorNumber();
        if (cpu < 64)
        {
            ++m_state->m_cpuSamples[cpu];
            const uint64_t block = order >> 10;
            if (block <= sizeof(m_state->m_cpuTrace) / sizeof(m_state->m_cpuTrace[0]))
            {
                m_state->m_cpuTrace[block - 1] = static_cast<uint8_t>(cpu);
            }
        }
#endif
        m_state->m_observed.store(order, std::memory_order_release);
    }
    if (order == m_state->m_total)
    {
        m_state->m_observed.store(order, std::memory_order_release);
        m_state->m_finish.store(CTaskPreparedState::now(), std::memory_order_release);
    }
}

CTaskPreparedCountedItem::CTaskPreparedCountedItem(CTaskPreparedState& state) :
m_state(&state)
{

}

void CTaskPreparedCountedItem::doTask()
{
    const uint64_t order = ++m_state->m_completed;
    if ((order & 1023) == 0)
    {
        m_state->m_observed.store(order, std::memory_order_release);
    }
    if (order == m_state->m_total)
    {
        m_state->m_observed.store(order, std::memory_order_release);
        m_state->m_finish.store(CTaskPreparedState::now(), std::memory_order_release);
    }
}

CTaskPreparedProducer::CTaskPreparedProducer(CTaskPreparedState& state, const std::shared_ptr<CTaskPreparedThread>& target, size_t producer) :
m_state(&state),
m_target(target),
m_producer(producer)
{

}

void CTaskPreparedProducer::doTask()
{
    CTaskPreparedProducerData& progress = m_state->m_progress[m_producer];
    try
    {
#if defined(CTTM_BENCH_FIXED_CPU) && defined(_WIN32)
        const DWORD cpus[] = {0, 4, 6, 8, 10, 12, 14, 1};
        if (m_producer >= 8 || SetThreadAffinityMask(GetCurrentThread(), DWORD_PTR(1) << cpus[m_producer]) == 0)
        {
            throw std::runtime_error("diagnostic producer affinity failed");
        }
#endif
        m_state->m_ready.fetch_add(1, std::memory_order_release);
        while (!m_state->m_start.load(std::memory_order_acquire) && !m_state->m_stop.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
        if (m_state->m_window != 0)
        {
            produceWindow();
            progress.m_done.store(true, std::memory_order_release);
            return;
        }
        const size_t count = m_state->m_total / m_state->m_producers;
        const size_t samples = (count + 1008) / 1009;
        uint64_t accepted = 0;
        size_t taskIndex = m_state->m_taskOffsets[m_producer];
        const size_t taskEnd = m_state->m_taskOffsets[m_producer + 1];
        for (size_t sequence = 0; sequence < count && !m_state->m_stop.load(std::memory_order_acquire); ++sequence)
        {
            CTaskPreparedSample* sample = m_state->m_latency && sequence % 1009 == 0 ? &m_state->m_samples[m_producer * samples + sequence / 1009] : nullptr;
            const int32_t level = static_cast<int32_t>(sequence % m_state->m_levels) + 1;
            const uint64_t retriesBefore = progress.m_retries;
            if (sample != nullptr)
            {
                sample->m_begin = CTaskPreparedState::now();
            }
            const std::shared_ptr<CTask>& task = m_state->m_tasks[taskIndex];
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
            for (;;)
            {
                const CTaskThreadResult result = m_target->postTask(task, level);
                if (result.m_committed)
                {
                    if (result.m_status != CTaskThreadOk)
                    {
                        ++m_state->m_errors;
                    }
                    break;
                }
                if (result.m_status != CTaskThreadRetry || m_state->m_stop.load(std::memory_order_acquire))
                {
                    throw std::runtime_error("postTask rejected prepared task");
                }
                ++progress.m_retries;
                std::this_thread::yield();
            }
#else
            m_target->postTask(task, level);
#endif
            if (sample != nullptr)
            {
                sample->m_returned = CTaskPreparedState::now();
                sample->m_retries = progress.m_retries - retriesBefore;
            }
            ++accepted;
            if (++taskIndex == taskEnd)
            {
                taskIndex = m_state->m_taskOffsets[m_producer];
            }
            if ((accepted & 1023) == 0)
            {
                progress.m_published.store(accepted, std::memory_order_release);
            }
        }
        // 先观察消费进度，再结束本生产者的计时；反过来会把两步间的排空消费算入同期下界。
        progress.m_consumedAtEnd = m_state->m_observed.load(std::memory_order_acquire);
        progress.m_end = CTaskPreparedState::now();
        progress.m_published.store(accepted, std::memory_order_release);
    }
    catch (...)
    {
        ++m_state->m_errors;
        m_state->m_stop.store(true, std::memory_order_release);
    }
    progress.m_done.store(true, std::memory_order_release);
}

void CTaskPreparedProducer::stopTask()
{
    m_state->m_stop.store(true, std::memory_order_release);
}

CTaskPreparedSession::CTaskPreparedSession(CTaskPreparedState& state) :
m_targetId(0),
m_state(&state)
{
    m_producerIds.reserve(state.m_producers);
}

CTaskPreparedSession::~CTaskPreparedSession()
{
    m_state->m_stop.store(true, std::memory_order_release);
    m_state->m_start.store(true, std::memory_order_release);
    try
    {
        for (size_t i = 0; i < m_producerIds.size(); ++i)
        {
            if (m_producerIds[i] != 0)
            {
                CTaskPreparedManager::instance().uninit(m_producerIds[i]);
            }
        }
        if (m_targetId != 0)
        {
            CTaskPreparedManager::instance().uninit(m_targetId);
        }
    }
    catch (...)
    {
        std::cerr << "FAIL prepared benchmark cleanup\n";
    }
}

/** 静止创建消费者，V2 预留足以容纳整批任务的容量，V1 保持真实原实现
@param [in] total 总任务数
@param [in] levels 优先级数
@param [out] hashCapacity V2 双槽池总容量，V1 为 0
@param [out] queueCapacity V2 每优先级节点容量，V1 为 0
@param [out] buckets V2 哈希桶数，V1 为 0
@return 消费者 ID，0 表示失败
*/
static uint32_t PreparedCreateTarget(size_t total, int32_t levels, size_t& hashCapacity, size_t& queueCapacity, size_t& buckets)
{
    hashCapacity = 0;
    queueCapacity = 0;
    buckets = 0;
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
    CTaskThreadConfig config;
    hashCapacity = 2 * (total + 4096);
    queueCapacity = (total + levels - 1) / levels + 4096;
    buckets = 1024;
    while (buckets < total + 4096)
    {
        buckets *= 2;
    }
#ifdef CTTM_BENCH_V3
    // V3 一条记录占一槽，V2 一条记录占键/值两槽；按相同可容纳任务数预留。
    hashCapacity = total + 4096;
    buckets = 0;
#else
    config.m_taskBuckets = buckets;
#endif
    config.m_taskMap.initialCapacity = hashCapacity;
    config.m_taskMap.maximumCapacity = static_cast<int64_t>(hashCapacity);
    config.m_taskMap.growFreeThreshold = 0;
    config.m_queue.initialCapacity = queueCapacity;
    config.m_queue.maximumCapacity = static_cast<int64_t>(queueCapacity);
    config.m_queue.growThreshold = 0;
    config.m_priorityMap.nodes.initialCapacity = 64;
    config.m_priorityMap.values.initialCapacity = 128;
    return CTaskPreparedManager::instance().init(config);
#else
    return CTaskPreparedManager::instance().init();
#endif
}

/** 在计时外预建各优先级队列并验证预热任务已经执行
@param [in,out] warmState 专用预热状态，生命周期覆盖消费者
@param [in] target 消费者
@param [in] frequency 时钟频率
*/
static void PreparedWarmup(CTaskPreparedState& warmState, const std::shared_ptr<CTaskPreparedThread>& target, uint64_t frequency)
{
    for (size_t i = 0; i < warmState.m_total; ++i)
    {
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
        if (!target->postTask(warmState.m_tasks[i], static_cast<int32_t>(i % warmState.m_levels) + 1).m_committed)
        {
            throw std::runtime_error("warmup rejected");
        }
#else
        target->postTask(warmState.m_tasks[i], static_cast<int32_t>(i % warmState.m_levels) + 1);
#endif
    }
    const uint64_t deadline = CTaskPreparedState::now() + frequency * 120;
    while (warmState.m_finish.load(std::memory_order_acquire) == 0)
    {
        if (CTaskPreparedState::now() > deadline)
        {
            throw std::runtime_error("warmup timed out");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

/** 写采样分布；按优先级分组，提交耗时与包含排队的响应时间分列
@param [in] path 输出前缀
@param [in] state 已经收敛的测试状态
@param [in] frequency 时钟频率
*/
static void PreparedWriteLatency(const std::string& path, const CTaskPreparedState& state, uint64_t frequency)
{
    std::ofstream raw((path + ".samples.csv").c_str(), std::ios::binary | std::ios::trunc);
    std::ofstream summary((path + ".latency.csv").c_str(), std::ios::binary | std::ios::trunc);
    if (!raw || !summary)
    {
        throw std::runtime_error("cannot create latency reports");
    }
    raw << "level,post_us,response_us,retries\n"
        << std::fixed << std::setprecision(6);
    summary << "level,samples,post_p50_us,post_p99_us,response_p50_us,response_p99_us\n"
            << std::fixed << std::setprecision(6);
    for (size_t i = 0; i < state.m_samples.size(); ++i)
    {
        const CTaskPreparedSample& sample = state.m_samples[i];
        if (sample.m_begin == 0 || sample.m_entered < sample.m_begin || sample.m_returned < sample.m_begin)
        {
            throw std::runtime_error("invalid latency sample");
        }
        raw << sample.m_level << ',' << (sample.m_returned - sample.m_begin) * 1000000.0 / frequency << ','
            << (sample.m_entered - sample.m_begin) * 1000000.0 / frequency << ',' << sample.m_retries << '\n';
    }
    for (int32_t level = 0; level <= state.m_levels; ++level)
    {
        std::vector<uint64_t> post;
        std::vector<uint64_t> response;
        for (size_t i = 0; i < state.m_samples.size(); ++i)
        {
            const CTaskPreparedSample& sample = state.m_samples[i];
            if (level == 0 || level == sample.m_level)
            {
                post.push_back(sample.m_returned - sample.m_begin);
                response.push_back(sample.m_entered - sample.m_begin);
            }
        }
        if (!post.empty())
        {
            std::sort(post.begin(), post.end());
            std::sort(response.begin(), response.end());
            // nearest-rank 定义：ceil(N * p) - 1。
            const size_t p50 = (post.size() + 1) / 2 - 1;
            const size_t p99 = (post.size() * 99 + 99) / 100 - 1;
            summary << level << ',' << post.size() << ',' << post[p50] * 1000000.0 / frequency << ','
                    << post[p99] * 1000000.0 / frequency << ',' << response[p50] * 1000000.0 / frequency << ','
                    << response[p99] * 1000000.0 / frequency << '\n';
        }
    }
    raw.close();
    summary.close();
    if (!raw || !summary)
    {
        throw std::runtime_error("latency report write failed");
    }
}

int runPreparedTaskBenchmark(int argc, char** argv)
{
    try
    {
        if (argc != 6)
        {
            throw std::runtime_error("usage: throughput|throughput-reused|throughput-counted|throughput-separated|throughput-reused-separated|latency|latency-reused|latency-window producers total levels report.csv");
        }
        const std::string mode = argv[1];
        const size_t producers = static_cast<size_t>(std::strtoul(argv[2], nullptr, 10));
        const size_t total = static_cast<size_t>(std::strtoul(argv[3], nullptr, 10));
        const int32_t levels = std::atoi(argv[4]);
        if ((mode != "throughput" && mode != "throughput-reused" && mode != "throughput-separated" &&
                mode != "throughput-reused-separated" && mode != "throughput-counted" && mode != "latency" &&
                mode != "latency-reused" && mode != "latency-window") ||
            producers == 0 || producers > 32 ||
            total == 0 || total > 40000000 || total % producers != 0 || levels < 1 || levels > 10)
        {
            throw std::runtime_error("invalid prepared benchmark arguments");
        }
        const uint64_t frequency = CTaskPreparedState::frequency();
#if defined(CTTM_BENCH_FIXED_CPU) && defined(_WIN32)
        if (producers > 8)
        {
            throw std::runtime_error("fixed CPU diagnostic supports at most 8 producers; use the default benchmark for 16/32");
        }
#endif
        const uint64_t preparationStart = CTaskPreparedState::now();
        CTaskPreparedManager& manager = CTaskPreparedManager::instance();
        const bool separate = mode == "throughput-separated" || mode == "throughput-reused-separated";
        const bool countOnly = mode == "throughput-counted";
        const bool latency = mode == "latency" || mode == "latency-reused" || mode == "latency-window";
        const bool reused = countOnly || mode == "throughput-reused" || mode == "throughput-reused-separated" || mode == "latency-reused" || mode == "latency-window";
#ifdef _WIN32
        DWORD_PTR processMask = 0, systemMask = 0;
        if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask))
        {
            throw std::runtime_error("cannot inspect benchmark CPU affinity");
        }
        // 只记录本进程实际可用范围，不修改亲和性；输出发生在工作负载计时之前。
        std::cout << "environment process_affinity=0x" << std::hex << processMask
            << " system_affinity=0x" << systemMask << std::dec << std::endl;
#endif
        CTaskPreparedState state(producers, total, levels, latency, separate, reused, countOnly, mode == "latency-window" ? 1 : 0);
        CTaskPreparedState warmState(1, static_cast<size_t>(levels) * 64, levels, false, separate);
        CTaskPreparedSession session(state);
        // 时间线内存同样在计时前申请并触碰；顺序表由 state 在准备期建立。
        std::vector<uint64_t> timeline;
        timeline.reserve(180000);
        size_t hashCapacity = 0, queueCapacity = 0, buckets = 0;
        session.m_targetId = PreparedCreateTarget(total, levels, hashCapacity, queueCapacity, buckets);
        const std::shared_ptr<CTaskPreparedThread> target = manager.getThreadInterface(session.m_targetId);
        if (!target)
        {
            throw std::runtime_error("consumer initialization failed");
        }
        PreparedWarmup(warmState, target, frequency);
#ifdef CTTM_BENCH_SAMPLER
        CTaskPreparedProfiler profiler(state, warmState.m_consumerId.load(std::memory_order_acquire));
#endif
        for (size_t p = 0; p < producers; ++p)
        {
            const uint32_t id = manager.init();
            if (id == 0)
            {
                throw std::runtime_error("producer initialization failed");
            }
            session.m_producerIds.push_back(id);
            const std::shared_ptr<CTaskPreparedThread> thread = manager.getThreadInterface(id);
            const std::shared_ptr<CTask> producer = std::make_shared<CTaskPreparedProducer>(state, target, p);
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
            if (!thread || !thread->postTask(producer).m_committed)
            {
                throw std::runtime_error("producer start rejected");
            }
#else
            if (!thread)
            {
                throw std::runtime_error("producer lookup failed");
            }
            thread->postTask(producer);
#endif
        }
        const uint64_t readyDeadline = CTaskPreparedState::now() + frequency * 30;
        while (state.m_ready.load(std::memory_order_acquire) != producers)
        {
            if (state.m_stop.load(std::memory_order_acquire))
            {
                throw std::runtime_error("producer failed before the start barrier");
            }
            if (CTaskPreparedState::now() > readyDeadline)
            {
                throw std::runtime_error("producer start timed out");
            }
            std::this_thread::yield();
        }
        const uint64_t start = CTaskPreparedState::now();
        state.m_start.store(true, std::memory_order_release);
        for (;;)
        {
            uint64_t accepted = 0;
            bool allDone = true;
            for (size_t p = 0; p < producers; ++p)
            {
                accepted += state.m_progress[p].m_published.load(std::memory_order_acquire);
                allDone = allDone && state.m_progress[p].m_done.load(std::memory_order_acquire);
            }
            timeline.push_back(CTaskPreparedState::now() - start);
            timeline.push_back(accepted);
            timeline.push_back(state.m_observed.load(std::memory_order_acquire));
            if (allDone && state.m_finish.load(std::memory_order_acquire) != 0)
            {
                break;
            }
            if (state.m_stop.load(std::memory_order_acquire) || CTaskPreparedState::now() - start > frequency * 300)
            {
                throw std::runtime_error("prepared workload failed or timed out");
            }
#ifdef CTTM_BENCH_SAMPLER
            profiler.sample();
            const uint64_t nextSample = CTaskPreparedState::now() + frequency / 2000;
            while (CTaskPreparedState::now() < nextSample)
            {
                std::this_thread::yield();
            }
#else
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
#endif
        }
        for (size_t p = 0; p < producers; ++p)
        {
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
            if (manager.waitForEnd(session.m_producerIds[p]).m_status != CTaskThreadOk)
            {
                throw std::runtime_error("producer drain failed");
            }
#else
            manager.waitForEnd(session.m_producerIds[p]);
#endif
            session.m_producerIds[p] = 0;
        }
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
        if (manager.waitForEnd(session.m_targetId).m_status != CTaskThreadOk)
        {
            throw std::runtime_error("consumer drain failed");
        }
#else
        manager.waitForEnd(session.m_targetId);
#endif
        session.m_targetId = 0;
        uint64_t submit = 0, accepted = 0, consumedAtSubmit = 0, retries = 0;
        for (size_t p = 0; p < producers; ++p)
        {
            const CTaskPreparedProducerData& progress = state.m_progress[p];
            accepted += progress.m_published.load();
            retries += progress.m_retries;
            if (progress.m_end > submit)
            {
                submit = progress.m_end;
                consumedAtSubmit = progress.m_consumedAtEnd;
            }
        }
        if (accepted != total || state.m_completed != total || state.m_errors.load() != 0)
        {
            throw std::runtime_error("prepared count/error verification failed");
        }
        const size_t count = total / producers;
        for (size_t p = 0; p < producers; ++p)
        {
            for (int32_t level = 0; level < levels; ++level)
            {
                const uint64_t expected = count / levels + (static_cast<size_t>(level) < count % levels ? 1 : 0);
                if (!countOnly && state.m_expected[p * levels + level] != expected)
                {
                    throw std::runtime_error("prepared missing/duplicate/per-producer FIFO violation");
                }
            }
        }
        const uint64_t finish = state.m_finish.load();
#ifdef CTTM_BENCH_SAMPLER
        profiler.write(std::string(argv[5]) + ".profile.csv");
#endif
#ifdef CTTM_BENCH_CPU_TRACE
        uint64_t overlapCpus[64] = {};
        const size_t sampledBlocks = static_cast<size_t>(consumedAtSubmit >> 10);
        if (sampledBlocks > sizeof(state.m_cpuTrace) / sizeof(state.m_cpuTrace[0]))
        {
            throw std::runtime_error("CPU diagnostic trace is incomplete");
        }
        for (size_t block = 0; block < sampledBlocks; ++block)
        {
            const uint8_t cpu = state.m_cpuTrace[block];
            if (cpu < 64)
            {
                ++overlapCpus[cpu];
            }
        }
        for (size_t cpu = 0; cpu < 64; ++cpu)
        {
            if (state.m_cpuSamples[cpu] != 0)
            {
                std::cout << "consumer_cpu=" << cpu << " samples=" << state.m_cpuSamples[cpu]
                          << " overlap_samples=" << overlapCpus[cpu]
                          << " drain_samples=" << state.m_cpuSamples[cpu] - overlapCpus[cpu] << '\n';
            }
        }
#endif
        const double submitMs = (submit - start) * 1000.0 / frequency;
        const double completeMs = (finish - start) * 1000.0 / frequency;
        const double drainMs = finish > submit ? (finish - submit) * 1000.0 / frequency : 0;
        double peakMiB = 0;
#ifdef _WIN32
        PROCESS_MEMORY_COUNTERS memory = {};
        memory.cb = sizeof(memory);
        if (!GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)))
        {
            throw std::runtime_error("GetProcessMemoryInfo failed");
        }
        peakMiB = memory.PeakWorkingSetSize / 1048576.0;
#endif
        std::ofstream output(argv[5], std::ios::binary | std::ios::trunc);
        std::ofstream trace((std::string(argv[5]) + ".timeline.csv").c_str(), std::ios::binary | std::ios::trunc);
        if (!output || !trace)
        {
            throw std::runtime_error("cannot create prepared reports");
        }
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
#ifdef CTTM_BENCH_V3
#ifdef CTTM_BENCH_V5
        const char* version = "V5";
#elif defined(CTTM_BENCH_V4)
        const char* version = "V4";
#else
        const char* version = "V3";
#endif
#else
        const char* version = "V2";
#endif
#else
        const char* version = "V1";
#endif
        output << "version,mode,producers,levels,total,accepted,completed,submit_ms,complete_ms,submit_ops_s,complete_ops_s,consumed_at_submit_lower_bound,overlap_consume_ops_s_lower_bound,drain_ms,retries,peak_mib,preparation_ms,hash_slots,queue_nodes_per_level,hash_buckets,errors,task_objects\n";
        output << std::fixed << std::setprecision(6) << version << ',' << mode << ',' << producers << ',' << levels << ',' << total << ','
               << accepted << ',' << state.m_completed << ',' << submitMs << ',' << completeMs << ',' << total * 1000.0 / submitMs << ','
               << total * 1000.0 / completeMs << ',' << consumedAtSubmit << ',' << consumedAtSubmit * 1000.0 / submitMs << ',' << drainMs << ','
               << retries << ',' << peakMiB << ',' << (start - preparationStart) * 1000.0 / frequency << ',' << hashCapacity << ','
               << queueCapacity << ',' << buckets << ',' << state.m_errors.load() << ',' << state.m_tasks.size() << '\n';
        trace << "elapsed_ms,submitted_lower_bound,completed_lower_bound\n"
              << std::fixed << std::setprecision(6);
        for (size_t i = 0; i < timeline.size(); i += 3)
        {
            trace << timeline[i] * 1000.0 / frequency << ',' << timeline[i + 1] << ',' << timeline[i + 2] << '\n';
        }
        output.close();
        trace.close();
        if (!output || !trace)
        {
            throw std::runtime_error("prepared report write failed");
        }
        if (state.m_latency)
        {
            PreparedWriteLatency(argv[5], state, frequency);
        }
        std::cout << "PASS " << version << ' ' << mode << " total=" << total << " submit_ms=" << submitMs
                  << " complete_ms=" << completeMs << " retries=" << retries << '\n';
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL prepared benchmark: " << error.what() << '\n';
        return 1;
    }
    catch (...)
    {
        std::cerr << "FAIL prepared benchmark: unknown exception\n";
        return 1;
    }
}

CTaskPreparedSampledItem::CTaskPreparedSampledItem(CTaskPreparedState& state, size_t producer, size_t sequence, size_t cycle) :
CTaskPreparedItem(state, nullptr, state.m_expected[producer * state.m_levels + sequence % state.m_levels],
    sequence / state.m_levels, &state.m_repeatNext[producer * state.m_levels + sequence % state.m_levels], cycle),
m_expected(&state.m_expected[producer * state.m_levels + sequence % state.m_levels]),
m_samples(&state.m_samples[producer * ((state.m_total / state.m_producers + 1008) / 1009)]),
m_levels(state.m_levels),
m_level(sequence % state.m_levels),
m_total(state.m_total / state.m_producers),
m_completion(state.m_window != 0 ? &state.m_windowCompletions[producer].m_count : nullptr)
{

}

void CTaskPreparedSampledItem::doTask()
{
    const uint64_t sequence = *m_expected * m_levels + m_level;
    if (sequence < m_total && sequence % 1009 == 0)
    {
        m_samples[sequence / 1009].m_entered = CTaskPreparedState::now();
    }
    CTaskPreparedItem::doTask();
    if (m_completion != nullptr)
    {
        m_completion->store(sequence + 1, std::memory_order_release);
    }
}

void CTaskPreparedProducer::produceWindow()
{
    CTaskPreparedProducerData& progress = m_state->m_progress[m_producer];
    const size_t count = m_state->m_total / m_state->m_producers;
    const size_t samples = (count + 1008) / 1009;
    uint64_t accepted = 0;
    size_t taskIndex = m_state->m_taskOffsets[m_producer];
    const size_t taskEnd = m_state->m_taskOffsets[m_producer + 1];
    std::atomic<uint64_t>& completed = m_state->m_windowCompletions[m_producer].m_count;
    for (size_t sequence = 0; sequence < count && !m_state->m_stop.load(std::memory_order_acquire); ++sequence)
    {
        while (sequence - completed.load(std::memory_order_acquire) >= m_state->m_window &&
            !m_state->m_stop.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }
        if (m_state->m_stop.load(std::memory_order_acquire))
        {
            break;
        }
        CTaskPreparedSample* sample = sequence % 1009 == 0 ? &m_state->m_samples[m_producer * samples + sequence / 1009] : nullptr;
        const int32_t level = static_cast<int32_t>(sequence % m_state->m_levels) + 1;
        const uint64_t retriesBefore = progress.m_retries;
        if (sample != nullptr)
        {
            sample->m_begin = CTaskPreparedState::now();
        }
        const std::shared_ptr<CTask>& task = m_state->m_tasks[taskIndex];
#if defined(CTTM_BENCH_NEW) || defined(CTTM_TEST_NO_LEGACY_SEND_TASK)
        for (;;)
        {
            const CTaskThreadResult result = m_target->postTask(task, level);
            if (result.m_committed)
            {
                if (result.m_status != CTaskThreadOk)
                {
                    ++m_state->m_errors;
                }
                break;
            }
            if (result.m_status != CTaskThreadRetry || m_state->m_stop.load(std::memory_order_acquire))
            {
                throw std::runtime_error("windowed postTask rejected prepared task");
            }
            ++progress.m_retries;
            std::this_thread::yield();
        }
#else
        m_target->postTask(task, level);
#endif
        if (sample != nullptr)
        {
            sample->m_returned = CTaskPreparedState::now();
            sample->m_retries = progress.m_retries - retriesBefore;
        }
        ++accepted;
        if (++taskIndex == taskEnd)
        {
            taskIndex = m_state->m_taskOffsets[m_producer];
        }
        if ((accepted & 1023) == 0)
        {
            progress.m_published.store(accepted, std::memory_order_release);
        }
    }
    progress.m_consumedAtEnd = m_state->m_observed.load(std::memory_order_acquire);
    progress.m_end = CTaskPreparedState::now();
    progress.m_published.store(accepted, std::memory_order_release);
}

#ifdef CTTM_PREPARED_MAIN
int main(int argc, char** argv)
{
    return runPreparedTaskBenchmark(argc, argv);
}

#endif