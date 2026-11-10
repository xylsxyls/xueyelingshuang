#include "LockFreeMapTest.h"
#include "LockFreeMap/LockFreeMapAPI.h"

/** 检查真实目标原子宽度、释放后坐标复用及 Wasm 堆申请失败
@return 全部适用的公开合同通过时返回零
*/
int LockFreeMapPlatform()
{
    std::atomic<void*> pointer;
    std::atomic<size_t> size;
    std::atomic<uint64_t> wide;
    std::atomic<uint32_t> word;

    int failures = pointer.is_lock_free() && size.is_lock_free() && wide.is_lock_free() && word.is_lock_free() ? 0 : 1;
    std::fprintf(LockFreeMapTestReport::m_file, "atomics,pointer=%d,size=%d,uint64=%d,uint32=%d,pointer_bytes=%llu,value_alignment=%llu\n",
        pointer.is_lock_free() ? 1 : 0, size.is_lock_free() ? 1 : 0, wide.is_lock_free() ? 1 : 0, word.is_lock_free() ? 1 : 0,
        static_cast<unsigned long long>(sizeof(void*)), static_cast<unsigned long long>(std::alignment_of<long double>::value));
    try
    {
        // 使用当前平台基本对齐要求检查实际载荷地址
        LockFreeMap<int, long double> aligned;
        for (int key = 0; key < 128; ++key)
        {
            failures += !aligned.set(key, static_cast<long double>(key) + 0.25L);
        }
        failures += aligned.forEach([&failures](const int& key, const long double& value) -> void
        {
            failures += reinterpret_cast<uintptr_t>(&value) % std::alignment_of<long double>::value != 0;
            failures += value != static_cast<long double>(key) + 0.25L;
        }) != 128;
        aligned.clearAndFreeMemory();
        failures += aligned.retainedNodeCount() != 0 || aligned.retainedValueCount() != 0;
        LockFreeMap<int, int> map;
        for (int cycle = 0; cycle < 100; ++cycle)
        {
            // 改变外部静止释放后实际存在的目录和槽坐标
            const int count = cycle % 37 + 1;
            for (int key = 0; key < count; ++key)
            {
                failures += !map.set(key, cycle);
                failures += !map.set(key, cycle + 1);
            }
            int key = -1, value = -1;
            LockFreeMapToken token;
            failures += map.lockFreeMapLast(key, value, token) != LOCK_FREE_MAP_OK;
            failures += key != count - 1 || value != cycle + 1;
            map.clearAndFreeMemory();
            failures += !map.empty();
            failures += !map.set(count - 1, cycle + 2);
            failures += map.lockFreeMapUpdate(count - 1, token, 0) != LOCK_FREE_MAP_CONFLICT;
            map.clearAndFreeMemory();
        }
#ifdef LOCK_FREE_MAP_TEST_FIXED_HEAP
        // 本程序使用真实固定 16 MiB Wasm 堆，不模拟分配器
        // 请求的批次无法容纳，必须返回申请失败
        LockFreeMapConfig config;
        config.values.initialCapacity = 10000000;
        config.values.maximumCapacity = 10000000;
        const LockFreeMapStatus status = map.lockFreeMapConfigure(config);
        failures += status != LOCK_FREE_MAP_ALLOCATION_FAILURE;
        failures += !map.set(7, 9);
        int value = 0;
        failures += !map.get(7, value) || value != 9;

#endif
    }
    catch (...)
    {
        ++failures;
    }

    return failures == 0 ? 0 : 1;
}