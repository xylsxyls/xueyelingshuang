#include "LockFreeHashMapWorkloads.h"
#include <LockFreeHashMap.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#endif

double LockFreeHashMapWorkloads::s_frequency = 1;

double LockFreeHashMapWorkloads::timestamp()
{
#if defined(_WIN32)
    LARGE_INTEGER ticks = {};
    QueryPerformanceCounter(&ticks);
    return double(ticks.QuadPart) / s_frequency;
#else
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

size_t LockFreeHashMapWorkloadCollisionHash::operator()(int key) const
{
    (void)key;
    return 0;
}

/** 执行独立测试入口，参数错误和未预期异常均返回非零
@param [in] argc 参数数量
@param [in] argv 参数数组，具体选择见测试文档
@return 0 完成并通过，非零表示输入、校验或设施失败
*/
int main(int argc, char** argv)
{
#if defined(_WIN32)
    LARGE_INTEGER frequency = {};
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart == 0)
    {
        return 1;
    }
    LockFreeHashMapWorkloads::s_frequency = double(frequency.QuadPart);
#endif
    char* end = nullptr;
    long requested = argc == 1 ? 1 : std::strtol(argv[1], &end, 10);
    if (argc > 2 || requested < 1 || requested > 20 || (argc == 2 && (end == argv[1] || *end != 0)))
    {
        return 3;
    }
    int scale = static_cast<int>(requested);
    std::cout << "WORKLOAD_CONFIG scale=" << scale << std::endl;
    try
    {
        bool passed = LockFreeHashMapWorkloads::run("read", 1024, 1, 200000 * scale, 0, 0);
        passed = LockFreeHashMapWorkloads::run("cold_after_hot_updates", 1, 1, 20000 * scale, 0, 10000) && passed;
        passed = LockFreeHashMapWorkloads::run("mixed_95read", 1024, 4, 100000 * scale, 20, 0) && passed;
        passed = LockFreeHashMapWorkloads::run("mixed_50read", 1024, 4, 100000 * scale, 2, 0) && passed;
        passed = LockFreeHashMapWorkloads::run("collision_95read", 1, 4, 20000 * scale, 20, 0) && passed;
        passed = LockFreeHashMapWorkloads::run<LockFreeHashMapWorkloadCollisionHash>("zero_hash_read", 1, 1, 100000 * scale, 0, 0) && passed;
        passed = LockFreeHashMapWorkloads::run<LockFreeHashMapWorkloadCollisionHash>("zero_hash_mixed", 1, 4, 20000 * scale, 20, 0) && passed;
        passed = LockFreeHashMapWorkloads::runOwned<>("owned_95read", 1024, 4, 100000 * scale, 20, 0) && passed;
        passed = LockFreeHashMapWorkloads::runOwned<>("owned_50read", 1024, 4, 100000 * scale, 2, 0) && passed;
        passed = LockFreeHashMapWorkloads::runOwned<LockFreeHashMapWorkloadCollisionHash>("owned_zero_hash", 1, 4, 20000 * scale, 20, 0) && passed;
        return passed ? 0 : 1;
    }
    catch (...)
    {
        std::cerr << "WORKLOAD unexpected exception" << std::endl;
        return 2;
    }
}