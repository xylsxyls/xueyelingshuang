#define NOMINMAX
#include "LockFreeMapBenchmark.h"
#include "LockFreeMapBenchmarkClock.h"
#include "LockFreeMapTestThread.h"
#ifdef LOCK_FREE_MAP_BENCHMARK_PROGRESS
#include "LockFreeMapProgressTest.h"
#endif
#include <fstream>
#include <cerrno>
#include <iomanip>
#include <sstream>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstddef>
#include <type_traits>
#include <vector>
#include <thread>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#ifdef _WIN32
#include <Windows.h>
#include <Psapi.h>
#endif

std::atomic<size_t> LockFreeMapBenchmark::m_bytes(0), LockFreeMapBenchmark::m_peak(0);
std::atomic<uint64_t> LockFreeMapBenchmark::m_newCalls(0);
int LockFreeMapBenchmark::m_iterations = 200000;
bool LockFreeMapBenchmark::m_steady = false;
bool LockFreeMapBenchmark::m_pinWorkers = false;
std::vector<uintptr_t> LockFreeMapBenchmark::m_workerMasks;

bool LockFreeMapBenchmark::initializePlacement()
{
    if (!m_pinWorkers)
    {
        return true;
    }
#ifdef _WIN32
    DWORD_PTR processMask = 0, systemMask = 0;
    DWORD bytes = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask) ||
        GetLogicalProcessorInformation(nullptr, &bytes) || GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
        bytes == 0 || bytes % sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) != 0)
    {
        return false;
    }
    std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> topology(bytes / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
    if (!GetLogicalProcessorInformation(topology.data(), &bytes))
    {
        return false;
    }
    for (size_t index = 0; index < bytes / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++index)
    {
        if (topology[index].Relationship == RelationProcessorCore)
        {
            const DWORD_PTR available = topology[index].ProcessorMask & processMask;
            if (available != 0)
            {
                // 无符号运算提取最低有效位，只选择进程当前获准使用的处理器
                m_workerMasks.push_back(static_cast<uintptr_t>(available & (~available + 1)));
            }
        }
    }
    return !m_workerMasks.empty() && placeWorker(0);
#else
    return false;
#endif
}

bool LockFreeMapBenchmark::placeWorker(unsigned worker)
{
    if (!m_pinWorkers)
    {
        return true;
    }
#ifdef _WIN32
    return !m_workerMasks.empty() && SetThreadAffinityMask(GetCurrentThread(),
        static_cast<DWORD_PTR>(m_workerMasks[worker % m_workerMasks.size()])) != 0;
#else
    (void)worker;
    return false;
#endif
}

int LockFreeMapBenchmark::phase(std::ostream& report, LockFreeMapBenchmarkMap<int, int>& map, const char* name, int kind, int count)
{
    if (count <= 0 || kind < 0 || kind > 6 || name == nullptr)
    {
        return 1;
    }
    // 原短阶段仅数百微秒，容易受调度和 Wasm 分层编译影响；两版均执行相同倍数
    if (m_steady)
    {
        count *= 40;
    }
    std::vector<uint64_t> samples(count);
    const uint64_t beforeNew = LockFreeMapBenchmark::m_newCalls.load();
    LockFreeMapBenchmark::m_peak.store(LockFreeMapBenchmark::m_bytes.load());
    const uint64_t start = LockFreeMapBenchmarkClock::nanoseconds();
    int errors = 0;
    volatile uint64_t checksum = 0;
    for (int iteration = 0; iteration < count; ++iteration)
    {
        const uint64_t begin = LockFreeMapBenchmarkClock::nanoseconds();
        int key = iteration % 128, value = -1;
        if (kind == 0)
        {
            if (!map.get(key, value))
            {
                ++errors;
            }
            checksum += value;
        }
        else if (kind == 1)
        {
            if (!map.set(key, key * 2))
            {
                ++errors;
            }
        }
        else if (kind == 2)
        {
            if (!map.erase(key) || !map.set(key, key * 2))
            {
                ++errors;
            }
        }
        else if (kind == 3)
        {
            if (!map.last(key, value) || key != 127)
            {
                ++errors;
            }
            checksum += value;
        }
        else if (kind == 4)
        {
            if (!map.set(100000 + iteration, iteration) || !map.erase(100000 + iteration))
            {
                ++errors;
            }
        }
        else if (kind == 5)
        {
#ifndef LOCK_FREE_MAP_BASELINE
            size_t released = 0;
            if (map.lockFreeMapMaintain(4096, released) != LOCK_FREE_MAP_OK)
            {
                ++errors;
            }
#endif
        }
        else if (kind == 6)
        {
            map.clear();
        }
        const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
        if (begin == 0 || end < begin)
        {
            ++errors;
        }
        samples[iteration] = end >= begin ? end - begin : 0;
    }
    const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
    if (start == 0 || end <= start)
    {
        return 1;
    }
    const double seconds = static_cast<double>(end - start) / 1000000000.0;
    const uint64_t allocations = LockFreeMapBenchmark::m_newCalls.load() - beforeNew;
    // 独立校验放在计时之后，既保留旧负载口径，也拒绝只返回成功但内容错误的实现
    if (kind == 0)
    {
        const uint64_t tail = static_cast<uint64_t>(count % 128);
        const uint64_t expected = static_cast<uint64_t>(count / 128) * 128 * 127 + tail * (tail == 0 ? 0 : tail - 1);
        errors += checksum == expected ? 0 : 1;
    }
    else if (kind == 3)
    {
        errors += checksum == static_cast<uint64_t>(count) * 254 ? 0 : 1;
    }
    if (kind <= 4)
    {
        errors += map.size() == 128 ? 0 : 1;
        for (int key = 0; key < 128; ++key)
        {
            int value = -1;
            errors += map.get(key, value) && value == key * 2 ? 0 : 1;
        }
    }
    else
    {
        errors += map.empty() ? 0 : 1;
        if (kind == 5)
        {
            errors += map.retainedNodeCount() == 0 && map.retainedValueCount() == 0 ? 0 : 1;
        }
    }
    std::sort(samples.begin(), samples.end());
#ifndef LOCK_FREE_MAP_BASELINE
    uint64_t retries = 0;
    size_t nodeCapacity = 0, valueCapacity = 0, managed = 0;
    LockFreeMapStats stats = map.lockFreeMapStats();
    retries = stats.casRetries; nodeCapacity = stats.nodes.capacity; valueCapacity = stats.values.capacity; managed = stats.totalBytes;
#endif
    report << name << ',' << count << ',' << count / seconds << ',' << samples[count / 2] << ',' <<
    samples[count * 95 / 100] << ',' << samples[count * 99 / 100] << ',';
#ifdef LOCK_FREE_MAP_BENCHMARK_PLAIN
    (void)allocations;
    report << "NA,NA,NA,";
#else
    report << allocations << ',' << LockFreeMapBenchmark::m_peak.load() << ',' << LockFreeMapBenchmark::m_bytes.load() << ',';
#endif
    report << map.retainedNodeCount() << ',' << map.retainedValueCount() << ',';
#ifdef LOCK_FREE_MAP_BASELINE
    report << "NA,NA,NA,NA,";
#else
    report << nodeCapacity << ',' << valueCapacity << ',' << managed << ',' << retries << ',';
#endif
    report << errors << '\n';
    return errors;
}

int LockFreeMapBenchmark::diagnoseErase(std::ostream& report)
{
    LockFreeMapBenchmarkMap<int, int> map;
    int errors = 0;
    for (int key = 0; key < 128; ++key)
    {
        errors += !map.set(key, key * 2);
    }
    for (int key = 0; key < 128; ++key)
    {
        errors += !map.erase(key);
        errors += !map.set(key, key * 2);
    }
    report << "diagnostic=erase_128\n";
    errors += phase(report, map, "erase_insert", 2, 5000);
#ifdef __EMSCRIPTEN__
    // 仅测试模块使用；计数包装由独立JS助手安装，不给容器增加平台接口
    emscripten_run_script("globalThis.lockFreeMapInvokeCounts = {}; globalThis.lockFreeMapInvokeEnabled = true;");
#endif
    const uint64_t begin = LockFreeMapBenchmarkClock::nanoseconds();
    for (int iteration = 0; iteration < m_iterations; ++iteration)
    {
        const int key = iteration % 128;
        errors += !map.erase(key);
        errors += !map.set(key, key * 2);
    }
    const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
#ifdef __EMSCRIPTEN__
    emscripten_run_script("globalThis.lockFreeMapInvokeEnabled = false;");
#endif
    if (begin == 0 || end <= begin)
    {
        ++errors;
    }
    const double seconds = end > begin ? static_cast<double>(end - begin) / 1000000000.0 : 1;
    errors += map.size() != 128;
    for (int key = 0; key < 128; ++key)
    {
        int value = -1;
        errors += !map.get(key, value) || value != key * 2;
    }
    report << "erase_insert_bulk,cycles=" << m_iterations << ",cycles_per_second=" << m_iterations / seconds <<
        ",nanoseconds_per_cycle=" << seconds * 1000000000.0 / m_iterations << ",errors=" << errors << '\n';
#ifdef __EMSCRIPTEN__
    const char* bridges = emscripten_run_script_string("JSON.stringify(globalThis.lockFreeMapInvokeCounts)");
    if (bridges == nullptr)
    {
        ++errors;
    }
    else
    {
        report << "exception_bridge_counts=" << bridges << '\n';
    }
#endif
    report << "retained_keys=" << map.retainedNodeCount() << ",retained_values=" << map.retainedValueCount() << '\n';
    report << "failures=" << errors << '\n';
    return errors == 0 && report.good() ? 0 : 1;
}

int LockFreeMapBenchmark::changing(std::ostream& report, int keys, int workers)
{
    if (keys < 1 || workers < 1 || workers > 64 || (keys != 1 && (keys < workers || keys % workers != 0)))
    {
        return 1;
    }
    LockFreeMapBenchmarkMap<int, int> map;
    int errors = 0;
    const int iterations = m_iterations;
    for (int key = 0; key < keys; ++key)
    {
        if (!map.set(key, 0))
        {
            ++errors;
        }
    }
    // 测量稳定状态前预热备用值槽和索引退役
    // 计时内每次设置仍会改变值，单键容器也相同
    for (int key = 0; key < keys; ++key)
    {
        if (!map.set(key, -1))
        {
            ++errors;
        }
    }
    std::atomic<int> start(0), ready(0), failures(0);
#ifdef LOCK_FREE_MAP_BENCHMARK_SINGLE
    (void)ready;
#endif
    const auto work = [&](int worker)
    {
        for (int iteration = 0; iteration < iterations; ++iteration)
        {
            const int key = (iteration * workers + worker) % keys;
            if (!map.set(key, iteration + 1))
            {
                failures.fetch_add(1);
            }
        }
    };
#ifndef LOCK_FREE_MAP_BENCHMARK_SINGLE
    std::vector<LockFreeMapTestThread> threads;
    threads.reserve(workers);
    if (workers > 1)
    {
        try
        {
            for (int worker = 0; worker < workers; ++worker)
            {
                threads.push_back(LockFreeMapTestThread([&, worker]()
                {
                    if (!placeWorker(static_cast<unsigned>(worker)))
                    {
                        failures.fetch_add(1);
                    }
                    ready.fetch_add(1);
                    while (start.load() == 0)
                    {
                        std::this_thread::yield();
                    }
                    work(worker);
                }));
            }
        }
        catch (...)
        {
            start.store(1);
            for (size_t index = 0; index < threads.size(); ++index)
            {
                threads[index].join();
            }
            return 1;
        }
        while (ready.load() != workers)
        {
            std::this_thread::yield();
        }
    }
#else
    if (workers != 1)
    {
        return 1;
    }
#endif
    const uint64_t beforeNew = LockFreeMapBenchmark::m_newCalls.load();
    const uint64_t begin = LockFreeMapBenchmarkClock::nanoseconds();
    start.store(1);
    if (workers == 1)
    {
        work(0);
    }
#ifndef LOCK_FREE_MAP_BENCHMARK_SINGLE
    for (size_t index = 0; index < threads.size(); ++index)
    {
        threads[index].join();
    }
#endif
    const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
    if (begin == 0 || end <= begin)
    {
        return 1;
    }
    const uint64_t elapsed = end - begin;
    const uint64_t allocations = LockFreeMapBenchmark::m_newCalls.load() - beforeNew;
    errors += failures.load();
    for (int key = 0; key < keys; ++key)
    {
        int value = 0;
        const int expected = keys == 1 ? iterations : iterations - ((iterations - 1 - key / workers) % (keys / workers));
        if (!map.get(key, value) || value != expected)
        {
            ++errors;
        }
    }
#ifndef LOCK_FREE_MAP_BASELINE
    size_t released = 0;
    map.lockFreeMapMaintain(65536, released);
    if (map.retainedValueCount() != static_cast<size_t>(keys))
    {
        ++errors;
    }
#endif
    report << "changing,keys=" << keys << ",threads=" << workers << ",operations=" << iterations * workers <<
        ",ops_per_second=" << static_cast<double>(iterations) * workers * 1000000000.0 / elapsed <<
        ",new_calls=";
#ifdef LOCK_FREE_MAP_BENCHMARK_PLAIN
    (void)allocations;
    report << "NA";
#else
    report << allocations;
#endif
    report << ",retained=" << map.retainedValueCount() << ",errors=" << errors << '\n';
    return errors + (begin == 0 || elapsed == 0 ? 1 : 0);
}

int LockFreeMapBenchmark::randomUpdates(std::ostream& report, int keys)
{
    if (keys < 1)
    {
        return 1;
    }
    const int iterations = m_iterations;
    const uint32_t seed = 179424673;
    uint32_t state = seed;
    std::vector<int> sequence(iterations), expected(keys, 0);
    LockFreeMapBenchmarkMap<int, int> map;
    int errors = 0;
    for (int key = 0; key < keys; ++key)
    {
        errors += !map.set(key, 0);
        errors += !map.set(key, 0);
    }
    for (int index = 0; index < iterations; ++index)
    {
        state = state * 1664525u + 1013904223u;
        sequence[index] = static_cast<int>((state >> 8) % keys);
        expected[sequence[index]] = index + 1;
    }
    const uint64_t beforeNew = LockFreeMapBenchmark::m_newCalls.load();
    const uint64_t begin = LockFreeMapBenchmarkClock::nanoseconds();
    for (int index = 0; index < iterations; ++index)
    {
        errors += !map.set(sequence[index], index + 1);
    }
    const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
    if (begin == 0 || end <= begin)
    {
        return 1;
    }
    const uint64_t elapsed = end - begin;
    const uint64_t allocations = LockFreeMapBenchmark::m_newCalls.load() - beforeNew;
    for (int key = 0; key < keys; ++key)
    {
        int value = -1;
        errors += !map.get(key, value) || value != expected[key];
    }
#ifndef LOCK_FREE_MAP_BASELINE
    size_t released = 0;
    map.lockFreeMapMaintain(65536, released);
    errors += map.retainedValueCount() != static_cast<size_t>(keys);
#endif
    report << "random_updates,keys=" << keys << ",threads=1,seed=" << seed << ",operations=" << iterations <<
        ",ops_per_second=" << static_cast<double>(iterations) * 1000000000.0 / elapsed << ",new_calls=";
#ifdef LOCK_FREE_MAP_BENCHMARK_PLAIN
    (void)allocations;
    report << "NA";
#else
    report << allocations;
#endif
    report << ",retained=" << map.retainedValueCount() << ",errors=" << errors << '\n';
    return errors + (begin == 0 || elapsed == 0 ? 1 : 0);
}

int LockFreeMapBenchmark::longKeyChurn(std::ostream& report)
{
    LockFreeMapBenchmarkMap<std::string, int> map;
    const std::string key(4096, 'k');
    const int iterations = m_iterations / 4;
    int errors = !map.set(key, -1) || !map.erase(key) ? 1 : 0;
    const uint64_t beforeNew = m_newCalls.load();
    const uint64_t begin = LockFreeMapBenchmarkClock::nanoseconds();
    for (int index = 0; index < iterations; ++index)
    {
        if (!map.set(key, index) || !map.erase(key))
        {
            ++errors;
        }
    }
    const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
    if (begin == 0 || end <= begin)
    {
        return 1;
    }
    const uint64_t elapsed = end - begin;
    const uint64_t allocations = m_newCalls.load() - beforeNew;
    errors += map.empty() ? 0 : 1;
    report << "long_key_churn,keys=1,threads=1,operations=" << iterations * 2 << ",ops_per_second=" <<
        static_cast<double>(iterations) * 2 * 1000000000.0 / elapsed << ",new_calls=";
#ifdef LOCK_FREE_MAP_BENCHMARK_PLAIN
    (void)allocations;
    report << "NA";
#else
    report << allocations;
#endif
    report << ",errors=" << errors << '\n';
    return errors + (begin == 0 || elapsed == 0 ? 1 : 0);
}

int LockFreeMapBenchmark::timerOverhead(std::ostream& report, int count)
{
    if (count < 1)
    {
        return 1;
    }
    std::vector<uint64_t> samples(count);
    const uint64_t start = LockFreeMapBenchmarkClock::nanoseconds();
    for (int index = 0; index < count; ++index)
    {
        const uint64_t begin = LockFreeMapBenchmarkClock::nanoseconds();
        const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
        if (begin == 0 || end < begin)
        {
            return 1;
        }
        samples[index] = end - begin;
    }
    const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
    if (start == 0 || end <= start)
    {
        return 1;
    }
    const uint64_t elapsed = end - start;
    uint64_t checksum = 0;
    for (int index = 0; index < count; ++index)
    {
        checksum += samples[index];
    }
    if (start == 0 || elapsed == 0)
    {
        return 1;
    }
    report << "timer_only,iterations=" << count << ",ops_per_second=" <<
        static_cast<double>(count) * 1000000000.0 / elapsed << ",ns_per_iteration=" <<
        static_cast<double>(elapsed) / count << ",checksum=" << checksum << '\n';
    return 0;
}

int LockFreeMapBenchmark::eraseInsertBulk(std::ostream& report)
{
    LockFreeMapBenchmarkMap<int, int> map;
    int errors = 0;
    for (int key = 0; key < 128; ++key)
    {
        errors += !map.set(key, key * 2);
    }
    // 两端同样预热一次完整键集合，不在计时循环中调用时钟或输出报告
    for (int key = 0; key < 128; ++key)
    {
        errors += !map.erase(key);
        errors += !map.set(key, key * 2);
    }
    const uint64_t allocations = m_newCalls.load();
    const uint64_t begin = LockFreeMapBenchmarkClock::nanoseconds();
    for (int iteration = 0; iteration < m_iterations; ++iteration)
    {
        const int key = iteration % 128;
        errors += !map.erase(key);
        errors += !map.set(key, key * 2);
    }
    const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
    if (begin == 0 || end <= begin)
    {
        return 1;
    }
    const uint64_t allocated = m_newCalls.load() - allocations;
    errors += map.size() != 128;
    for (int key = 0; key < 128; ++key)
    {
        int value = -1;
        errors += !map.get(key, value) || value != key * 2;
    }
    report << "erase_insert_bulk,keys=128,threads=1,cycles=" << m_iterations << ",ops_per_second=" <<
        static_cast<double>(m_iterations) * 1000000000.0 / (end - begin) << ",new_calls=";
#ifdef LOCK_FREE_MAP_BENCHMARK_PLAIN
    (void)allocated;
    report << "NA";
#else
    report << allocated;
#endif
    report << ",retained_keys=" << map.retainedNodeCount() << ",retained_values=" <<
        map.retainedValueCount() << ",errors=" << errors << '\n';
    return errors;
}

/** 保持原有阶段输入，仅在所有独立检查通过后返回成功
*/
int LockFreeMapBenchmark::run(std::ostream& report)
{
#ifdef LOCK_FREE_MAP_BENCHMARK_STD_MUTEX
    report << "container=std_map_mutex\n";
#else
    report << "container=lock_free_map\n";
#endif
#ifdef LOCK_FREE_MAP_BENCHMARK_SINGLE
    report << "thread_model=single\n";
#else
    report << "thread_model=concurrent\n";
#endif
#ifdef LOCK_FREE_MAP_BENCHMARK_PLAIN
    report << "allocator_instrumentation=disabled\n";
#else
    report << "allocator_instrumentation=enabled\n";
#endif
#ifdef LOCK_FREE_MAP_BASELINE
    // 原始版本没有维护接口，共有接口对照时两端都使用这一负载集合
    report << "interface_set=legacy\n";
#else
    report << "interface_set=full\n";
#endif
    report << "phase,iterations,ops_per_second,p50_ns,p95_ns,p99_ns,new_calls,peak_bytes,live_bytes,retained_keys,retained_values,node_capacity,value_capacity,managed_bytes,retries,errors\n";
    // 先初始化当前报告流的整数和浮点格式化状态，再观察容器生命周期净申请
    // 运行库可能把这部分缓存持有到报告流析构，必须与容器保留量分开记录
    const size_t beforeFormat = m_bytes.load();
    report << "format_warmup=" << static_cast<uint64_t>(0) << '/' << 0.0 << '\n';
#ifdef LOCK_FREE_MAP_BENCHMARK_PLAIN
    (void)beforeFormat;
    report << "format_warmup_bytes=NA\n";
#else
    report << "format_warmup_bytes=" << m_bytes.load() - beforeFormat << '\n';
#endif
    report << "measurement_iterations=" << m_iterations << '\n';
    report << "steady_measurement=" << (m_steady ? 1 : 0) << '\n';
    report << "worker_placement=" << (m_pinWorkers ? "windows:" : "runtime");
    for (size_t index = 0; index < m_workerMasks.size(); ++index)
    {
        report << (index == 0 ? "" : "/") << m_workerMasks[index];
    }
    report << '\n';
    report << "bulk_erase_insert=1\n";
    const size_t initialBytes = LockFreeMapBenchmark::m_bytes.load();
    int errors = 0;
    try
    {
#ifndef LOCK_FREE_MAP_BENCHMARK_PLAIN
        // 在计时前独立确认非抛出申请和对应释放使用同一个字节跟踪器
        for (int array = 0; array < 2; ++array)
        {
            const uint64_t before = m_newCalls.load();
            void* memory = array == 0 ? ::operator new(31, std::nothrow) : ::operator new[](31, std::nothrow);
            const bool tracked = memory != nullptr && m_newCalls.load() == before + 1 && m_bytes.load() == initialBytes + 31;
            if (array == 0)
            {
                ::operator delete(memory, std::nothrow);
            }
            else
            {
                ::operator delete[](memory, std::nothrow);
            }
            if (!tracked || m_bytes.load() != initialBytes)
            {
                ++errors;
            }
        }
#endif
        LockFreeMapBenchmarkMap<int, int> map;
        // 先核对缺失键、空末项和重复删除，失败不能进入有效吞吐对照
        int absentKey = 0, absentValue = 0;
        errors += !map.empty() || map.get(0, absentValue) || map.erase(0) || map.last(absentKey, absentValue);
        for (int key = 0; key < 128; ++key)
        {
            if (!map.set(key, key * 2))
            {
                ++errors;
            }
        }
        errors += LockFreeMapBenchmark::phase(report, map, "get", 0, 20000);
        errors += LockFreeMapBenchmark::phase(report, map, "update", 1, 5000);
        errors += LockFreeMapBenchmark::phase(report, map, "erase_insert", 2, 5000);
        errors += LockFreeMapBenchmark::phase(report, map, "maximum", 3, 10000);
        errors += LockFreeMapBenchmark::phase(report, map, "history_churn", 4, 20000);
        errors += LockFreeMapBenchmark::phase(report, map, "get_after_history", 0, 20000);
        errors += LockFreeMapBenchmark::phase(report, map, "logical_clear", 6, 1000);
#ifndef LOCK_FREE_MAP_BASELINE
        errors += LockFreeMapBenchmark::phase(report, map, "reclaim", 5, 1000);
#endif
        map.clearAndFreeMemory();
#ifdef LOCK_FREE_MAP_BENCHMARK_PLAIN
        report << "explicit_free_bytes=NA\n";
#else
        report << "explicit_free_bytes=" << LockFreeMapBenchmark::m_bytes.load() << '\n';
#endif
        errors += LockFreeMapBenchmark::changing(report, 1, 1);
        errors += LockFreeMapBenchmark::changing(report, 128, 1);
        errors += LockFreeMapBenchmark::changing(report, 4096, 1);
#ifndef LOCK_FREE_MAP_BENCHMARK_SINGLE
        errors += LockFreeMapBenchmark::changing(report, 128, 4);
        errors += LockFreeMapBenchmark::changing(report, 128, 8);
        errors += LockFreeMapBenchmark::changing(report, 1, 4);
#endif
        errors += LockFreeMapBenchmark::randomUpdates(report, 128);
        errors += LockFreeMapBenchmark::randomUpdates(report, 4096);
        errors += LockFreeMapBenchmark::longKeyChurn(report);
        errors += LockFreeMapBenchmark::eraseInsertBulk(report);
#ifndef LOCK_FREE_MAP_BENCHMARK_PLAIN
        const size_t beforeTimers = m_bytes.load();
#endif
        errors += LockFreeMapBenchmark::timerOverhead(report, 5000);
#ifndef LOCK_FREE_MAP_BENCHMARK_PLAIN
        errors += m_bytes.load() == beforeTimers ? 0 : 1;
#endif
        errors += LockFreeMapBenchmark::timerOverhead(report, 200000);
#ifndef LOCK_FREE_MAP_BENCHMARK_PLAIN
        errors += m_bytes.load() == beforeTimers ? 0 : 1;
#endif
    }
    catch (...)
    {
        ++errors;
    }
#ifdef LOCK_FREE_MAP_BENCHMARK_PLAIN
    report << "destruction_delta_bytes=NA\n";
#else
    report << "destruction_delta_bytes=" << static_cast<int64_t>(LockFreeMapBenchmark::m_bytes.load()) - static_cast<int64_t>(initialBytes) << '\n';
#endif
    if (LockFreeMapBenchmark::m_bytes.load() != initialBytes)
    {
        ++errors;
    }
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS memory = {};
    memory.cb = sizeof(memory);
    DWORD handles = 0;
    FILETIME created, exited, kernel, user;
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)) ||
        !GetProcessHandleCount(GetCurrentProcess(), &handles) || !GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
    {
        ++errors;
    }
    else
    {
        const uint64_t cpu = (static_cast<uint64_t>(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
        (static_cast<uint64_t>(user.dwHighDateTime) << 32) + user.dwLowDateTime;
        report << "process_peak_working_set=" << memory.PeakWorkingSetSize << " handles=" << handles << " cpu_100ns=" << cpu << '\n';
    }
#endif
#ifndef LOCK_FREE_MAP_BENCHMARK_SINGLE
    if (LockFreeMapTestThread::m_errors.load() != 0)
    {
        ++errors;
    }
#endif
    report << "failures=" << errors << '\n';
    return errors == 0 && report.good() ? 0 : 1;
}

bool LockFreeMapBenchmark::validWorkload(const std::vector<std::string>& fields, unsigned iterations, bool steady, bool single)
{
    if (fields.size() < 3 || iterations == 0)
    {
        return false;
    }
    const std::string& name = fields[0];
    unsigned count = 0;
    size_t column = 1;
    std::string prefix;
    if (name == "changing")
    {
        const bool one = fields[2] == "threads=1";
        const bool four = !single && fields[2] == "threads=4";
        const bool eight = !single && fields[2] == "threads=8";
        if (fields.size() != 8 || !((one && (fields[1] == "keys=1" || fields[1] == "keys=128" || fields[1] == "keys=4096")) ||
            (four && (fields[1] == "keys=1" || fields[1] == "keys=128")) || (eight && fields[1] == "keys=128")))
        {
            return false;
        }
        count = iterations * (one ? 1 : four ? 4 : 8);
        column = 3;
        prefix = "operations=";
    }
    else if (name == "random_updates")
    {
        if (fields.size() != 9 || (fields[1] != "keys=128" && fields[1] != "keys=4096") ||
            fields[2] != "threads=1" || fields[3] != "seed=179424673")
        {
            return false;
        }
        count = iterations;
        column = 4;
        prefix = "operations=";
    }
    else if (name == "long_key_churn" || name == "erase_insert_bulk")
    {
        const bool longKey = name == "long_key_churn";
        if (fields.size() != (longKey ? 7 : 9) || fields[1] != (longKey ? "keys=1" : "keys=128") || fields[2] != "threads=1")
        {
            return false;
        }
        count = longKey ? iterations / 2 : iterations;
        column = 3;
        prefix = longKey ? "operations=" : "cycles=";
    }
    else
    {
        if (fields.size() != 16)
        {
            return false;
        }
        if (name == "get" || name == "get_after_history" || name == "history_churn")
        {
            count = 20000;
        }
        else if (name == "update" || name == "erase_insert")
        {
            count = 5000;
        }
        else if (name == "maximum")
        {
            count = 10000;
        }
        else if (name == "logical_clear" || name == "reclaim")
        {
            count = 1000;
        }
        else
        {
            return false;
        }
        count *= steady ? 40 : 1;
    }
    // 数量由已验证的模式推导，再逐字核对，不接受符号、尾随字符或溢出文本
    std::ostringstream expected;
    expected << prefix << count;
    return expected.good() && fields[column] == expected.str();
}

bool LockFreeMapBenchmark::read(const char* path, std::map<std::string, double>& values, unsigned& iterations,
    bool& steady, std::string& placement)
{
    iterations = 0;
    steady = false;
    placement = "worker_placement=runtime";
    if (path == nullptr || *path == 0)
    {
        return false;
    }
    std::ifstream input(path, std::ios::binary);
    std::string line;
    bool plain = false;
    bool passed = false;
    bool legacy = false;
    bool interfaceSeen = false;
    bool bulk = false;
    bool single = false;
    bool threadModelSeen = false;
    bool steadySeen = false;
    bool placementSeen = false;
    while (std::getline(input, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
        {
            line.erase(line.size() - 1);
        }
        if (line.find("thread_model=") == 0)
        {
            if (threadModelSeen || (line != "thread_model=single" && line != "thread_model=concurrent"))
            {
                return false;
            }
            threadModelSeen = true;
            single = line == "thread_model=single";
        }
        if (line.find("allocator_instrumentation=") == 0)
        {
            if (line != "allocator_instrumentation=disabled" || plain)
            {
                return false;
            }
            plain = true;
        }
        if (line.find("failures=") == 0)
        {
            if (line != "failures=0" || passed)
            {
                return false;
            }
            passed = true;
        }
        if (line.find("measurement_iterations=") == 0)
        {
            if (iterations != 0 || (line != "measurement_iterations=200000" && line != "measurement_iterations=4000000"))
            {
                return false;
            }
            iterations = line == "measurement_iterations=200000" ? 200000 : 4000000;
        }
        if (line.find("steady_measurement=") == 0)
        {
            if (steadySeen || (line != "steady_measurement=0" && line != "steady_measurement=1"))
            {
                return false;
            }
            steadySeen = true;
            steady = line == "steady_measurement=1";
        }
        if (line.find("worker_placement=") == 0)
        {
            if (placementSeen || (line != "worker_placement=runtime" &&
                (line.find("worker_placement=windows:") != 0 || line.size() <= 25)))
            {
                return false;
            }
            placementSeen = true;
            placement = line;
        }
        if (line.find("interface_set=") == 0)
        {
            if (interfaceSeen || (line != "interface_set=legacy" && line != "interface_set=full"))
            {
                return false;
            }
            interfaceSeen = true;
            legacy = line == "interface_set=legacy";
        }
        if (line.find("bulk_erase_insert=") == 0)
        {
            if (bulk || line != "bulk_erase_insert=1")
            {
                return false;
            }
            bulk = true;
        }
        if (line.find("timer_only,") == 0 || line.find("phase,") == 0 || line.find(',') == std::string::npos)
        {
            continue;
        }
        std::vector<std::string> fields;
        std::istringstream row(line);
        std::string field;
        while (std::getline(row, field, ','))
        {
            fields.push_back(field);
        }
        if (!validWorkload(fields, iterations, steady, single))
        {
            return false;
        }
        std::string name = fields[0];
        std::string rate;
        if (name == "changing" || name == "random_updates" || name == "long_key_churn" || name == "erase_insert_bulk")
        {
            name += '/' + fields[1] + '/' + fields[2];
            if (fields.back() != "errors=0")
            {
                return false;
            }
            for (size_t index = 3; index < fields.size(); ++index)
            {
                if (fields[index].find("ops_per_second=") == 0)
                {
                    rate = fields[index].substr(15);
                }
            }
        }
        else
        {
            if (fields.size() != 16 || fields.back() != "0")
            {
                return false;
            }
            rate = fields[2];
        }
        char* end = nullptr;
        errno = 0;
        const double value = std::strtod(rate.c_str(), &end);
        if (errno != 0 || end == rate.c_str() || *end != 0 || !(value > 0) ||
            value > (std::numeric_limits<double>::max)() || !values.insert(std::make_pair(name, value)).second)
        {
            return false;
        }
    }
    // 早期完整报告没有集合标记，仍必须含全部17项；缺项不能自动降级为共有接口报告
    return plain && passed && iterations != 0 && input.eof() && !input.bad() &&
        values.size() == static_cast<size_t>((legacy ? 16 : 17) + (bulk ? 1 : 0) - (single ? 3 : 0)) &&
        values.count("changing/keys=128/threads=4") == (single ? 0 : 1) &&
        values.count("changing/keys=128/threads=8") == (single ? 0 : 1) &&
        values.count("changing/keys=1/threads=4") == (single ? 0 : 1) &&
        values.count("erase_insert_bulk/keys=128/threads=1") == (bulk ? 1 : 0) &&
        (legacy ? values.count("reclaim") == 0 : values.count("reclaim") == 1);
}

int LockFreeMapBenchmark::compare(int argc, char** argv)
{
    if (argc < 3 || argc % 2 != 1)
    {
        return 2;
    }
    if (*argv[0] == 0)
    {
        return 3;
    }
    std::map<std::string, std::vector<double>> before, after;
    unsigned iterations = 0;
    bool steady = false;
    std::string placement;
    for (int index = 1; index < argc; index += 2)
    {
        std::map<std::string, double> left, right;
        unsigned leftCount = 0, rightCount = 0;
        bool leftSteady = false, rightSteady = false;
        std::string leftPlacement, rightPlacement;
        if (!read(argv[index], left, leftCount, leftSteady, leftPlacement) ||
            !read(argv[index + 1], right, rightCount, rightSteady, rightPlacement) ||
            left.size() != right.size() || leftCount != rightCount ||
            leftSteady != rightSteady || leftPlacement != rightPlacement ||
            (iterations != 0 && (iterations != leftCount || before.size() != left.size() ||
                steady != leftSteady || placement != leftPlacement)))
        {
            return 1;
        }
        iterations = leftCount;
        steady = leftSteady;
        placement = leftPlacement;
        for (auto item = left.begin(); item != left.end(); ++item)
        {
            const auto match = right.find(item->first);
            if (match == right.end())
            {
                return 1;
            }
            before[item->first].push_back(item->second);
            after[item->first].push_back(match->second);
        }
    }
    std::ofstream report(argv[0], std::ios::binary);
    report << "phase,measurement_iterations,pairs,before_min,before_median,before_max,after_min,after_median,after_max,change_percent,steady_measurement\n";
    report << std::fixed << std::setprecision(2);
    for (auto entry = before.begin(); entry != before.end(); ++entry)
    {
        std::vector<double>& left = entry->second;
        std::vector<double>& right = after.find(entry->first)->second;
        std::sort(left.begin(), left.end());
        std::sort(right.begin(), right.end());
        const size_t count = left.size();
        const double leftMedian = (left[count / 2] + left[(count - 1) / 2]) / 2;
        const double rightMedian = (right[count / 2] + right[(count - 1) / 2]) / 2;
        report << entry->first << ',' << iterations << ',' << count << ',' << left.front() << ',' << leftMedian << ',' << left.back() << ',' <<
            right.front() << ',' << rightMedian << ',' << right.back() << ',' << (rightMedian / leftMedian - 1) * 100 << ',' << (steady ? 1 : 0) << '\n';
    }
    report.close();
    return report.fail() ? 3 : 0;
}

int main(int argc, char** argv)
{
    try
    {
        if (argc > 1 && std::string(argv[1]) == "--compare")
        {
            return LockFreeMapBenchmark::compare(argc - 2, argv + 2);
        }
#ifdef LOCK_FREE_MAP_BENCHMARK_PROGRESS
        if (argc > 1 && std::string(argv[1]) == "--query-churn")
        {
            if (argc != 3)
            {
                return 2;
            }
            if (*argv[2] == 0)
            {
                return 3;
            }
            std::ofstream report(argv[2], std::ios::binary);
            if (!report)
            {
                return 3;
            }
            const unsigned changes = LockFreeMapProgressTest::queryChurn(report);
            report.close();
            return report.fail() ? 3 : changes == 0 ? 1 : 0;
        }
        const bool progressSteady = argc > 1 && std::string(argv[1]) == "--progress-steady";
        if (progressSteady || (argc > 1 && std::string(argv[1]) == "--progress"))
        {
            if (argc != 3)
            {
                return 2;
            }
            if (*argv[2] == 0)
            {
                return 3;
            }
            if (progressSteady)
            {
                // 每种负载先完整执行一轮并校验内容，预热数据单独保留，不混入正式吞吐
                std::ofstream warmup((std::string(argv[2]) + ".warmup.csv").c_str(), std::ios::binary);
                if (!warmup)
                {
                    return 3;
                }
                const int warmupResult = LockFreeMapProgressTest::run(warmup, 2000);
                warmup.close();
                if (warmupResult != 0 || warmup.fail())
                {
                    return 1;
                }
            }
            std::ofstream report(argv[2], std::ios::binary);
            if (!report)
            {
                return 3;
            }
            report << "progress_warmup," << (progressSteady ? "complete" : "none") << '\n';
            const int result = LockFreeMapProgressTest::run(report, 2000);
            report.close();
            return report.fail() ? 3 : result;
        }
#endif
        const bool diagnostic = argc > 1 && std::string(argv[1]) == "--erase-diagnose";
        const bool extended = argc > 1 && std::string(argv[1]) == "--extended";
        const bool pinned = argc > 1 && std::string(argv[1]) == "--pinned";
        const bool steady = pinned || (argc > 1 && std::string(argv[1]) == "--steady");
        if (((extended || diagnostic || steady) && argc != 3) || (!extended && !diagnostic && !steady && argc > 2))
        {
            return 2;
        }
        const char* path = extended || diagnostic || steady ? argv[2] : argc == 2 ? argv[1] : "LockFreeMapBenchmark-result.csv";
        if (*path == 0)
        {
            return 3;
        }
        LockFreeMapBenchmark::m_iterations = extended || diagnostic || steady ? 4000000 : 200000;
        LockFreeMapBenchmark::m_steady = steady;
        LockFreeMapBenchmark::m_pinWorkers = pinned;
        if (!LockFreeMapBenchmark::initializePlacement())
        {
            return 2;
        }
        if (steady)
        {
            // 预热仍执行全部内容检查，失败立即退出，不剔除慢样本或失败样本
            std::ofstream warmup((std::string(path) + ".warmup.csv").c_str(), std::ios::binary);
            if (!warmup)
            {
                return 3;
            }
            const int result = LockFreeMapBenchmark::run(warmup);
            warmup.close();
            if (result != 0 || warmup.fail())
            {
                return 1;
            }
        }
        std::ofstream report(path, std::ios::binary);
        if (!report)
        {
            return 3;
        }
        const int result = diagnostic ? LockFreeMapBenchmark::diagnoseErase(report) : LockFreeMapBenchmark::run(report);
        report.close();
        return report.fail() ? 3 : result;
    }
    catch (...)
    {
        return 1;
    }
}