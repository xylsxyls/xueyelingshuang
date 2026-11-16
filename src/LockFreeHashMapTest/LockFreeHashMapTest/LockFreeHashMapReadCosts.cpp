#include "LockFreeHashMapAPI.h"
#include "LockFreeHashMapTestThread.h"
#include "LockFreeHashMapPerfModels.h"
#include "LockFreeHashMapPerfValidation.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <stdexcept>
#include <string>
#include <cstring>
#if defined(_WIN32)
#include <Windows.h>
#endif

/** 读取诊断使用单调时钟，不改动正式性能用例
@return 秒数，时钟失败以异常交给诊断入口，不能报告无效吞吐
*/
static double ReadClock()
{
#if defined(_WIN32)
    LARGE_INTEGER frequency = {}, tick = {};
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0 || !QueryPerformanceCounter(&tick))
    {
        throw std::runtime_error("读取诊断时钟失败");
    }
    return double(tick.QuadPart) / double(frequency.QuadPart);
#else
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

/** 比较相邻输出和分离输出，只用于定位缓存行争用，不能替代验收数据
@param [in] threads 线程数
@param [in] stride 相邻线程输出间隔，单位为整数
@param [in] control 是否在不访问容器的情况下模拟每次读取的本线程原子屏障
@return 校验和计时有效时返回 true
*/
static bool ReadBatch(size_t threads, size_t stride, bool control)
{
    LockFreeHashMap<int, int> map(4096);
    for (int key = 0; key < 4096; ++key)
    {
        if (!map.set(key, key + 1))
        {
            return false;
        }
    }
    const size_t iterations = 2000000;
    std::vector<int> outputs(threads * stride, 0);
    std::vector<uint64_t> failures(threads, 0);
    std::atomic<size_t> ready(0);
    std::atomic<bool> start(false);
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(threads);
    double begin = 0;
    try
    {
        for (size_t thread = 0; thread < threads; ++thread)
        {
            workers.push_back(LockFreeHashMapTestThread([&, thread]()
            {
                // 本线程的原子对象在栈上，与其他线程没有共享写入
                LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<uint64_t> fence(0);
                int& output = outputs[thread * stride];
                uint32_t random = 123 + static_cast<uint32_t>(thread);
                uint64_t errors = 0;
                ready.fetch_add(1);
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                for (size_t i = 0; i < iterations; ++i)
                {
                    random = random * 1664525U + 1013904223U;
                    int key = static_cast<int>((random >> 8) & 4095);
                    if (control)
                    {
                        uint64_t zero = 0;
                        if (!fence.compare_exchange_strong(zero, 1))
                        {
                            ++errors;
                        }
                        output = key + 1;
                        fence.store(0, std::memory_order_release);
                    }
                    else if (!map.get(key, output))
                    {
                        ++errors;
                    }
                    if (output != key + 1)
                    {
                        ++errors;
                    }
                }
                failures[thread] = errors;
            }));
        }
        while (ready.load() != threads)
        {
            std::this_thread::yield();
        }
        begin = ReadClock();
        start.store(true);
    }
    catch (...)
    {
        start.store(true);
        throw;
    }
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    double elapsed = ReadClock() - begin;
    uint64_t errors = 0;
    for (size_t i = 0; i < failures.size(); ++i)
    {
        errors += failures[i];
    }
    std::cout << "READ_COST threads=" << threads << " stride=" << stride << " control=" << control
        << " seconds=" << elapsed << " ops=" << double(iterations * threads) / elapsed
        << " errors=" << errors << std::endl;
    return elapsed > 0 && errors == 0 && LockFreeHashMapTestThread::failures() == 0;
}

/** 对照字符串输出缓冲区的缓存行相邻情况，不修改生产库或正式性能负载
@param [in] padded 为 true 时在计时前给每个输出预留 512 字节，否则只预留最大输入长度
@return 全部完整字符串正确、线程正常退出且计时有效时为 true；地址只作为诊断证据
*/
static bool StringReadBatch(bool padded)
{
    const size_t threads = 4;
    const size_t iterations = 2000000;
    LockFreeHashMap<int, std::string> map(4096);
    std::vector<std::string> expected;
    for (size_t key = 0; key < 256; ++key)
    {
        expected.push_back(std::string(64, 'x') + std::to_string(key));
        if (!map.set(static_cast<int>(key), expected.back()))
        {
            return false;
        }
    }
    uintptr_t addresses[threads] = {}, finalAddresses[threads] = {};
    size_t capacities[threads] = {};
    uint64_t failures[threads] = {};
    std::atomic<size_t> ready(0);
    std::atomic<bool> start(false), cancelled(false);
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(threads);
    const int previousFailures = LockFreeHashMapTestThread::failures();
    double begin = 0;
    try
    {
        for (size_t thread = 0; thread < threads; ++thread)
        {
            // 每线程独占栈输出，地址和容量在 ready 前发布；诊断打印在全部线程退出后进行。
            workers.push_back(LockFreeHashMapTestThread([&, thread]()
            {
                bool entered = false;
                try
                {
                    LockFreeHashMapPerfModels::bindWorker(thread);
                    std::string output(expected[0]);
                    output.reserve(padded ? 512 : expected.back().size());
                    addresses[thread] = reinterpret_cast<uintptr_t>(output.data());
                    capacities[thread] = output.capacity();
                    entered = true;
                    ready.fetch_add(1);
                    while (!start.load())
                    {
                        std::this_thread::yield();
                    }
                    uint32_t random = 123 + static_cast<uint32_t>(thread);
                    uint64_t errors = 0;
                    for (size_t i = 0; i < iterations && !cancelled.load(); ++i)
                    {
                        random = random * 1664525U + 1013904223U;
                        size_t key = thread * 64 + ((random >> 8) & 63);
                        if (!map.get(static_cast<int>(key), output) || output != expected[key])
                        {
                            ++errors;
                        }
                    }
                    finalAddresses[thread] = reinterpret_cast<uintptr_t>(output.data());
                    failures[thread] = errors;
                }
                catch (...)
                {
                    cancelled.store(true);
                    if (!entered)
                    {
                        ready.fetch_add(1);
                    }
                    throw;
                }
            }));
        }
        while (ready.load() != threads)
        {
            std::this_thread::yield();
        }
        begin = ReadClock();
        start.store(true);
    }
    catch (...)
    {
        cancelled.store(true);
        start.store(true);
        throw;
    }
    for (size_t thread = 0; thread < threads; ++thread)
    {
        workers[thread].join();
    }
    const double elapsed = ReadClock() - begin;
    uint64_t errors = 0;
    size_t sharedLines = 0;
    bool stableAddresses = true;
    // 按本机 64 字节缓存行计算实际写入范围交集；512 字节预留不等于强制对齐。
    for (size_t thread = 0; thread < threads; ++thread)
    {
        errors += failures[thread];
        stableAddresses = stableAddresses && addresses[thread] == finalAddresses[thread];
        const uintptr_t first = addresses[thread] / 64;
        const uintptr_t last = (addresses[thread] + expected.back().size()) / 64;
        for (size_t other = 0; other < thread; ++other)
        {
            const uintptr_t otherFirst = addresses[other] / 64;
            const uintptr_t otherLast = (addresses[other] + expected.back().size()) / 64;
            sharedLines += first <= otherLast && otherFirst <= last ? 1 : 0;
        }
        std::cout << "STRING_OUTPUT thread=" << thread << " address=" << addresses[thread]
            << " final_address=" << finalAddresses[thread] << " capacity=" << capacities[thread] << std::endl;
    }
    std::cout << "STRING_READ_COST padded=" << padded << " seconds=" << elapsed
        << " ops=" << double(iterations * threads) / elapsed << " shared_line_pairs=" << sharedLines
        << " stable_addresses=" << stableAddresses << " errors=" << errors << std::endl;
    return elapsed > 0 && errors == 0 && !cancelled.load() &&
        LockFreeHashMapTestThread::failures() == previousFailures;
}

/** 拆分共享值导出的取得与销毁成本，校验始终在两段计时之外
@param [in] keys 已准备的键，至少4096个
@param [in] candidates 双份候选值，与正式负载相同，取偶数下标
@param [in] family 诊断输出中的类型名
@param [in] sortedValidation 是否在销毁前增加复制载荷的完整排序校验，用于观察校验干扰
@return 所有结果均符合独立预期且计时有效为true
*/
template <typename K>
static bool ExportCostBatch(const std::vector<K>& keys, const std::vector<std::shared_ptr<const uint64_t>>& candidates,
    const char* family, bool sortedValidation)
{
    typedef std::shared_ptr<const uint64_t> Value;
    const size_t count = 4096, repeats = 512;
    LockFreeHashMap<K, Value> map(4096);
    for (size_t key = 0; key < count; ++key)
    {
        if (!map.set(keys[key], candidates[key * 2]))
        {
            return false;
        }
    }
    double creation = 0, destruction = 0, validation = 0;
    uint64_t checksum = 0;
    for (size_t round = 0; round <= repeats; ++round)
    {
        bool valid = true;
        double begin = ReadClock(), created = 0, cleanup = 0;
        {
            std::vector<Value> actual = map.values();
            created = ReadClock();
            valid = actual.size() == count;
            bool seen[4096] = {};
            for (size_t index = 0; valid && index < actual.size(); ++index)
            {
                if (!actual[index])
                {
                    valid = false;
                    break;
                }
                uint64_t marker = *actual[index];
                if (marker == 0 || marker > count * 2 || (marker & 1) == 0)
                {
                    valid = false;
                    break;
                }
                size_t key = size_t((marker - 1) / 2);
                valid = !seen[key] && actual[index] == candidates[key * 2];
                seen[key] = true;
                checksum += marker;
            }
            if (valid && sortedValidation)
            {
                valid = LockFreeHashMapPerfValidation::values(actual, candidates, count);
            }
            cleanup = ReadClock();
        }
        double done = ReadClock();
        if (!valid || created <= begin || done <= cleanup)
        {
            return false;
        }
        // 第一次只预热，不能混入报告。每次都核对数量及唯一拥有值，不用校验和替代判定。
        if (round != 0)
        {
            creation += created - begin;
            validation += cleanup - created;
            destruction += done - cleanup;
        }
    }
    std::cout << "EXPORT_COST family=" << family << " sorted_validation=" << sortedValidation
        << " batches=" << repeats << " creation_us=" << creation * 1000000 / repeats
        << " destruction_us=" << destruction * 1000000 / repeats
        << " validation_us=" << validation * 1000000 / repeats << " checksum=" << checksum
        << " errors=0" << std::endl;
    return true;
}

/** 独立读取诊断入口；全部异常转为失败退出，线程先收拢再销毁夹具
@param [in] argc 无参数或一个string/exports参数
@param [in] argv string选择字符串缓冲区诊断，exports拆分导出取得与销毁，无参数保留整数诊断
@return 全部校验通过返回零
*/
int main(int argc, char** argv)
{
    try
    {
        if (argc == 2 && std::strcmp(argv[1], "exports") == 0)
        {
            std::vector<int> integers;
            std::vector<std::string> strings;
            std::vector<std::shared_ptr<const uint64_t>> candidates;
            integers.reserve(8192);
            strings.reserve(8192);
            candidates.reserve(8192);
            for (size_t index = 0; index < 8192; ++index)
            {
                integers.push_back(static_cast<int>(index));
                strings.push_back(std::string(64, 'k') + std::to_string(index));
                candidates.push_back(std::make_shared<uint64_t>(index + 1));
            }
            for (size_t round = 0; round < 4; ++round)
            {
                if (!ExportCostBatch(integers, candidates, "integer", (round & 1) != 0) ||
                    !ExportCostBatch(strings, candidates, "string", (round & 1) != 0))
                {
                    return 1;
                }
            }
            return 0;
        }
        if (argc == 2 && std::strcmp(argv[1], "string") == 0)
        {
            if (!LockFreeHashMapPerfModels::initializeWorkers())
            {
                return 1;
            }
            for (size_t round = 0; round < 12; ++round)
            {
                // 交替先后次序，保留所有轮次，不以最快一轮报告缓冲区效果。
                if (!StringReadBatch(round % 2 == 0) || !StringReadBatch(round % 2 != 0))
                {
                    return 1;
                }
            }
            return 0;
        }
        if (argc != 1)
        {
            return 2;
        }
        for (size_t round = 0; round < 3; ++round)
        {
            for (size_t threads = 1; threads <= 16; threads *= 4)
            {
                for (size_t stride = 1; stride <= 32; stride *= 32)
                {
                    if (!ReadBatch(threads, stride, false) || !ReadBatch(threads, stride, true))
                    {
                        return 1;
                    }
                }
            }
        }
        return 0;
    }
    catch (...)
    {
        std::cerr << "读取诊断发生异常" << std::endl;
        return 1;
    }
}