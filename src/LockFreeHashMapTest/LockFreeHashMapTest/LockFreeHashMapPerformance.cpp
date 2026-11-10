#include "LockFreeHashMapPerformance.h"
#include <cstdlib>
#include <limits>
#include <cerrno>

std::string LockFreeHashMapPerfData<std::string>::make(size_t marker)
{
    return std::string(64, 'k') + std::to_string(marker);
}

uint64_t LockFreeHashMapPerfData<uint64_t>::make(size_t marker)
{
    return UINT64_C(0x100000000) + marker;
}

std::shared_ptr<const uint64_t> LockFreeHashMapPerfData<std::shared_ptr<const uint64_t>>::make(size_t marker)
{
    return std::make_shared<uint64_t>(marker);
}

bool LockFreeHashMapPerformance::dispatch(const LockFreeHashMapPerfSpec& spec, int32_t repeats)
{
    switch (spec.m_family)
    {
    case 0:
    {
        return select<int32_t, int32_t>(spec, repeats);
    }
    case 1:
    {
        return select<int32_t, std::shared_ptr<const uint64_t>>(spec, repeats);
    }
    case 2:
    {
        return select<std::string, std::shared_ptr<const uint64_t>>(spec, repeats);
    }
    case 3:
    {
        return select<uint64_t, uint64_t>(spec, repeats);
    }
    case 4:
    {
        return select<int32_t, std::string>(spec, repeats);
    }
    case 5:
    {
        return select<int32_t, LockFreeHashMapPerfLargeValue>(spec, repeats);
    }
    default:
    {
        return false;
    }
    }
}

/** 数字参数编码为 批次数*1000+配置编号；只启动一个隔离负载
@param [in] argc 参数数量，2 或 3；内存计量只允许 2
@param [in] argv 完整十进制编码，批次数 1 至 64；可追加 get/set 连续循环倍率 1 至 256
@return 0 校验成功，非零表示参数、数据或设施失败
*/
int main(int argc, char** argv)
{
    try
    {
        if ((argc != 2 && argc != 3) || !LockFreeHashMapPerfModels::initializeClock() || !LockFreeHashMapPerfModels::initializeWorkers())
        {
            return 2;
        }
        char* end = nullptr;
        errno = 0;
        long encoded = std::strtol(argv[1], &end, 10);
        if (errno != 0 || end == argv[1] || *end != 0 || encoded < 1001 || encoded > 64999)
        {
            return 2;
        }
#if defined(LOCKFREEHASHMAP_PERF_MEMORY)
        if (argc != 2)
        {
            return 2;
        }
        std::vector<LockFreeHashMapPerfSpec> specs = LockFreeHashMapPerfModels::memoryCatalogue();
        std::cout << "MEMORY_CONFIG tracking=requested_heap_bytes warm=" << size_t(0) << uint64_t(0) << 0.0 << std::endl;
#else
        std::vector<LockFreeHashMapPerfSpec> specs = LockFreeHashMapPerfModels::catalogue();
#endif
        size_t id = static_cast<size_t>(encoded % 1000);
        if (id == 0 || id > specs.size())
        {
            return 2;
        }
        if (argc == 3)
        {
            errno = 0;
            unsigned long multiplier = std::strtoul(argv[2], &end, 10);
            if (errno != 0 || end == argv[2] || *end != 0 ||
                !LockFreeHashMapPerfModels::scaleIterations(specs[id - 1], multiplier))
            {
                return 2;
            }
        }
#if !defined(LOCKFREEHASHMAP_PERF_CURRENT)
        if (LockFreeHashMapPerfModels::currentOnly(specs[id - 1]))
        {
            return 2;
        }
#endif
#if defined(LOCKFREEHASHMAP_PERF_STD_MUTEX)
        std::cout << "BACKEND std_unordered_map_mutex max_load_factor=1 automatic_rehash=1" << std::endl;
#else
        std::cout << "BACKEND LockFreeHashMap" << std::endl;
#endif
        return LockFreeHashMapPerformance::dispatch(specs[id - 1], static_cast<int32_t>(encoded / 1000)) ? 0 : 1;
    }
    catch (...)
    {
        std::cerr << "PERFORMANCE unexpected exception" << std::endl;
        return 3;
    }
}