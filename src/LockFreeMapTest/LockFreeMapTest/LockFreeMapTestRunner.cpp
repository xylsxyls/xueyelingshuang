#include "LockFreeMapTestRunner.h"
#include "LockFreeMapTest.h"
#include "LockFreeMapStatusTest.h"
#ifndef LOCK_FREE_MAP_TEST_SINGLE
#include "LockFreeMapTests.h"
#include "LockFreeMapArrivalTest.h"
#endif
#include <cstdio>
#include <cstring>
#include <exception>
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
        if (last > LOCK_FREE_MAP_TEST_LATE_ARRIVAL)
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
    const std::map<unsigned, LockFreeMapTestEntry> cases = {
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
        { LOCK_FREE_MAP_TEST_POOL, { "池模块独立检查", false, []() { return LockFreeMapPoolModule() == 0; } } },
        { LOCK_FREE_MAP_TEST_HISTORICAL, { "原五十二项兼容检查", true, []() { return LockFreeMapHistorical() == 0; } } },
        { LOCK_FREE_MAP_TEST_PAUSED_GROWTH, { "暂停申请与预留额度", false, &LockFreeMapTests::pausedGrowth } },
        { LOCK_FREE_MAP_TEST_RETIRED_VERSION, { "旧版本退役竞争", false, &LockFreeMapTests::retiredVersionRace } },
        { LOCK_FREE_MAP_TEST_HELPING, { "描述符帮助和清空", false, &LockFreeMapTests::descriptorHelping } },
        { LOCK_FREE_MAP_TEST_FAST_UPDATES, { "无申请更新和条件累加", true, &LockFreeMapTests::fastUpdates } },
        { LOCK_FREE_MAP_TEST_HEADERS, { "四库公开头共存", false, []() { return LockFreeMapHeaders() == 0; } } },
        { LOCK_FREE_MAP_TEST_DIRECT_KEY, { "直接构造键及回滚", false, &LockFreeMapTests::directKeyConstruction } },
        { LOCK_FREE_MAP_TEST_SMALL_BUDGET, { "小预算回收与暂停读者", false, &LockFreeMapTests::smallBudget } },
        { LOCK_FREE_MAP_TEST_SHARED_INDEX, { "索引引用转交与暂停发布者", false, &LockFreeMapTests::sharedIndex } },
        { LOCK_FREE_MAP_TEST_SLOT_CACHE, { "空槽转交与释放后重建", false, &LockFreeMapTests::slotCache } },
        { LOCK_FREE_MAP_TEST_DETACHED_DESTRUCTION, { "独占析构暂停与槽复用", false, &LockFreeMapTests::detachedDestruction } },
        { LOCK_FREE_MAP_TEST_CONTENTION, { "竞争让步及暂停请求者", false, []() { return LockFreeMapContentionProtocol() == 0; } } },
        { LOCK_FREE_MAP_TEST_PROGRESS, { "生产消费逐线程进展", true, []() { return LockFreeMapConcurrentProgress() == 0; } } },
        { LOCK_FREE_MAP_TEST_QUERY_PROGRESS, { "精确查询与结构修改交错", false, []() { return LockFreeMapQueryProgress() == 0; } } },
        { LOCK_FREE_MAP_TEST_LATE_ARRIVAL, { "后来线程逐窗口推进", true, &LockFreeMapArrivalTest::run } },
#endif
        { LOCK_FREE_MAP_TEST_EXAMPLES, { "六端统一接口样例", false, []() { return LockFreeMapCompileExamples() == 0; } } },
        { LOCK_FREE_MAP_TEST_STATUS_CONTRACT, { "普通接口重试与资源错误", false, []()
        {
            const int result = LockFreeMapStatusContract();
            return std::fprintf(LockFreeMapTestReport::m_file, "status_contract,result,%d\n", result) > 0 && result == 0;
        } } },
        { LOCK_FREE_MAP_TEST_PLATFORM, { "原子对齐及低内存", false, []() { return LockFreeMapPlatform() == 0; } } },
        { LOCK_FREE_MAP_TEST_ARGUMENTS, { "参数与报告错误退出码", false, &LockFreeMapTestRunner::arguments } }
    };
    std::set<unsigned> selected;
    const char* mode = "--all";
    const char* reportPath = "LockFreeMapTest-result.csv";
    unsigned repeats = 1;
    bool modeSet = false;
    for (int index = 1; index < argc; ++index)
    {
        const char* arg = argv[index];
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
    LockFreeMapTestFaults::m_enabled.store(1);
#endif
    for (unsigned repeat = 0; repeat < repeats; ++repeat)
    {
        for (auto id = selected.begin(); id != selected.end(); ++id)
        {
            const LockFreeMapTestEntry& entry = cases.find(*id)->second;
            bool passed = false;
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
            passed = passed && LockFreeMapTestValue::m_live.load() == 0 && LockFreeMapTestThread::m_errors.load() == 0 &&
                LockFreeMapTestValue::m_timeouts.load() == 0;
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
    char* parameters[] = { program, ids, first, report, empty };
    if (run(5, parameters) != 3)
    {
        return false;
    }
    char directory[] = "/";
    parameters[4] = directory;
    return run(5, parameters) == 3;
}