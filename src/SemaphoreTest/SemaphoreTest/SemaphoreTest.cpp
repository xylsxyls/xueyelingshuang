#include "SemaphoreTest.h"
#include "SemaphoreTestProcess.h"
#ifndef SEMAPHORE_TEST_BASELINE
#include "SemaphoreTests.h"
#if defined(_WIN32) && defined(PROCESS_WORK_PORTABLE_NOTIFY)
#include "SemaphoreTestProcessMemory.h"
#else
#include "SemaphoreProcessMemory.h"
#endif
#endif
#ifdef _WIN32
#include "SemaphorePerformance.h"
#endif
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <stdexcept>

// 编号、场景、预期和分类只在此表维护，所有运行模式共用
static const SemaphoreTestCase kCases[] = {{SemaphoreTestCount, L"积存计数与重置", L"发布和消费数量完全一致", false},
    {SemaphoreTestDeadline, L"固定超时预算", L"无关提示不能重置总预算", false},
    {SemaphoreTestOverflow, L"64位计数上限", L"溢出不提交且兼容异常可观察", false},
    {SemaphoreTestContention, L"多生产多消费", L"无丢失重复且正常退出", true},
    {SemaphoreTestVisibility, L"发布获取可见性", L"普通数据在成功等待后可见", false},
    {SemaphoreTestEvent, L"非积存事件", L"事件不留给未来等待者", false},
    {SemaphoreTestMixed, L"计数与事件混合", L"事件不增加或消费许可", false},
    {SemaphoreTestRegistration, L"登记睡眠边界", L"受控交错不丢通知", false},
    {SemaphoreTestBroadcast, L"广播快照集合", L"后登记者不消费旧广播", false},
    {SemaphoreTestGrowth, L"池增长策略", L"阈值及最大容量正确", false},
    {SemaphoreTestAllocation, L"批次申请回滚", L"部分申请失败不泄漏预算", false},
    {SemaphoreTestShrink, L"动态缩容", L"遵守空闲期限和初始下限", false},
    {SemaphoreTestRetirement, L"暂停读者回收保护", L"借用期间不复用记录", false},
    {SemaphoreTestPoolStress, L"事件池并发压力", L"容量守恒且记录安全回收", true},
    {SemaphoreTestFault, L"系统故障与提交状态", L"提交后错误不能伪报未提交", false},
    {SemaphoreTestName, L"命名通知边界", L"不存在对象不被打开创建", false},
    {SemaphoreTestIpc, L"真实跨进程通知", L"双方握手计数及退出成功", false},
    {SemaphoreTestExit, L"无限等待退出唤醒", L"退出通知后线程全部回收", false},
    {SemaphoreTestResource, L"资源回落及空闲CPU", L"句柄回到基线且等待不忙转", true},
    {SemaphoreTestEventReuse, L"事件记录复用", L"旧广播不命中新代记录", false},
    {SemaphoreTestHeap, L"实际堆申请故障", L"惰性初始化失败允许恢复", false},
    {SemaphoreTestObjectType, L"同名错误资源类型", L"不接受同名错误内核对象", false},
    {SemaphoreTestQueue, L"四库头与真实队列消费", L"8000项提交和消费集合一致", true},
    {SemaphoreTestWasmMain, L"Wasm主线程能力", L"不支持阻塞和命名IPC明确失败", false},
    {SemaphoreTestWasmWorker, L"Wasm工作线程等待", L"主循环通知和工作线程超时正确", false},
    {SemaphoreTestReservation, L"并发扩容预算", L"预留与已分配合计不超限", true},
    {SemaphoreTestPausedWaiter, L"暂停等待者不私占许可", L"其他消费者仍能取得许可", false},
    {SemaphoreTestPolicy, L"跨库边界策略副本", L"副本存活并正确报告回调异常", false},
    {SemaphoreTestNameCompatibility, L"旧命名规则兼容", L"不擅自收紧原有合法名称", false},
    {SemaphoreTestPartialClose, L"部分关闭故障", L"剩余资源可重试关闭", false},
    {SemaphoreTestPerformance, L"原始公共接口性能", L"工作量正确并记录全量指标", true},
    {SemaphoreTestLazyPool, L"首次事件池并发创建", L"候选只有一次发布且失败不泄漏", false},
    {SemaphoreTestNativeIpc, L"直接原生IPC对照", L"直接系统调用计数守恒", true},
    {SemaphoreTestNativeCombined, L"原生组合调用诊断", L"组合原生往返计数守恒", true},
    {SemaphoreTestIpcCombined, L"IPC组合接口边界", L"独立报告发送和消费状态", false},
    {SemaphoreTestIpcExchange, L"双进程组合往返", L"200次请求回复精确匹配", false},
    {SemaphoreTestCombinedFault, L"组合操作各阶段故障", L"未知结果不能被当作可重发", false},
    {SemaphoreTestLocalCombined, L"本地组合准备状态", L"四种准备组合及总预算正确", false},
    {SemaphoreTestPreparedEvent, L"预备之后事件兼容", L"原有事件用途保持", false},
    {SemaphoreTestLocalFault, L"本地组合提交资源边界", L"64位溢出及句柄回收正确", false},
    {SemaphoreTestCombinedContention, L"并发组合计数", L"2000个许可无丢失重复", true},
    {SemaphoreTestHandoffPerformance, L"冷唤醒和IPC往返性能", L"保留有序样本及逐轮分位数", true},
    {SemaphoreTestWasmCombined, L"Wasm组合等待", L"工作线程组合等待及主循环通知正确", false},
    {SemaphoreTestHarness, L"测试选择及失败判定自检", L"拒绝非法空选择且捕获失败退出", false},
    {SemaphoreTestBenchmarkSmoke, L"早期性能烟测", L"计数和往返烟测通过", true},
    {SemaphoreTestPublicationReservation, L"发布前容量预留", L"批次接入可用链前竞争者可重试", false},
    {SemaphoreTestAlignment, L"真实栈对象对齐", L"不同栈帧及线程中的计数对象满足8字节对齐", false},
    {SemaphoreTestEventPerformance, L"事件后端性能诊断", L"相同代次工作量记录默认及预备后端延迟", true},
    {SemaphoreTestCountPerformance, L"计数与生命周期性能", L"独立复测完整计数工作量及资源回落", true},
    {SemaphoreTestIpcPerformance, L"命名通知往返性能", L"独立进程回复恰好匹配全部请求", true},
    {SemaphoreTestEventAddressLifetime, L"事件地址借用生命期", L"暂停提示期间禁止复用，完成后不积存旧事件", false},
    {SemaphoreTestRetirementBudget, L"退役回收总预算", L"大批记录每轮至多回收64条且最终完整回落", false},
    {SemaphoreTestRetirementBorrow, L"退役栈借用保护", L"暂停读者及并发重新入栈不丢记录、不提前复用", false},
    {SemaphoreTestNameOwner, L"撤名所有权与新旧代隔离", L"打开者不能撤名，旧引用不影响同名新对象", false},
    {SemaphoreTestPoolPerformance, L"事件记录完整循环性能", L"固定容量无新分配，登记通知回收计数守恒", true},
    {SemaphoreTestMarkedFinish, L"退出标记与帮助摘除", L"暂停退出者返回前记录不可复用，之后容量完整回落", false},
    {SemaphoreTestMobileFileType, L"移动映射错误文件类型", L"FIFO打开及时失败且不删除外部文件", false},
    {SemaphoreTestEventFairness, L"事件等待进展", L"后登记者不能反复越过较早等待者", false},
    {SemaphoreTestCountHandoff, L"计数直接交付", L"暂停接收者保有已交付许可且后续消费者可继续", false},
    {SemaphoreTestFairnessStress, L"持续竞争逐线程进展", L"记录每线程完成量和最长无进展时间且许可守恒", true},
    {SemaphoreTestLargeCount, L"大计数表示与资源边界", L"64位计数完整且失败不提交", false},
    {SemaphoreTestFairnessBoundary, L"公平等待取消及故障", L"超时竞争无丢失、错误可见、记录回落", false},
    {SemaphoreTestWasmAdmission, L"Wasm内部登记及计数表示", L"Wasm32/64等待顺序与大计数切换正确", false}};

/** 将中文元数据转换为UTF8，不依赖进程区域设置
@param [in] source 宽字符文本
@return UTF8文本
*/
static std::string Utf8(const wchar_t* source)
{
    std::string output;
    for (size_t i = 0; source[i] != 0; ++i)
    {
        const uint32_t value = static_cast<uint32_t>(source[i]);
        if (value < 0x80)
        {
            output += static_cast<char>(value);
        }
        else if (value < 0x800)
        {
            output += static_cast<char>(0xc0 | (value >> 6));
            output += static_cast<char>(0x80 | (value & 63));
        }
        else
        {
            output += static_cast<char>(0xe0 | (value >> 12));
            output += static_cast<char>(0x80 | ((value >> 6) & 63));
            output += static_cast<char>(0x80 | (value & 63));
        }
    }
    return output;
}

/** 查找稳定用例，拒绝未知编号
@param [in] id 稳定编号
@return 元数据或空
*/
static const SemaphoreTestCase* FindCase(int32_t id)
{
    for (size_t i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i)
    {
        if (kCases[i].m_id == id)
        {
            return &kCases[i];
        }
    }
    return nullptr;
}

/** 生成十进制参数
@param [in] value 非负数
@return 十进制文本
*/
static std::string Number(int32_t value)
{
    char buffer[32];
    std::sprintf(buffer, "%d", value);
    return buffer;
}

/** 创建新报告，已有报告拒绝覆盖，读写异常向主入口传播
@param [in,out] file 报告流
@param [in] path 本批文件名
*/
static void OpenReport(std::ofstream& file, const std::string& path)
{
    std::ifstream existing(path.c_str(), std::ios::binary);
    if (existing.good())
    {
        throw std::runtime_error("Report exists; use a new output directory");
    }
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file.open(path.c_str(), std::ios::binary);
}

bool SemaphoreTest::select(const std::string& text, std::vector<int32_t>& selected)
{
    std::vector<int32_t> result;
    size_t offset = 0;
    while (offset < text.size())
    {
        int32_t first = 0;
        if (text[offset] < '0' || text[offset] > '9')
        {
            return false;
        }
        while (offset < text.size() && text[offset] >= '0' && text[offset] <= '9')
        {
            first = first * 10 + text[offset++] - '0';
            if (first > SemaphoreTestWasmAdmission)
            {
                return false;
            }
        }
        int32_t last = first;
        if (offset < text.size() && text[offset] == '-')
        {
            ++offset;
            last = 0;
            if (offset == text.size() || text[offset] < '0' || text[offset] > '9')
            {
                return false;
            }
            while (offset < text.size() && text[offset] >= '0' && text[offset] <= '9')
            {
                last = last * 10 + text[offset++] - '0';
                if (last > SemaphoreTestWasmAdmission)
                {
                    return false;
                }
            }
        }
        if (last < first)
        {
            return false;
        }
        for (int32_t id = first; id <= last; ++id)
        {
            if (FindCase(id) == nullptr)
            {
                return false;
            }
            if (std::find(result.begin(), result.end(), id) == result.end())
            {
                result.push_back(id);
            }
        }
        if (offset < text.size() && (text[offset++] != ',' || offset == text.size()))
        {
            return false;
        }
    }
    if (result.empty())
    {
        return false;
    }
    selected.swap(result);
    return true;
}

int32_t SemaphoreTest::execute(int32_t id, bool combined)
{
    if (id == SemaphoreTestHarness)
    {
        std::vector<int32_t> selected;
        const char* invalid[] = {"", ",", "25,", "25x", "26-25", "0", "24", "999", "999999999999999", "25--26"};
        for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        {
            if (select(invalid[i], selected))
            {
                return 1;
            }
        }
        return select("25,28,30-33,25", selected) && selected.size() == 6 && checkSamples() ? 0 : 1;
    }
#ifdef _WIN32
    if (id == SemaphoreTestNativeIpc || id == SemaphoreTestNativeCombined)
    {
#ifdef SEMAPHORE_TEST_NATIVE_CONTROL
        return SemaphorePerformance::run(id, id == SemaphoreTestNativeCombined);
#else
        return 77;
#endif
    }
    if (id == SemaphoreTestPerformance || id == SemaphoreTestHandoffPerformance || id == SemaphoreTestAlignment ||
        id == SemaphoreTestEventPerformance || id == SemaphoreTestCountPerformance || id == SemaphoreTestIpcPerformance ||
        id == SemaphoreTestPoolPerformance)
    {
#ifdef SEMAPHORE_TEST_BASELINE
        if (combined || id == SemaphoreTestAlignment)
        {
            return 77;
        }
#endif
        return SemaphorePerformance::run(id, combined);
    }
#endif
#ifndef SEMAPHORE_TEST_BASELINE
    if (id == SemaphoreTestQueue)
    {
#ifdef SEMAPHORE_TEST_QUEUE
        return SemaphoreQueueTests();
#else
        return 77;
#endif
    }
    if (id == SemaphoreTestPolicy)
    {
        return SemaphorePolicyTests();
    }
#ifdef _WIN32
    if (id == SemaphoreTestBenchmarkSmoke)
    {
        return SemaphoreBench();
    }
#endif
    if ((id >= 25 && id <= 65 && id != 47 && id != 48 && id != 49 && id != 52 && id != 55 && id != 57 && id != 58) ||
        id == SemaphoreTestPublicationReservation || id == SemaphoreTestMarkedFinish ||
        id == SemaphoreTestMobileFileType || (id >= SemaphoreTestEventFairness && id <= SemaphoreTestFairnessBoundary) ||
        (id >= SemaphoreTestEventAddressLifetime && id <= SemaphoreTestNameOwner))
    {
        return SemaphoreTests::runCase(id) ? 0 : 77;
    }
#endif
    return 77;
}

int32_t SemaphoreTest::batch(const std::vector<int32_t>& selected, const std::string& output, bool combined)
{
    if (!SemaphoreTestProcess::makeDirectory(output))
    {
        throw std::runtime_error("Cannot create output directory; create its parent first");
    }
    const std::string executable = SemaphoreTestProcess::executable();
    const std::string identity = SemaphoreTestProcess::hash(executable);
    std::ofstream report;
    OpenReport(report, output + "/cases.csv");
    report << "id,name,expected,type,result,exit,elapsed_ms,log\n";
    int32_t passed = 0, failed = 0, skipped = 0;
    const double started = SemaphoreTestProcess::now();
    for (size_t i = 0; i < selected.size(); ++i)
    {
        const SemaphoreTestCase* item = FindCase(selected[i]);
        const std::string log = output + "/case-" + Number(item->m_id) + ".log";
        std::vector<std::string> arguments;
        arguments.push_back("--execute");
        arguments.push_back(Number(item->m_id));
        if (combined)
        {
            arguments.push_back("--combined");
        }
        const double before = SemaphoreTestProcess::now();
        int32_t code = SemaphoreTestProcess::run(executable, arguments, log, item->m_stress ? 180000 : 90000);
        if (item->m_id == SemaphoreTestHarness && code == 0)
        {
            arguments.clear();
            arguments.push_back("--self-fail");
            if (SemaphoreTestProcess::run(executable, arguments, output + "/harness-failure.log", 5000) != 19)
            {
                code = 125;
            }
            arguments[0] = "--self-sleep";
            if (SemaphoreTestProcess::run(executable, arguments, output + "/harness-timeout.log", 20) != 124)
            {
                code = 125;
            }
        }
        const char* result = code == 0 ? "PASS" : (code == 77 ? "NOT_RUN" : "FAIL");
        passed += code == 0 ? 1 : 0;
        skipped += code == 77 ? 1 : 0;
        failed += code != 0 && code != 77 ? 1 : 0;
        report << item->m_id << ',' << Utf8(item->m_name) << ',' << Utf8(item->m_expected) << ','
               << (item->m_stress ? "stress" : "instant") << ',' << result << ',' << code << ','
               << SemaphoreTestProcess::now() - before << ',' << log << "\n";
        report.flush();
        std::printf("%s id=%d exit=%d\n", result, item->m_id, code);
        std::fflush(stdout);
    }
    report.close();
    std::ofstream summary;
    OpenReport(summary, output + "/summary.txt");
    summary << "executable=" << executable << "\nsha256=" << identity << "\n";
    summary << "unique_ids=" << selected.size() << "\nconfiguration_runs=" << selected.size() << "\npassed=" << passed
            << "\nfailed=" << failed << "\nnot_run=" << skipped
            << "\ncancelled=0\nelapsed_ms=" << SemaphoreTestProcess::now() - started << "\n";
    if (SemaphoreTestProcess::hash(executable) != identity)
    {
        ++failed;
        summary << "FAIL executable changed during batch\n";
    }
    summary.close();
    return failed != 0 ? 1 : (passed == 0 ? 77 : 0);
}

/** 计算全部轮次的中位数，不剔除慢轮次
@param [in] values 本组全部有效轮次
@return 中位数
*/
static double Median(std::vector<double> values)
{
    if (values.empty())
    {
        throw std::runtime_error("Empty performance group");
    }
    std::sort(values.begin(), values.end());
    const size_t count = values.size();
    return count % 2 != 0 ? values[count / 2] : (values[count / 2 - 1] + values[count / 2]) / 2;
}

/** 读取原始样本行，验证数值及分隔符，拒绝无穷、NaN和截断数据
@param [in] line 有序原始样本行
@param [in] name 指标名称
@param [out] values 原始样本
@return 本行是否匹配指标，匹配但格式错误时抛异常
*/
static bool ReadSamples(const std::string& line, const char* name, std::vector<double>& values)
{
    const std::string prefix = std::string("SAMPLES ") + name + " [";
    if (line.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }
    if (!values.empty())
    {
        throw std::runtime_error("Duplicate sample series");
    }
    const char* cursor = line.c_str() + prefix.size();
    for (;;)
    {
        char* next = nullptr;
        const double value = std::strtod(cursor, &next);
        if (next == cursor || !std::isfinite(value) || value < 0)
        {
            throw std::runtime_error("Invalid latency sample");
        }
        values.push_back(value);
        if (*next == ']' && next[1] == 0)
        {
            return true;
        }
        if (*next != ',')
        {
            throw std::runtime_error("Truncated sample series");
        }
        cursor = next + 1;
    }
}

/** 读取性能日志中的唯一数值字段，拒绝重复、截断和非有限数
@param [in] line 本程序生成的单行指标
@param [in] field 字段名称
@return 有限非负数；缺少或格式错误时抛异常交由测试入口报告
*/
static double ReadMetricNumber(const std::string& line, const char* field)
{
    const std::string key = std::string("\"") + field + "\":";
    const size_t position = line.find(key);
    if (position == std::string::npos || line.find(key, position + key.size()) != std::string::npos)
    {
        throw std::runtime_error("Missing or duplicate metric field");
    }
    const char* first = line.c_str() + position + key.size();
    char* end = nullptr;
    const double value = std::strtod(first, &end);
    if (end == first || !std::isfinite(value) || value < 0 || (*end != ',' && *end != '}'))
    {
        throw std::runtime_error("Invalid metric field");
    }
    return value;
}

bool SemaphoreTest::checkSamples()
{
    std::vector<double> values;
    if (!ReadSamples("SAMPLES probe [3,1,2,4]", "probe", values) || values.size() != 4 || Median(values) != 2.5)
    {
        return false;
    }
    const char* invalid[] = {"SAMPLES probe []", "SAMPLES probe [NaN]", "SAMPLES probe [1,]", "SAMPLES probe [1",
        "SAMPLES probe [-1]", "SAMPLES probe [1e999]"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        values.clear();
        bool rejected = false;
        try
        {
            ReadSamples(invalid[i], "probe", values);
        }
        catch (const std::runtime_error&)
        {
            rejected = true;
        }
        if (!rejected)
        {
            return false;
        }
    }
    if (ReadMetricNumber("METRIC {\"name\":\"probe\",\"value\":12.5}", "value") != 12.5)
    {
        return false;
    }
    const char* badFields[] = {"{}", "{\"value\":NaN}", "{\"value\":-1}", "{\"value\":1e999}",
        "{\"value\":1,\"value\":2}", "{\"value\":1", "{\"value\":1x}"};
    for (size_t i = 0; i < sizeof(badFields) / sizeof(badFields[0]); ++i)
    {
        bool rejected = false;
        try
        {
            ReadMetricNumber(badFields[i], "value");
        }
        catch (const std::runtime_error&)
        {
            rejected = true;
        }
        if (!rejected)
        {
            return false;
        }
    }
    return true;
}

int32_t SemaphoreTest::compare(const std::string& baseline, const std::string& output, int32_t rounds, int32_t id,
    bool baselineCurrent)
{
    if ((id != 55 && id != 66 && id != 72 && id != 73 && id != 74 && id != 79) || rounds < 1 || rounds > 9 ||
        !SemaphoreTestProcess::makeDirectory(output))
    {
        return 2;
    }
    const std::string current = SemaphoreTestProcess::executable();
    const std::string baselineHash = SemaphoreTestProcess::hash(baseline);
    const std::string currentHash = SemaphoreTestProcess::hash(current);
    const char* legacyVariants[] = {"original", "current_separate", "current_combined"};
    const char* recentVariants[] = {"previous_separate", "previous_combined", "current_separate", "current_combined"};
    const char* const* variants = baselineCurrent ? recentVariants : legacyVariants;
    const int32_t variantCount = baselineCurrent ? 4 : 3;
    const int32_t firstCurrent = baselineCurrent ? 2 : 1;
    const int32_t latencyCount = 8, rateCount = 6, scalarCount = 13;
    const char* names[] = {"cold_publish_to_observe", "ipc_roundtrip", "warm_roundtrip",
        "event_one_observed", "event_all_8_observed", "event_pool_cycle", "event_pool_batch_8", "event_pool_batch_64"};
    const uint32_t sampleCounts[] = {300, 2000, 20000, 200, 200, 300, 300, 300};
    std::vector<double> observations[4][latencyCount][4];
    std::vector<double> throughput[4][rateCount];
    std::vector<double> elapsedMilliseconds[4][rateCount];
    const char* rates[] = {"bulk", "spsc_1p1c", "mpsc_4p1c", "mpmc_4p4c", "mpmc_8p8c", "mpmc_16p16c"};
    const char* scalarMetrics[] = {"idle", "idle", "lifecycle", "lifecycle", "event_one_boundary",
        "event_all_boundary", "spsc_1p1c", "mpsc_4p1c", "mpmc_4p4c", "mpmc_8p8c", "mpmc_16p16c",
        "lifecycle_resources", "lifecycle_resources"};
    const char* scalarFields[] = {"wall_ms", "cpu_ms", "cycle_us", "object_bytes", "rescue_timeouts",
        "rescue_timeouts", "cpu_ms", "cpu_ms", "cpu_ms", "cpu_ms", "cpu_ms", "handles_before", "handles_after"};
    std::vector<double> additional[4][scalarCount];
    // 独立性能入口复用55号的同一工作负载及完整判定，只选择本次实际执行的指标
    const bool latencyActive[] = {id == 55 || id == 66, id == 55 || id == 66 || id == 74,
        id == 55 || id == 66, id == 55 || id == 72, id == 55 || id == 72, id == 79, id == 79, id == 79};
    const bool ratesActive = id == 55 || id == 73;
    const bool scalarActive[] = {id == 55 || id == 66, id == 55 || id == 66, ratesActive, ratesActive,
        id == 55 || id == 72, id == 55 || id == 72, ratesActive, ratesActive, ratesActive, ratesActive, ratesActive,
        ratesActive, ratesActive};
    std::ofstream report;
    OpenReport(report, output + "/rounds.csv");
    report << "round,variant,exit,log\n";
    for (int32_t round = 0; round < rounds; ++round)
    {
        for (int32_t position = 0; position < variantCount; ++position)
        {
            const int32_t variant = (position + round) % variantCount;
            const std::string log = output + "/round-" + Number(round + 1) + "-" + variants[variant] + ".log";
            std::vector<std::string> arguments;
            arguments.push_back("--execute");
            arguments.push_back(Number(id));
            if (baselineCurrent ? variant % 2 == 1 : variant == 2)
            {
                arguments.push_back("--combined");
            }
            int32_t code = SemaphoreTestProcess::run(variant < firstCurrent ? baseline : current,
                arguments, log, 120000);
            std::ifstream input(log.c_str());
            bool seen[latencyCount] = {};
            bool rateSeen[rateCount] = {};
            bool scalarSeen[scalarCount] = {};
            for (int32_t metric = 0; metric < latencyCount; ++metric)
            {
                seen[metric] = !latencyActive[metric];
            }
            for (int32_t metric = 0; metric < rateCount; ++metric)
            {
                rateSeen[metric] = !ratesActive;
            }
            for (int32_t metric = 0; metric < scalarCount; ++metric)
            {
                scalarSeen[metric] = !scalarActive[metric];
            }
            std::vector<double> raw[latencyCount];
            std::string line;
            while (std::getline(input, line))
            {
                for (int32_t metric = 0; metric < latencyCount; ++metric)
                {
                    ReadSamples(line, names[metric], raw[metric]);
                }
                char name[128] = {};
                if (std::sscanf(line.c_str(), "METRIC {\"name\":\"%127[^\"]\"", name) == 1)
                {
                    for (int32_t metric = 0; metric < scalarCount; ++metric)
                    {
                        if (std::strcmp(name, scalarMetrics[metric]) == 0)
                        {
                            const double value = ReadMetricNumber(line, scalarFields[metric]);
                            if (scalarSeen[metric])
                            {
                                code = 125;
                            }
                            scalarSeen[metric] = true;
                            additional[variant][metric].push_back(value);
                        }
                    }
                }
                uint32_t samples = 0;
                double p50 = 0, p95 = 0, p99 = 0, maximum = 0;
                uint32_t permits = 0;
                double rate = 0;
                if (std::sscanf(line.c_str(),
                        "METRIC {\"name\":\"%127[^\"]\",\"permits\":%u,\"permits_per_second\":%lf", name, &permits,
                        &rate) == 3)
                {
                    for (int32_t metric = 0; metric < rateCount; ++metric)
                    {
                        if (std::strcmp(name, rates[metric]) == 0)
                        {
                            if (rateSeen[metric] || permits != (metric == 0 ? 1000000 : 400000) ||
                                !std::isfinite(rate) || rate <= 0)
                            {
                                code = 125;
                            }
                            rateSeen[metric] = true;
                            throughput[variant][metric].push_back(rate);
                            const double elapsed = ReadMetricNumber(line, "elapsed_ms");
                            if (elapsed <= 0 || std::fabs(rate * elapsed / 1000.0 / permits - 1) > 0.01)
                            {
                                code = 125;
                            }
                            elapsedMilliseconds[variant][metric].push_back(elapsed);
                        }
                    }
                }
                if (std::sscanf(line.c_str(),
                        "METRIC "
                        "{\"name\":\"%127[^\"]\",\"samples\":%u,\"p50_us\":%lf,\"p95_us\":%lf,\"p99_us\":%lf,"
                        "\"max_us\":%lf}",
                        name, &samples, &p50, &p95, &p99, &maximum) != 6)
                {
                    continue;
                }
                for (int32_t metric = 0; metric < latencyCount; ++metric)
                {
                    if (std::strcmp(name, names[metric]) == 0)
                    {
                        const uint32_t expected = sampleCounts[metric];
                        if (seen[metric] || samples != expected || raw[metric].size() != expected ||
                            !std::isfinite(p50) || !std::isfinite(p95) || !std::isfinite(p99) ||
                            !std::isfinite(maximum) || p50 < 0 || p95 < p50 || p99 < p95 || maximum < p99)
                        {
                            code = 125;
                        }
                        else
                        {
                            std::sort(raw[metric].begin(), raw[metric].end());
                            if (std::fabs(raw[metric][(expected * 50 + 99) / 100 - 1] - p50) > 0.0001 ||
                                std::fabs(raw[metric][(expected * 95 + 99) / 100 - 1] - p95) > 0.0001 ||
                                std::fabs(raw[metric][(expected * 99 + 99) / 100 - 1] - p99) > 0.0001 ||
                                std::fabs(raw[metric].back() - maximum) > 0.0001)
                            {
                                code = 125;
                            }
                        }
                        seen[metric] = true;
                        observations[variant][metric][0].push_back(p50);
                        observations[variant][metric][1].push_back(p95);
                        observations[variant][metric][2].push_back(p99);
                        observations[variant][metric][3].push_back(maximum);
                    }
                }
            }
            const bool missingLatency = std::find(seen, seen + latencyCount, false) != seen + latencyCount;
            const bool missingRate = std::find(rateSeen, rateSeen + rateCount, false) != rateSeen + rateCount;
            const bool missingScalar = std::find(scalarSeen, scalarSeen + scalarCount, false) != scalarSeen + scalarCount;
            if (missingLatency || missingRate || missingScalar || input.bad())
            {
                code = 125;
            }
            report << round + 1 << ',' << variants[variant] << ',' << code << ',' << log << "\n";
            report.flush();
            std::printf("ROUND %d %s exit=%d\n", round + 1, variants[variant], code);
            std::fflush(stdout);
            if (code != 0)
            {
                return 1;
            }
        }
    }
    report.close();
    std::ofstream summary;
    OpenReport(summary, output + "/comparison.csv");
    summary << "metric,percentile,variant,original_us,current_us,reduction_percent,min_us,max_us\n";
    const char* percentiles[] = {"p50", "p95", "p99", "max"};
    for (int32_t metric = 0; metric < latencyCount; ++metric)
    {
        if (!latencyActive[metric])
        {
            continue;
        }
        for (int32_t p = 0; p < 4; ++p)
        {
            for (int32_t variant = firstCurrent; variant < variantCount; ++variant)
            {
                const int32_t reference = baselineCurrent ? variant - firstCurrent : 0;
                const double before = Median(observations[reference][metric][p]);
                const std::vector<double>& values = observations[variant][metric][p];
                const double after = Median(values);
                summary << names[metric] << ',' << percentiles[p] << ',' << variants[variant] << ',' << before << ','
                        << after << ',' << (1 - after / before) * 100 << ','
                        << *std::min_element(values.begin(), values.end()) << ','
                        << *std::max_element(values.begin(), values.end()) << "\n";
            }
        }
    }
    summary.close();
    std::ofstream speeds;
    OpenReport(speeds, output + "/throughput.csv");
    speeds << "metric,variant,original_per_second,current_per_second,ratio,min_per_second,max_per_second,"
              "original_total_per_second,current_total_per_second,total_ratio\n";
    for (int32_t metric = 0; metric < rateCount; ++metric)
    {
        if (!ratesActive)
        {
            continue;
        }
        for (int32_t variant = firstCurrent; variant < variantCount; ++variant)
        {
            const int32_t reference = baselineCurrent ? variant - firstCurrent : 0;
            const double before = Median(throughput[reference][metric]);
            const std::vector<double>& values = throughput[variant][metric];
            const double after = Median(values);
            double beforeMilliseconds = 0, afterMilliseconds = 0;
            for (int32_t round = 0; round < rounds; ++round)
            {
                beforeMilliseconds += elapsedMilliseconds[reference][metric][round];
                afterMilliseconds += elapsedMilliseconds[variant][metric][round];
            }
            const double permits = (metric == 0 ? 1000000.0 : 400000.0) * rounds;
            speeds << rates[metric] << ',' << variants[variant] << ',' << before << ',' << after << ','
                   << after / before << ',' << *std::min_element(values.begin(), values.end()) << ','
                   << *std::max_element(values.begin(), values.end()) << ',' << permits * 1000 / beforeMilliseconds
                   << ',' << permits * 1000 / afterMilliseconds << ',' << beforeMilliseconds / afterMilliseconds << "\n";
        }
    }
    speeds.close();
    std::ofstream extras;
    OpenReport(extras, output + "/additional.csv");
    extras << "metric,field,variant,original,current,change_percent,min,max\n";
    for (int32_t metric = 0; metric < scalarCount; ++metric)
    {
        if (!scalarActive[metric])
        {
            continue;
        }
        for (int32_t variant = firstCurrent; variant < variantCount; ++variant)
        {
            const int32_t reference = baselineCurrent ? variant - firstCurrent : 0;
            const double before = Median(additional[reference][metric]);
            const std::vector<double>& values = additional[variant][metric];
            const double after = Median(values);
            extras << scalarMetrics[metric] << ',' << scalarFields[metric] << ',' << variants[variant] << ','
                   << before << ',' << after << ',';
            if (before != 0)
            {
                extras << (after / before - 1) * 100;
            }
            else
            {
                extras << "NA";
            }
            extras << ',' << *std::min_element(values.begin(), values.end()) << ','
                   << *std::max_element(values.begin(), values.end()) << "\n";
        }
    }
    extras.close();
    std::ofstream identity;
    OpenReport(identity, output + "/identity.txt");
    identity << "original=" << baseline << "\noriginal_sha256=" << baselineHash << "\ncurrent=" << current
             << "\ncurrent_sha256=" << currentHash << "\nrounds=" << rounds << "\ncase=" << id
             << "\nbaseline_current=" << (baselineCurrent ? 1 : 0)
             << "\ncontrols=default affinity/priority/power; no sample removal\n";
    identity.close();
    return baselineHash == SemaphoreTestProcess::hash(baseline) && currentHash == SemaphoreTestProcess::hash(current)
        ? 0
        : 1;
}

int32_t SemaphoreTest::run(int32_t argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--self-sleep") == 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return 0;
    }
#if defined(SEMAPHORE_SHARED_COUNTER) && !defined(_WIN32)
    const char* directory = std::getenv("SEMAPHORE_TEST_DIRECTORY");
    if (directory == nullptr || !Semaphore::setProcessDirectory(directory))
    {
        std::fprintf(stderr, "Set SEMAPHORE_TEST_DIRECTORY to an owned existing absolute directory\n");
        return 2;
    }
#endif
    if (argc == 2 && std::strcmp(argv[1], "--self-fail") == 0)
    {
        std::fprintf(stderr, "Intentional harness failure; no crash\n");
        return 19;
    }
#ifdef _WIN32
    if (argc == 3 &&
        (std::strcmp(argv[1], "--performance-peer") == 0 || std::strcmp(argv[1], "--performance-peer-combined") == 0))
    {
        return SemaphorePerformance::peer(argv[2], std::strcmp(argv[1], "--performance-peer-combined") == 0);
    }
#endif
#ifndef SEMAPHORE_TEST_BASELINE
    if (argc == 3 && (std::strcmp(argv[1], "--peer") == 0 || std::strcmp(argv[1], "--exchange-peer") == 0))
    {
        return SemaphoreTests::peer(argv[2], std::strcmp(argv[1], "--exchange-peer") == 0);
    }
#endif
    std::string mode = "instant", selection, output, baseline;
    bool combined = false;
    bool baselineCurrent = false;
    int32_t rounds = 5, direct = 0;
    for (int32_t i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--combined")
        {
            combined = true;
        }
        else if (argument == "--list")
        {
            for (size_t k = 0; k < sizeof(kCases) / sizeof(kCases[0]); ++k)
            {
                std::printf(
                    "%d %s %s\n", kCases[k].m_id, Utf8(kCases[k].m_name).c_str(), Utf8(kCases[k].m_expected).c_str());
            }
            return 0;
        }
        else if (i + 1 < argc)
        {
            const std::string value = argv[++i];
            if (argument == "--mode")
            {
                mode = value;
            }
            else if (argument == "--cases" || argument == "--case")
            {
                mode = "selected";
                selection = value;
            }
            else if (argument == "--output")
            {
                output = value;
            }
            else if (argument == "--compare" || argument == "--compare-current")
            {
                baseline = value;
                baselineCurrent = argument == "--compare-current";
            }
            else if (argument == "--rounds" && value.size() == 1 && value[0] >= '1' && value[0] <= '9')
            {
                rounds = value[0] - '0';
            }
            else if (argument == "--execute")
            {
                std::vector<int32_t> ids;
                if (!select(value, ids) || ids.size() != 1)
                {
                    return 2;
                }
                direct = ids[0];
            }
            else
            {
                return 2;
            }
        }
        else
        {
            return 2;
        }
    }
    if (direct != 0)
    {
        const int32_t code = execute(direct, combined);
        std::printf("%s id=%d exit=%d\n", code == 0 ? "PASS" : (code == 77 ? "NOT_RUN" : "FAIL"), direct, code);
        return code;
    }
    if (output.empty())
    {
        std::fprintf(stderr, "Usage: --mode selected|instant|stress|all --cases 25,28,30-33 --output NEW_DIRECTORY\n");
        std::fprintf(stderr, "Compare: --compare ORIGINAL_EXE --rounds 5 --output NEW_DIRECTORY\n");
        return 2;
    }
    if (!baseline.empty())
    {
        int32_t compared = SemaphoreTestPerformance;
        if (!selection.empty())
        {
            std::vector<int32_t> ids;
            if (!select(selection, ids) || ids.size() != 1)
            {
                return 2;
            }
            compared = ids[0];
        }
        return compare(baseline, output, rounds, compared, baselineCurrent);
    }
    std::vector<int32_t> selected;
    if (mode == "selected")
    {
        if (!select(selection, selected))
        {
            return 2;
        }
    }
    else if (mode == "instant" || mode == "stress" || mode == "all")
    {
        for (size_t i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i)
        {
            if (mode == "all" || (mode == "stress") == kCases[i].m_stress)
            {
                selected.push_back(kCases[i].m_id);
            }
        }
    }
    else
    {
        return 2;
    }
    return batch(selected, output, combined);
}

/** 测试主入口，所有可传播的异常都形成失败退出
@param [in] argc 参数数量
@param [in] argv 参数数组
@return 非零表示失败或明确未执行
*/
int32_t main(int32_t argc, char** argv)
{
    try
    {
        return SemaphoreTest::run(argc, argv);
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "FAIL exception: %s\n", error.what());
        return 1;
    }
    catch (...)
    {
        std::fprintf(stderr, "FAIL unknown exception\n");
        return 1;
    }
}