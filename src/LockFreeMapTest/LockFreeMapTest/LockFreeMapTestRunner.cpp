#include "LockFreeMapTestRunner.h"
#include "LockFreeMapTest.h"
#include "LockFreeMapStatusTest.h"
#include "LockFreeMapRetryTests.h"
#include "LockFreeMapSearchTest.h"
#ifndef LOCK_FREE_MAP_TEST_SINGLE
#include "LockFreeMapTests.h"
#include "LockFreeMapArrivalTest.h"
#endif
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <exception>
#include <initializer_list>
#include <map>
#include <set>
#include <string>
#include <vector>

#ifdef _MSC_VER
// VS2013 也将报告里的中文固定文本写为 UTF-8
#pragma execution_character_set("utf-8")
#endif

FILE* LockFreeMapTestReport::m_file = nullptr;

bool LockFreeMapTestRunner::number(const char*& text, unsigned& value)
{
    value = 0;
    if (*text < '0' || *text > '9')
    {
        return false;
    }
    do
    {
        if (value > 100000)
        {
            return false;
        }
        value = value * 10 + static_cast<unsigned>(*text++ - '0');
    } while (*text >= '0' && *text <= '9');
    return value != 0;
}

bool LockFreeMapTestRunner::selection(const char* text, std::set<unsigned>& ids)
{
    while (*text != 0)
    {
        unsigned first = 0, last = 0;
        if (!number(text, first))
        {
            return false;
        }
        last = first;
        if (*text == '-')
        {
            ++text;
            if (!number(text, last) || last < first)
            {
                return false;
            }
        }
        if (last > LOCK_FREE_MAP_TEST_CONFIG)
        {
            return false;
        }
        for (unsigned id = first; id <= last; ++id)
        {
            ids.insert(id);
        }
        if (*text == 0)
        {
            break;
        }
        if (*text++ != ',' || *text == 0)
        {
            return false;
        }
    }
    return !ids.empty();
}

int LockFreeMapTestRunner::run(int argc, char** argv)
{
    // 设备测试宿主可直接传入参数，必须在首次读取参数或创建报告之前校验。
    if (argc < 1 || argv == nullptr)
    {
        return 2;
    }
    for (int index = 0; index < argc; ++index)
    {
        if (argv[index] == nullptr)
        {
            return 2;
        }
    }
    typedef std::map<unsigned, LockFreeMapTestEntry> CaseMap;
    const std::initializer_list<CaseMap::value_type> entries = {
        /** 把有序参考检查的错误数转换为注册表布尔结果
        @return 错误数为零时true
        */
        { LOCK_FREE_MAP_TEST_BASIC, { "有序参考随机对照", false, []() { return LockFreeMapBasic() == 0; } } },
#ifndef LOCK_FREE_MAP_TEST_SINGLE
        { LOCK_FREE_MAP_TEST_TOKENS, { "版本凭据和地址复用", false, &LockFreeMapTests::tokens } },
        { LOCK_FREE_MAP_TEST_EXCEPTIONS, { "比较和复制异常", false, &LockFreeMapTests::exceptions } },
        { LOCK_FREE_MAP_TEST_SAME_KEY, { "同键插入唯一胜者", false, &LockFreeMapTests::sameKey } },
        { LOCK_FREE_MAP_TEST_PAUSED_READER, { "暂停读者与满容量", false, &LockFreeMapTests::pausedReader } },
        { LOCK_FREE_MAP_TEST_MAXIMUM, { "最高优先级竞争", false, &LockFreeMapTests::maximumRace } },
        { LOCK_FREE_MAP_TEST_CLEAR, { "清空和写入竞争", false, &LockFreeMapTests::clearRace } },
        { LOCK_FREE_MAP_TEST_FULL, { "双池满容量释放", false, &LockFreeMapTests::fullPools } },
        { LOCK_FREE_MAP_TEST_SHRINK, { "动态缩容与再增长", false, &LockFreeMapTests::shrinking } },
        { LOCK_FREE_MAP_TEST_STRESS, { "混合并发压力", true, &LockFreeMapTests::stress } },
        { LOCK_FREE_MAP_TEST_OWNERSHIP, { "借用所有权与顺序", false, &LockFreeMapTests::ownership } },
        { LOCK_FREE_MAP_TEST_ALLOCATIONS, { "逐申请点故障注入", false, &LockFreeMapTests::allocations } },
        { LOCK_FREE_MAP_TEST_LIMITS, { "参数重入版本上限", false, &LockFreeMapTests::limits } },
        { LOCK_FREE_MAP_TEST_POOL_CONCURRENCY, { "并发池额度", true, &LockFreeMapTests::poolConcurrency } },
        { LOCK_FREE_MAP_TEST_PAUSED_RECLAIMER, { "暂停回收时推进", false, &LockFreeMapTests::pausedReclaimer } },
        { LOCK_FREE_MAP_TEST_HISTORY_BOUND, { "历史版本有界", true, &LockFreeMapTests::historyBound } },
        /** 把池模块检查的错误数转换为注册表布尔结果
        @return 错误数为零时true
        */
        { LOCK_FREE_MAP_TEST_POOL, { "池模块独立检查", false, []() { return LockFreeMapPoolModule() == 0; } } },
        /** 把公开接口五十二项检查的错误数转换为注册表布尔结果
        @return 错误数为零时true
        */
        { LOCK_FREE_MAP_TEST_HISTORICAL, { "公开接口五十二项检查", true, []() { return LockFreeMapHistorical() == 0; } } },
        { LOCK_FREE_MAP_TEST_PAUSED_GROWTH, { "暂停申请与预留额度", false, &LockFreeMapTests::pausedGrowth } },
        { LOCK_FREE_MAP_TEST_RETIRED_VERSION, { "旧版本退役竞争", false, &LockFreeMapTests::retiredVersionRace } },
        { LOCK_FREE_MAP_TEST_HELPING, { "描述符帮助和清空", false, &LockFreeMapTests::descriptorHelping } },
        { LOCK_FREE_MAP_TEST_FAST_UPDATES, { "无申请更新和条件累加", true, &LockFreeMapTests::fastUpdates } },
        /** 把四库共存检查的错误数转换为注册表布尔结果
        @return 错误数为零时true
        */
        { LOCK_FREE_MAP_TEST_HEADERS, { "四库公开头共存", false, []() { return LockFreeMapHeaders() == 0; } } },
        { LOCK_FREE_MAP_TEST_DIRECT_KEY, { "直接构造键及回滚", false, &LockFreeMapTests::directKeyConstruction } },
        { LOCK_FREE_MAP_TEST_SMALL_BUDGET, { "小预算回收与暂停读者", false, &LockFreeMapTests::smallBudget } },
        { LOCK_FREE_MAP_TEST_SHARED_INDEX, { "索引引用转交与暂停发布者", false, &LockFreeMapTests::sharedIndex } },
        { LOCK_FREE_MAP_TEST_SLOT_CACHE, { "空槽转交与释放后重建", false, &LockFreeMapTests::slotCache } },
        { LOCK_FREE_MAP_TEST_DETACHED_DESTRUCTION, { "独占析构暂停与槽复用", false, &LockFreeMapTests::detachedDestruction } },
        /** 把竞争协议检查的错误数转换为注册表布尔结果
        @return 错误数为零时true
        */
        { LOCK_FREE_MAP_TEST_CONTENTION, { "竞争让步及暂停请求者", false, []() { return LockFreeMapContentionProtocol() == 0; } } },
        /** 把逐线程进展检查的错误数转换为注册表布尔结果
        @return 错误数为零时true
        */
        { LOCK_FREE_MAP_TEST_PROGRESS, { "生产消费逐线程进展", true, []() { return LockFreeMapConcurrentProgress() == 0; } } },
        /** 把查询交错检查的错误数转换为注册表布尔结果
        @return 错误数为零时true
        */
        { LOCK_FREE_MAP_TEST_QUERY_PROGRESS, { "精确查询与结构修改交错", false, []() { return LockFreeMapQueryProgress() == 0; } } },
        { LOCK_FREE_MAP_TEST_LATE_ARRIVAL, { "后来线程逐窗口推进", true, &LockFreeMapArrivalTest::run } },
#endif
        { LOCK_FREE_MAP_TEST_RETRY_OPTIONS, { "容量与竞争独立重试", false, &LockFreeMapRetryTest } },
        { LOCK_FREE_MAP_TEST_GROWTH_NOTIFICATION, { "扩容通知合同", false, &LockFreeMapGrowthTest } },
        { LOCK_FREE_MAP_TEST_CONFIG, { "初始化参数设置", false, &LockFreeMapConfigurationTest } },
        { LOCK_FREE_MAP_TEST_SEARCH, { "查询边界和自定义顺序", false, &LockFreeMapSearchTest } },
        /** 把统一接口样例的错误数转换为注册表布尔结果
        @return 错误数为零时true
        */
        { LOCK_FREE_MAP_TEST_EXAMPLES, { "六端统一接口样例", false, []() { return LockFreeMapCompileExamples() == 0; } } },
        /** 执行状态合同检查并记录其结果
        @return 检查返回零且报告写入成功时true
        */
        { LOCK_FREE_MAP_TEST_STATUS_CONTRACT, { "普通接口重试与资源错误", false, []()
        {
            const int result = LockFreeMapStatusContract();
            return std::fprintf(LockFreeMapTestReport::m_file, "status_contract,result,%d\n", result) > 0 && result == 0;
        } } },
        /** 把平台能力检查的错误数转换为注册表布尔结果
        @return 错误数为零时true
        */
        { LOCK_FREE_MAP_TEST_PLATFORM, { "原子对齐及低内存", false, []() { return LockFreeMapPlatform() == 0; } } },
        { LOCK_FREE_MAP_TEST_ARGUMENTS, { "参数与报告错误退出码", false, &LockFreeMapTestRunner::arguments } }
    };
    CaseMap cases(entries.begin(), entries.end());
    // map 会合并重复键；先核对原始条数，再检查本目标应有的编号。
    if (cases.size() != entries.size())
    {
        return 1;
    }
#ifdef LOCK_FREE_MAP_TEST_SINGLE
    const unsigned required[] = { LOCK_FREE_MAP_TEST_BASIC, LOCK_FREE_MAP_TEST_EXAMPLES, LOCK_FREE_MAP_TEST_PLATFORM,
        LOCK_FREE_MAP_TEST_ARGUMENTS, LOCK_FREE_MAP_TEST_STATUS_CONTRACT, LOCK_FREE_MAP_TEST_RETRY_OPTIONS,
        LOCK_FREE_MAP_TEST_GROWTH_NOTIFICATION, LOCK_FREE_MAP_TEST_SEARCH, LOCK_FREE_MAP_TEST_CONFIG };
    if (cases.size() != sizeof(required) / sizeof(required[0]))
    {
        return 1;
    }
    for (size_t index = 0; index < cases.size(); ++index)
    {
        if (cases.find(required[index]) == cases.end())
        {
            return 1;
        }
    }
#else
    if (cases.size() != LOCK_FREE_MAP_TEST_CONFIG)
    {
        return 1;
    }
#endif
    for (auto item = cases.begin(); item != cases.end(); ++item)
    {
        if (item->first < 1 || item->first > LOCK_FREE_MAP_TEST_CONFIG || item->second.m_run == nullptr ||
            item->second.m_name == nullptr || item->second.m_name[0] == 0)
        {
            return 1;
        }
    }
#ifndef LOCK_FREE_MAP_TEST_SINGLE
    // 独立进程自查使用真实线程异常进入累计计数；第一项失败不应污染下一项。
    const bool accounting = argc > 1 && std::strcmp(argv[1], "--check-accounting") == 0;
    if (accounting)
    {
        cases.clear();
        /** 启动受控异常线程以检查框架失败计数
        @return 线程回收成功时true，线程异常由框架独立计为失败
        */
        cases.insert(std::make_pair(1U, LockFreeMapTestEntry{ "预期线程异常", false, []() -> bool
        {
            /** 故意抛出线程异常，验证入口捕获并累计失败
            */
            LockFreeMapTestThread worker([]() { throw std::runtime_error("controlled worker failure"); });
            return worker.join();
        } }));
        /** 给故障自检提供随后正常用例，确认失败状态被正确隔离
        @return 始终返回true
        */
        cases.insert(std::make_pair(2U, LockFreeMapTestEntry{ "随后正常用例", false, []() -> bool { return true; } }));
    }
#else
    const bool accounting = false;
#endif
    std::set<unsigned> selected;
    const char* mode = "--all";
    const char* reportPath = "LockFreeMapTest-result.csv";
    unsigned repeats = 1;
    bool modeSet = false;
    for (int index = 1; index < argc; ++index)
    {
        const char* arg = argv[index];
        if (index == 1 && accounting)
        {
            continue;
        }
        if (std::strcmp(arg, "--report") == 0 && index + 1 < argc)
        {
            reportPath = argv[++index];
        }
        else if (std::strcmp(arg, "--repeat") == 0 && index + 1 < argc)
        {
            const char* value = argv[++index];
            if (!number(value, repeats) || *value != 0 || repeats > 50)
            {
                return 2;
            }
        }
        else if (!modeSet && (std::strcmp(arg, "--all") == 0 || std::strcmp(arg, "--instant") == 0 ||
            std::strcmp(arg, "--stress") == 0 || std::strcmp(arg, "--ids") == 0))
        {
            mode = arg;
            modeSet = true;
            if (std::strcmp(mode, "--ids") == 0 && (index + 1 == argc || !selection(argv[++index], selected)))
            {
                return 2;
            }
        }
        else
        {
            return 2;
        }
    }
    if (std::strcmp(mode, "--ids") != 0)
    {
        for (auto entry = cases.begin(); entry != cases.end(); ++entry)
        {
            if (std::strcmp(mode, "--all") == 0 ||
                (entry->second.m_stress == (std::strcmp(mode, "--stress") == 0)))
            {
                selected.insert(entry->first);
            }
        }
    }
    if (selected.empty())
    {
        return 2;
    }
    for (auto id = selected.begin(); id != selected.end(); ++id)
    {
        if (cases.find(*id) == cases.end())
        {
            return 2;
        }
    }
    // 预先排好执行顺序，打开报告及启用资源跟踪后不再为此分配内存。
    std::vector<unsigned> order(selected.begin(), selected.end());
    if (std::strcmp(mode, "--all") == 0)
    {
        /** 让瞬时用例排在压力用例之前，同组保留编号顺序
        @param [in] id 已验证存在的测试编号
        @return 瞬时用例返回true
        */
        std::stable_partition(order.begin(), order.end(), [&cases](unsigned id)
        {
            return !cases.find(id)->second.m_stress;
        });
    }

    // 不把空文件名交给 MSVC 的无效参数处理器
    if (*reportPath == 0)
    {
        return 3;
    }
    FILE* report = std::fopen(reportPath, "wb");
    if (report == nullptr)
    {
        return 3;
    }
    LockFreeMapTestReport::m_file = report;
    // 打开报告后，用例之外不再执行需要申请内存的容器操作
    bool written = std::fprintf(report, "repeat,id,name,result\n") > 0;
    unsigned failures = 0;
#ifndef LOCK_FREE_MAP_TEST_SINGLE
    int64_t warmOutstanding = 0;
    size_t warmHandles = 0;
    LockFreeMapTestFaults::m_failAfter.store(-1);
    const int previousFaultMode = LockFreeMapTestFaults::m_enabled.exchange(1);
#endif
    for (unsigned repeat = 0; repeat < repeats; ++repeat)
    {
        for (auto id = order.begin(); id != order.end(); ++id)
        {
            const LockFreeMapTestEntry& entry = cases.find(*id)->second;
            bool passed = false;
#ifndef LOCK_FREE_MAP_TEST_SINGLE
            const size_t errorsBefore = LockFreeMapTestThread::m_errors.load();
            const size_t timeoutsBefore = LockFreeMapTestValue::m_timeouts.load();
#endif
            try
            {
                passed = entry.m_run();
            }
            catch (const std::exception& error)
            {
                std::fprintf(LockFreeMapTestReport::m_file, "case=%u exception=%s\n", *id, error.what());
            }
            catch (...)
            {
                std::fprintf(LockFreeMapTestReport::m_file, "case=%u unexpected exception\n", *id);
            }
#ifndef LOCK_FREE_MAP_TEST_SINGLE
            LockFreeMapTestFaults::m_failAfter.store(-1);
            LockFreeMapTestFaults::m_pauseAllocation.store(-1);
            LockFreeMapTestValue::m_throwCopy.store(0);
            LockFreeMapTestValue::m_throwAssignment.store(0);
            LockFreeMapTestValue::m_pauseCopyValue.store(-1);
            LockFreeMapTestValue::m_pauseDestructorValue.store(-1);
            LockFreeMapTestLess::m_throw.store(0);
            LockFreeMapTestLess::m_throwConstructor.store(0);
            // 批次仍保留累计失败；单项只归属本项新增错误，避免一个故障污染后续结果。
            passed = passed && LockFreeMapTestValue::m_live.load() == 0 &&
                LockFreeMapTestThread::m_errors.load() == errorsBefore && LockFreeMapTestValue::m_timeouts.load() == timeoutsBefore;
#endif
            failures += passed ? 0 : 1;
            written = (std::fprintf(report, "%u,%u,%s,%s\n", repeat + 1, *id, entry.m_name, passed ? "PASS" : "FAIL") > 0) && written;
            // 每项落盘，异常终止时仍可确定最后完成的用例；不影响独立吞吐基准
            written = std::fflush(report) == 0 && written;
        }
#ifndef LOCK_FREE_MAP_TEST_SINGLE
        const int64_t outstanding = LockFreeMapTestFaults::m_outstanding.load();
        if (repeat == 0)
        {
            warmOutstanding = outstanding;
        }
        // 负数代表分配与释放没有成对进入跟踪器，首轮也不能当作合法基线
        const bool balanced = outstanding >= 0 && outstanding == warmOutstanding;
        failures += balanced ? 0 : 1;
        size_t threads = 0, handles = 0;
        bool observed = LockFreeMapTests::resources(threads, handles);
        if (repeat == 0)
        {
            warmHandles = handles;
        }
#ifdef _WIN32
        const uint64_t deadline = LockFreeMapClock::milliseconds() + 1000;
        while (observed && handles != warmHandles && LockFreeMapClock::milliseconds() < deadline)
        {
            Sleep(1);
            observed = LockFreeMapTests::resources(threads, handles);
        }
#endif
        failures += observed && handles == warmHandles ? 0 : 1;

#ifdef _WIN32
        written = (std::fprintf(report, "resource_counts,%u,%lld,%llu,%llu\n", repeat + 1, static_cast<long long>(outstanding), static_cast<unsigned long long>(handles), static_cast<unsigned long long>(threads)) > 0) && written;
#else
        written = (std::fprintf(report, "resource_counts,%u,%lld,NOT_MEASURED,NOT_MEASURED\n", repeat + 1, static_cast<long long>(outstanding)) > 0) && written;
#endif
        written = (std::fprintf(report, "%u,0,resources,%s\n", repeat + 1,
            observed && handles == warmHandles && balanced ? "PASS" : "FAIL") > 0) && written;
#endif
    }
#ifndef LOCK_FREE_MAP_TEST_SINGLE
    LockFreeMapTestFaults::m_enabled.store(previousFaultMode);
#endif
    written = (std::fprintf(report, "failures,%u\n", failures) > 0) && written;
    written = std::ferror(report) == 0 && written;
    written = std::fclose(report) == 0 && written;
    LockFreeMapTestReport::m_file = nullptr;

    return !written ? 3 : failures == 0 ? 0 : 1;
}

bool LockFreeMapTestRunner::arguments()
{
    const std::vector<std::vector<std::string>> invalid = {
        { "test", "--ids", "" }, { "test", "--ids", "0" }, { "test", "--ids", "1x" },
        { "test", "--ids", "3-2" }, { "test", "--ids", "1," }, { "test", "--ids", "1--2" },
        { "test", "--ids", "999" }, { "test", "--ids", "1-99999999999999999999" },
        { "test", "--ids", "-1" }, { "test", "--all", "--instant" },
        { "test", "--repeat", "0" }, { "test", "--repeat", "51" }, { "test", "--repeat", "1junk" },
        { "test", "--unknown" }, { "test", "--report" }, { "test", "--ids" }
    };
    for (size_t index = 0; index < invalid.size(); ++index)
    {
        std::vector<char*> parameters;
        for (size_t field = 0; field < invalid[index].size(); ++field)
        {
            // 被测入口只读参数，字符串的存储在整次调用内保持有效
            parameters.push_back(const_cast<char*>(invalid[index][field].c_str()));
        }
        if (run(static_cast<int>(parameters.size()), parameters.data()) != 2)
        {
            return false;
        }
    }
    char program[] = "test", ids[] = "--ids", first[] = "1", report[] = "--report", empty[] = "";
    char* missing[] = { program, nullptr };
    if (run(0, missing) != 2 || run(1, nullptr) != 2 || run(2, missing) != 2)
    {
        return false;
    }
    char* parameters[] = { program, ids, first, report, empty };
    if (run(5, parameters) != 3)
    {
        return false;
    }
    char directory[] = "/";
    parameters[4] = directory;
    return run(5, parameters) == 3;
}