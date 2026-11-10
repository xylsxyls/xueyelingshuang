#include "LockFreeHashMapSingleThread.h"
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <limits>
#if defined(_WIN32)
#include <Windows.h>
#endif

LockFreeHashMapSingleSample::LockFreeHashMapSingleSample() :
m_seconds(0), m_calls(0), m_errors(0), m_checksum(0), m_bucketsBefore(0), m_bucketsAfter(0)
{

}

std::shared_ptr<const int32_t> LockFreeHashMapSingleData<std::shared_ptr<const int32_t>>::make(int32_t marker)
{
    return std::make_shared<const int32_t>(marker);
}

uint64_t LockFreeHashMapSingleData<std::shared_ptr<const int32_t>>::marker(const std::shared_ptr<const int32_t>& value)
{
    return value ? static_cast<uint64_t>(*value) : 0;
}

bool LockFreeHashMapSingleData<std::shared_ptr<const int32_t>>::released(const std::shared_ptr<const int32_t>& value)
{
    return value.use_count() == 1;
}

double LockFreeHashMapSingleThread::timestamp()
{
#if defined(_WIN32)
    LARGE_INTEGER tick = {}, frequency = {};
    if (!QueryPerformanceCounter(&tick) || !QueryPerformanceFrequency(&frequency))
    {
        throw std::runtime_error("单调时钟失败");
    }
    return double(tick.QuadPart) / double(frequency.QuadPart);
#else
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

const char* LockFreeHashMapSingleThread::operationName(int32_t operation)
{
    static const char* const kNames[] = {
        "get_hit", "get_miss", "contains_hit", "contains_miss", "count_key", "value_hit", "subscript_read",
        "set_update", "subscript_update", "insert_new", "insert_exists", "erase_hit", "erase_miss",
        "erase_insert_cycle", "read90_write10", "size", "empty_nonempty", "forEach", "keys", "values",
        "toVector", "clear_return", "clear_reclaim", "construct_fill_destroy"
    };
    return kNames[operation];
}

/** 严格解析十进制参数，不接受负数、溢出或尾部字符
@param [in] text 非空参数文本
@return 数值，非法参数抛出由入口捕获的异常
*/
static size_t ParseUnsigned(const char* text)
{
    if (!text || !*text || *text < '0' || *text > '9')
    {
        throw std::runtime_error("参数必须为十进制非负整数");
    }
    errno = 0;
    char* end = nullptr;
    unsigned long value = std::strtoul(text, &end, 10);
    if (errno || !end || *end)
    {
        throw std::runtime_error("参数溢出或包含非法字符");
    }
    return static_cast<size_t>(value);
}

int LockFreeHashMapSingleThread::run(int argc, char** argv)
{
    try
    {
        if (argc < 2 || argc > 8)
        {
            std::cerr << "用法：程序 输出前缀 [奇数轮数] [校准毫秒] [键数，0为全部] [首项目0..23] [末项目0..23] [逻辑CPU编号]" << std::endl;
            return 2;
        }
        size_t rounds = argc > 2 ? ParseUnsigned(argv[2]) : 9;
        size_t milliseconds = argc > 3 ? ParseUnsigned(argv[3]) : 25;
        size_t selectedCount = argc > 4 ? ParseUnsigned(argv[4]) : 0;
        size_t first = argc > 5 ? ParseUnsigned(argv[5]) : 0;
        size_t last = argc > 6 ? ParseUnsigned(argv[6]) : SingleOperationCount - 1;
        size_t cpu = argc > 7 ? ParseUnsigned(argv[7]) : 2;
        if (rounds < 3 || rounds > 31 || rounds % 2 == 0 || milliseconds < 1 || milliseconds > 250 ||
            (selectedCount && (selectedCount < 2 || selectedCount > 65536 || (selectedCount & (selectedCount - 1)))) ||
            first > last || last >= SingleOperationCount)
        {
            throw std::runtime_error("轮数、时长、二次幂键数或项目范围非法");
        }
#if defined(_WIN32)
        // 两角色在同一进程、同一主线程、同一逻辑CPU执行，避开用户指出有问题的4和5。
        DWORD_PTR allowed = 0, system = 0;
        if (cpu >= sizeof(DWORD_PTR) * 8 || cpu == 4 || cpu == 5 || !GetProcessAffinityMask(GetCurrentProcess(), &allowed, &system))
        {
            throw std::runtime_error("无法读取或使用指定CPU");
        }
        DWORD_PTR selected = DWORD_PTR(1) << cpu;
        if (!(allowed & selected) || !SetProcessAffinityMask(GetCurrentProcess(), selected) ||
            !SetThreadAffinityMask(GetCurrentThread(), selected))
        {
            throw std::runtime_error("无法绑定单线程CPU");
        }
        std::cout << "ENV cpu=" << cpu << " process_priority=" << GetPriorityClass(GetCurrentProcess())
            << " thread_priority=" << GetThreadPriority(GetCurrentThread()) << " pointer_bits=" << sizeof(void*) * 8 << std::endl;
#else
        std::cout << "ENV affinity=unbound pointer_bits=" << sizeof(void*) * 8 << std::endl;
        (void)cpu;
#endif
        const std::string prefix = argv[1];
        // 不覆盖已有原始证据。文件创建失败和磁盘写入失败均使测试失败。
        std::ifstream existingRaw((prefix + "-runs.csv").c_str());
        std::ifstream existingSummary((prefix + "-summary.csv").c_str());
        if (existingRaw.good() || existingSummary.good())
        {
            throw std::runtime_error("输出前缀已存在");
        }
        std::ofstream raw((prefix + "-runs.csv").c_str());
        std::ofstream summary((prefix + "-summary.csv").c_str());
        if (!raw || !summary)
        {
            throw std::runtime_error("无法创建证据文件");
        }
        raw << std::setprecision(15);
        summary << std::setprecision(15);
        raw << "type,keys,id,operation,round,order,role,loops,calls,seconds,ns_per_op,checksum,errors,buckets_before,buckets_after\n";
        summary << "type,keys,id,operation,rounds,current_ns,std_ns,throughput_change_pct,current_ops_s,std_ops_s,current_min_ns,current_max_ns,std_min_ns,std_max_ns,paired_median_change_pct\n";
        double begin = timestamp();
        size_t counts[] = { 256, 4096, 65536 };
        for (size_t i = 0; i < (selectedCount ? 1U : 3U); ++i)
        {
            size_t count = selectedCount ? selectedCount : counts[i];
            if (!suite<int32_t>("int32", count, rounds, double(milliseconds), static_cast<int32_t>(first), static_cast<int32_t>(last), raw, summary) ||
                !suite<std::shared_ptr<const int32_t>>("shared_ptr", count, rounds, double(milliseconds), static_cast<int32_t>(first), static_cast<int32_t>(last), raw, summary))
            {
                return 1;
            }
        }
        raw.flush();
        summary.flush();
        if (!raw.good() || !summary.good())
        {
            throw std::runtime_error("证据写入失败");
        }
        std::cout << "SINGLE_RESULT passed=1 rounds=" << rounds << " elapsed_s=" << timestamp() - begin << std::endl;
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "SINGLE_RESULT passed=0 error=" << error.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "SINGLE_RESULT passed=0 unknown_exception=1" << std::endl;
        return 1;
    }
}

/** 独立单线程对照入口，不创建测试工作线程
@param [in] argc 参数数量
@param [in] argv 命令行参数
@return 验证成功为零，失败为非零
*/
int main(int argc, char** argv)
{
    return LockFreeHashMapSingleThread::run(argc, argv);
}