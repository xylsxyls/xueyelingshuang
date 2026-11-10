#include "LockFreeHashMapPerfModels.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <limits>
#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#endif

double LockFreeHashMapPerfModels::s_frequency = 1;
std::vector<uintptr_t> LockFreeHashMapPerfModels::s_workerMasks;

LockFreeHashMapPerfLargeValue::LockFreeHashMapPerfLargeValue(uint64_t marker)
{
    for (size_t i = 0; i < 64; ++i)
    {
        m_words[i] = marker + i;
    }
}

bool LockFreeHashMapPerfLargeValue::operator==(const LockFreeHashMapPerfLargeValue& other) const
{
    return std::memcmp(m_words, other.m_words, sizeof(m_words)) == 0;
}

bool LockFreeHashMapPerfModels::initializeClock()
{
#if defined(_WIN32)
    LARGE_INTEGER value = {};
    if (!QueryPerformanceFrequency(&value) || value.QuadPart <= 0)
    {
        return false;
    }
    s_frequency = double(value.QuadPart);
#endif
    return true;
}

bool LockFreeHashMapPerfModels::initializeWorkers()
{
#if defined(_WIN32)
    try
    {
        DWORD_PTR allowed = 0, system = 0;
        DWORD bytes = 0;
        if (!GetProcessAffinityMask(GetCurrentProcess(), &allowed, &system) || allowed == 0 ||
            GetLogicalProcessorInformation(nullptr, &bytes) || GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
            bytes == 0 || bytes % sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) != 0)
        {
            return false;
        }
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> topology(bytes / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (!GetLogicalProcessorInformation(topology.data(), &bytes) || bytes > topology.size() * sizeof(topology[0]) ||
            bytes % sizeof(topology[0]) != 0)
        {
            return false;
        }
        s_workerMasks.clear();
        DWORD_PTR remaining = allowed;
        for (size_t i = 0; i < bytes / sizeof(topology[0]); ++i)
        {
            DWORD_PTR core = topology[i].ProcessorMask & allowed;
            if (topology[i].Relationship == RelationProcessorCore && core != 0)
            {
                DWORD_PTR first = core & (~core + 1);
                if ((remaining & first) != 0)
                {
                    s_workerMasks.push_back(first);
                    remaining &= ~first;
                }
            }
        }
        if (s_workerMasks.empty())
        {
            return false;
        }
        while (remaining != 0)
        {
            DWORD_PTR first = remaining & (~remaining + 1);
            s_workerMasks.push_back(first);
            remaining &= ~first;
        }
        std::cout << "PERF_BINDING physical_first=1 logical_masks=";
        for (size_t i = 0; i < s_workerMasks.size(); ++i)
        {
            std::cout << (i == 0 ? "" : ",") << s_workerMasks[i];
        }
        std::cout << std::endl;
    }
    catch (...)
    {
        return false;
    }
#else
    std::cout << "PERF_BINDING physical_first=0 logical_masks=automatic" << std::endl;
#endif
    return true;
}

void LockFreeHashMapPerfModels::bindWorker(size_t index)
{
#if defined(_WIN32)
    if (s_workerMasks.empty() || SetThreadAffinityMask(GetCurrentThread(),
        static_cast<DWORD_PTR>(s_workerMasks[index % s_workerMasks.size()])) == 0)
    {
        throw std::runtime_error("performance worker affinity failed");
    }
#else
    (void)index;
#endif
}

double LockFreeHashMapPerfModels::timestamp()
{
#if defined(_WIN32)
    LARGE_INTEGER value = {};
    if (!QueryPerformanceCounter(&value))
    {
        throw std::runtime_error("QPC unavailable");
    }
    return double(value.QuadPart) / s_frequency;
#else
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

void LockFreeHashMapPerfModels::add(std::vector<LockFreeHashMapPerfSpec>& result,
    const std::string& name, int32_t family, size_t keys, int32_t threads, int32_t readPercent,
    LockFreeHashMapPerfOperation operation, size_t buckets, bool collision, bool sharedKeys, size_t history)
{
    LockFreeHashMapPerfSpec spec = {};
    spec.m_id = static_cast<int32_t>(result.size() + 1);
    spec.m_name = name;
    spec.m_family = family;
    spec.m_keys = keys;
    spec.m_buckets = buckets;
    spec.m_threads = threads;
    spec.m_readPercent = readPercent;
    spec.m_operation = operation;
    spec.m_collision = collision;
    spec.m_sharedKeys = sharedKeys;
    spec.m_history = history;
    spec.m_iterations = readPercent == 100 ? 1048576 : 131072;
    spec.m_iterationMultiplier = 1;
    if (collision || buckets == 1 || history != 0)
    {
        // 加长短批次，避免线程释放和一次时间片抖动主导全碰撞指标。
        spec.m_iterations = 131072;
    }
    if (operation == LockFreeHashMapPerfInsert || operation == LockFreeHashMapPerfErase)
    {
        spec.m_iterations = keys;
    }
    if (operation >= LockFreeHashMapPerfSize && operation <= LockFreeHashMapPerfPairs)
    {
        spec.m_iterations = operation == LockFreeHashMapPerfSize ? 16384 : 128;
    }
    if (operation == LockFreeHashMapPerfClear)
    {
        spec.m_iterations = 64;
    }
    if (operation == LockFreeHashMapPerfRefill)
    {
        spec.m_iterations = 1;
    }
    if (currentOnly(spec))
    {
        spec.m_iterations = 32768;
    }
    if (threads <= 0 || keys == 0 || (!sharedKeys && keys % threads != 0) || spec.m_iterations % threads != 0)
    {
        throw std::runtime_error("invalid performance catalogue");
    }
    result.push_back(spec);
}

std::vector<LockFreeHashMapPerfSpec> LockFreeHashMapPerfModels::catalogue()
{
    std::vector<LockFreeHashMapPerfSpec> result;
    const char* families[] = { "int", "shared", "string_key", "wide64", "string_value", "value512" };
    const int32_t threadCounts[] = { 1, 2, 4, 8, 16, 32 };
    const int32_t reads[] = { 100, 95, 50 };
    for (int32_t family = 0; family < 3; ++family)
    {
        for (size_t t = 0; t < 6; ++t)
        {
            for (size_t r = 0; r < 3; ++r)
            {
                add(result, std::string(families[family]) + "_r" + std::to_string(reads[r]) + "_t" + std::to_string(threadCounts[t]),
                    family, 256, threadCounts[t], reads[r]);
            }
        }
        add(result, std::string(families[family]) + "_write_t4", family, 256, 4, 0);
        add(result, std::string(families[family]) + "_r10_t4", family, 256, 4, 10);
        add(result, std::string(families[family]) + "_miss_t4", family, 256, 4, 100, LockFreeHashMapPerfMiss);
        const size_t sizes[] = { 64, 4096, 65536 };
        for (size_t k = 0; k < 3; ++k)
        {
            add(result, std::string(families[family]) + "_read_k" + std::to_string(sizes[k]), family, sizes[k], 4, 100);
            add(result, std::string(families[family]) + "_r95_k" + std::to_string(sizes[k]), family, sizes[k], 4, 95);
        }
        add(result, std::string(families[family]) + "_hash0_read", family, 64, 4, 100, LockFreeHashMapPerfMixed, 1024, true);
        add(result, std::string(families[family]) + "_hash0_r95", family, 64, 4, 95, LockFreeHashMapPerfMixed, 1024, true);
        add(result, std::string(families[family]) + "_hash0_miss", family, 64, 4, 100, LockFreeHashMapPerfMiss, 1024, true);
        add(result, std::string(families[family]) + "_bucket1_r95", family, 64, 4, 95, LockFreeHashMapPerfMixed, 1);
        const LockFreeHashMapPerfOperation operations[] = { LockFreeHashMapPerfContains, LockFreeHashMapPerfContainsMiss,
            LockFreeHashMapPerfInsert, LockFreeHashMapPerfInsertExists, LockFreeHashMapPerfErase, LockFreeHashMapPerfChurn,
            LockFreeHashMapPerfSize, LockFreeHashMapPerfVisit, LockFreeHashMapPerfKeys, LockFreeHashMapPerfValues,
            LockFreeHashMapPerfPairs, LockFreeHashMapPerfClear, LockFreeHashMapPerfRefill };
        const char* names[] = { "contains", "contains_miss", "insert_new", "insert_exists", "erase", "churn",
            "size", "visit", "keys", "values", "pairs", "clear", "clear_refill" };
        for (size_t i = 0; i < sizeof(operations) / sizeof(operations[0]); ++i)
        {
            add(result, std::string(families[family]) + "_" + names[i], family, 4096, 1, 95, operations[i]);
        }
    }
    for (int32_t family = 0; family < 2; ++family)
    {
        for (int32_t threads = 1; threads <= 16; threads *= 4)
        {
            add(result, std::string(families[family]) + "_hot8_r95_t" + std::to_string(threads),
                family, 8, threads, 95, LockFreeHashMapPerfMixed, 1024, false, true);
        }
        add(result, std::string(families[family]) + "_history10000", family, 64, 1, 100,
            LockFreeHashMapPerfMixed, 1, false, false, 10000);
        const LockFreeHashMapPerfOperation operations[] = { LockFreeHashMapPerfFind, LockFreeHashMapPerfConditionalExists,
            LockFreeHashMapPerfCompare, LockFreeHashMapPerfConditionalErase };
        const char* names[] = { "new_find", "new_insert_exists", "new_find_compare", "new_find_erase_insert" };
        for (size_t i = 0; i < 4; ++i)
        {
            add(result, std::string(families[family]) + "_" + names[i], family, 256, 4, 95, operations[i]);
        }
    }
    for (int32_t family = 3; family < 6; ++family)
    {
        for (size_t r = 0; r < 3; ++r)
        {
            add(result, std::string(families[family]) + "_r" + std::to_string(reads[r]), family, 256, 4, reads[r]);
        }
        add(result, std::string(families[family]) + "_miss", family, 256, 4, 100, LockFreeHashMapPerfMiss);
    }
    add(result, "int_read_load512", 0, 32768, 4, 100, LockFreeHashMapPerfMixed, 64);
    add(result, "int_read_load1", 0, 32768, 4, 100, LockFreeHashMapPerfMixed, 32768);
    add(result, "int_read_k255", 0, 255, 1, 100);
    add(result, "int_read_k257", 0, 257, 1, 100);
    return result;
}

std::vector<LockFreeHashMapPerfSpec> LockFreeHashMapPerfModels::memoryCatalogue()
{
    std::vector<LockFreeHashMapPerfSpec> result;
    for (int32_t family = 0; family < 6; ++family)
    {
        for (size_t keys = 64; keys <= 4096; keys *= 64)
        {
            for (int32_t churn = 0; churn < 2; ++churn)
            {
                add(result, "memory_f" + std::to_string(family) + "_k" + std::to_string(keys) + (churn ? "_churn" : "_update"),
                    family, keys, 1, 0, churn ? LockFreeHashMapPerfChurn : LockFreeHashMapPerfMixed);
                result.back().m_iterations = 10000;
            }
        }
    }
    return result;
}

bool LockFreeHashMapPerfModels::scaleIterations(LockFreeHashMapPerfSpec& spec, size_t multiplier)
{
    if (multiplier == 0 || multiplier > 256 || spec.m_iterationMultiplier != 1 ||
        (multiplier != 1 && spec.m_operation != LockFreeHashMapPerfMixed) ||
        spec.m_iterations > (std::numeric_limits<size_t>::max)() / multiplier)
    {
        return false;
    }
    spec.m_iterations *= multiplier;
    spec.m_iterationMultiplier = multiplier;
    return true;
}

uint64_t LockFreeHashMapPerfModels::callsPerIteration(const LockFreeHashMapPerfSpec& spec)
{
    if (spec.m_operation == LockFreeHashMapPerfChurn || spec.m_operation == LockFreeHashMapPerfConditionalErase)
    {
        return 3;
    }
    if (spec.m_operation == LockFreeHashMapPerfCompare)
    {
        return 2;
    }
    return spec.m_operation == LockFreeHashMapPerfRefill ? spec.m_keys + 1 : 1;
}

bool LockFreeHashMapPerfModels::currentOnly(const LockFreeHashMapPerfSpec& spec)
{
    return spec.m_operation >= LockFreeHashMapPerfFind;
}

double LockFreeHashMapPerfModels::percentile(std::vector<double>& samples, size_t percentile)
{
    if (samples.empty() || percentile == 0 || percentile > 100)
    {
        return 0;
    }
    std::sort(samples.begin(), samples.end());
    size_t rank = (samples.size() * percentile + 99) / 100;
    return samples[rank - 1];
}