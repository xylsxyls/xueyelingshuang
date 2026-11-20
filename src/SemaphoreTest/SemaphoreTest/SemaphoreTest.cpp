#include "SemaphoreTest.h"
#include "SemaphoreTestProcess.h"
#include "SemaphoreTests.h"
#if defined(_WIN32) && defined(PROCESS_WORK_PORTABLE_NOTIFY)
#include "SemaphoreTestProcessMemory.h"
#else
#include "SemaphoreProcessMemory.h"
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
    {SemaphoreTestOverflow, L"64位计数上限", L"满载探测不提交，普通发布等空位", false},
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
    {SemaphoreTestNameCompatibility, L"命名规则与平台边界", L"合法名称可连接并完成通知", false},
    {SemaphoreTestPartialClose, L"部分关闭故障", L"剩余资源可重试关闭", false},
    {SemaphoreTestPerformance, L"公共接口综合性能", L"工作量正确并记录全量指标", true},
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
    {SemaphoreTestBenchmarkSmoke, L"公开接口性能烟测", L"计数和往返烟测通过", true},
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
    {SemaphoreTestWasmAdmission, L"Wasm内部登记及计数表示", L"Wasm32/64等待顺序与大计数切换正确", false},
    {SemaphoreTestRetryOptions, L"重试开关及容量退避", L"满载只在允许时等待，4096次后每轮休眠且不重复提交", false},
    {SemaphoreTestConfiguration, L"实例构造前配置", L"默认、配置应用、维护预算、越界回滚及实例隔离", false},
    {SemaphoreTestLongSlice, L"本地长等待片段", L"配置超长片段，计数与事件均按总预算超时", false},
    {SemaphoreTestEmptyProbe, L"零预算组合探测", L"源发布一次，空目标超时且不创建池", false},
    {SemaphoreTestSendDeadline, L"组合发送竞争总期限", L"发送竞争耗尽预算后不发布也不消费", false}};

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

/** 转义CSV文本字段，允许测试输出路径包含逗号、引号或换行
@param [in] value 一个完整字段
@return 带引号且内部引号成对的CSV字段
*/
static std::string CsvField(const std::string& value)
{
    std::string field(1, '"');
    for (size_t i = 0; i < value.size(); ++i)
    {
        if (value[i] == '"')
        {
            field += '"';
        }
        field += value[i];
    }
    field += '"';
    return field;
}

/** 创建新报告，已有报告拒绝覆盖，读写异常向主入口传播
@param [in,out] file 报告流
@param [in] path 本批文件名
*/
static void OpenReport(std::ofstream& file, const std::string& path)
{
    if (!SemaphoreTestProcess::reserveReport(path))
    {
        throw std::runtime_error("Cannot exclusively create report; use a new writable output directory");
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
            if (first > SemaphoreTestSendDeadline)
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
                if (last > SemaphoreTestSendDeadline)
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
    std::sort(result.begin(), result.end());
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
        bool nulRejected = false;
        try
        {
            const std::vector<std::string> arguments(1, std::string("x\0y", 3));
            SemaphoreTestProcess::run(SemaphoreTestProcess::executable(), arguments, "", 1);
        }
        catch (const std::invalid_argument&)
        {
            nulRejected = true;
        }
        return select("33,25,28,30-33,25", selected) && selected.size() == 6 &&
            selected.front() == 25 && selected.back() == 33 && std::is_sorted(selected.begin(), selected.end()) &&
            nulRejected && checkSamples() ? 0 : 1;
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
        return SemaphorePerformance::run(id, combined);
    }
#endif
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
        id == SemaphoreTestMobileFileType || id == SemaphoreTestRetryOptions || id == SemaphoreTestConfiguration || (id >= SemaphoreTestLongSlice && id <= SemaphoreTestSendDeadline) ||
        (id >= SemaphoreTestEventFairness && id <= SemaphoreTestFairnessBoundary) ||
        (id >= SemaphoreTestEventAddressLifetime && id <= SemaphoreTestNameOwner))
    {
        return SemaphoreTests::runCase(id) ? 0 : 77;
    }
    return 77;
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
        if (*next == ']' && next + 1 == line.data() + line.size())
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
    if (line.find('\0') != std::string::npos || position == std::string::npos || line.find(key, position + key.size()) != std::string::npos)
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

/** 校验子进程确实执行了指定用例，并在同一份库上完整结束。
@param [in] input 子进程独占日志或内存夹具
@param [in] id 请求用例编号
@param [in] code 进程退出码，0通过，77未运行
@param [in] identity 期望的库摘要及路径
@return 首行、结束标记和末行身份均匹配返回true
*/
static bool CompletedCase(std::istream& input, int32_t id, int32_t code, const std::string& identity)
{
    std::string line, previous, last;
    if (!std::getline(input, line) || line != "IDENTITY_BEGIN " + identity)
    {
        return false;
    }
    while (std::getline(input, line))
    {
        previous.swap(last);
        last.swap(line);
    }
    const std::string result = code == 0 ? "PASS id=" : "NOT_RUN id=";
    return input.eof() && !input.bad() && (code == 0 || code == 77) &&
        previous == result + Number(id) + " exit=" + Number(code) && last == "IDENTITY_END " + identity;
}

bool SemaphoreTest::checkSamples()
{
    if (CsvField("a,\"b\"\nc") != "\"a,\"\"b\"\"\nc\"")
    {
        return false;
    }
    const std::string complete = "IDENTITY_BEGIN unit\nPASS id=68 exit=0\nIDENTITY_END unit\n";
    std::istringstream valid(complete), wrongId(complete), wrongIdentity(complete), wrongExit(complete);
    std::istringstream truncated("IDENTITY_BEGIN unit\nPASS id=68 exit=0\n");
    std::istringstream skipped("IDENTITY_BEGIN unit\nNOT_RUN id=68 exit=77\nIDENTITY_END unit\n");
    if (!CompletedCase(valid, 68, 0, "unit") || CompletedCase(wrongId, 67, 0, "unit") ||
        CompletedCase(wrongIdentity, 68, 0, "other") || CompletedCase(wrongExit, 68, 77, "unit") ||
        CompletedCase(truncated, 68, 0, "unit") || !CompletedCase(skipped, 68, 77, "unit"))
    {
        return false;
    }
    std::vector<double> values;
    if (!ReadSamples("SAMPLES probe [3,1,2,4]", "probe", values) || values.size() != 4 ||
        values[0] != 3 || values[1] != 1 || values[2] != 2 || values[3] != 4)
    {
        return false;
    }
    const std::string invalid[] = {std::string("SAMPLES probe [1]\0x", 19), "SAMPLES probe []", "SAMPLES probe [NaN]", "SAMPLES probe [1,]", "SAMPLES probe [1",
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
    const std::string badFields[] = {std::string("{\"value\":1}\0x", 13), "{}", "{\"value\":NaN}", "{\"value\":-1}", "{\"value\":1e999}",
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

/** 以固定摘要验证执行器读文件、跨块填充和字节顺序。
@param [in] output 本次独占报告目录
@return 全部已知消息的文件摘要与预期一致
*/
static bool CheckFileHash(const std::string& output)
{
    const size_t lengths[] = {0, 3, 55, 56, 64};
    const char* expected[] = {
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318",
        "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a",
        "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"};
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i)
    {
        const std::string path = output + "/harness-sha-" + Number(static_cast<int32_t>(lengths[i]));
        std::ofstream file;
        OpenReport(file, path);
        file << (lengths[i] == 3 ? "abc" : std::string(lengths[i], 'a'));
        file.close();
        if (SemaphoreTestProcess::reserveReport(path) || SemaphoreTestProcess::hash(path) != expected[i])
        {
            return false;
        }
    }
    return true;
}

/** 独立核对单次当前性能日志的产物身份、固定工作量、分位数及原始样本
@param [in] id 性能用例编号
@param [in] log 本次子进程日志
@param [in] identity 父进程加载的同批库身份
@return 所有必需指标都存在且可由原始样本复算时为true
*/
static bool ValidPerformanceLog(int32_t id, const std::string& log, const std::string& identity)
{
    int32_t code = 0;
    const int32_t latencyCount = 8, rateCount = 6, scalarCount = 13;
    const char* names[] = {"cold_publish_to_observe", "ipc_roundtrip", "warm_roundtrip",
        "event_one_observed", "event_all_8_observed", "event_pool_cycle", "event_pool_batch_8", "event_pool_batch_64"};
    const uint32_t sampleCounts[] = {300, 2000, 20000, 200, 200, 300, 300, 300};
    const char* rates[] = {"bulk", "spsc_1p1c", "mpsc_4p1c", "mpmc_4p4c", "mpmc_8p8c", "mpmc_16p16c"};
    const char* scalarMetrics[] = {"idle", "idle", "lifecycle", "lifecycle", "event_one_boundary",
        "event_all_boundary", "spsc_1p1c", "mpsc_4p1c", "mpmc_4p4c", "mpmc_8p8c", "mpmc_16p16c",
        "lifecycle_resources", "lifecycle_resources"};
    const char* scalarFields[] = {"wall_ms", "cpu_ms", "cycle_us", "object_bytes", "rescue_timeouts",
        "rescue_timeouts", "cpu_ms", "cpu_ms", "cpu_ms", "cpu_ms", "cpu_ms", "handles_before", "handles_after"};
    // 独立性能入口复用55号的同一工作负载及完整判定，只选择本次实际执行的指标
    const bool latencyActive[] = {id == 55 || id == 66, id == 55 || id == 66 || id == 74,
        id == 55 || id == 66, id == 55 || id == 72, id == 55 || id == 72, id == 79, id == 79, id == 79};
    const bool ratesActive = id == 55 || id == 73;
    const bool scalarActive[] = {id == 55 || id == 66, id == 55 || id == 66, ratesActive, ratesActive,
        id == 55 || id == 72, id == 55 || id == 72, ratesActive, ratesActive, ratesActive, ratesActive, ratesActive,
        ratesActive, ratesActive};
    std::ifstream input(log.c_str());
    std::string libraryBegin, libraryEnd;
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
        if (line.compare(0, 15, "IDENTITY_BEGIN ") == 0)
        {
            if (!libraryBegin.empty())
            {
                code = 125;
            }
            libraryBegin = line.substr(15);
        }
        if (line.compare(0, 13, "IDENTITY_END ") == 0)
        {
            if (!libraryEnd.empty())
            {
                code = 125;
            }
            libraryEnd = line.substr(13);
        }
        for (int32_t metric = 0; metric < latencyCount; ++metric)
        {
            ReadSamples(line, names[metric], raw[metric]);
        }
        if (line.compare(0, 7, "METRIC ") == 0 &&
            (line.find('\0') != std::string::npos || line.find('}') != line.size() - 1 || line.back() != '}'))
        {
            return false;
        }
        char name[128] = {};
        if (std::sscanf(line.c_str(), "METRIC {\"name\":\"%127[^\"]\"", name) == 1)
        {
            if (std::strcmp(name, "lifecycle") == 0 && ReadMetricNumber(line, "iterations") != 10000000)
            {
                code = 125;
            }
            for (int32_t metric = 0; metric < scalarCount; ++metric)
            {
                if (std::strcmp(name, scalarMetrics[metric]) == 0)
                {
                    const double value = ReadMetricNumber(line, scalarFields[metric]);
                    if (scalarSeen[metric] || ((metric == 4 || metric == 5) && value != 0))
                    {
                        code = 125;
                    }
                    scalarSeen[metric] = true;
                }
            }
        }
        uint32_t samples = 0;
        double p50 = 0, p95 = 0, p99 = 0, maximum = 0;
        uint32_t permits = 0;
        char endOfMetric = 0;
        double rate = 0;
        if (std::sscanf(line.c_str(),
                "METRIC {\"name\":\"%127[^\"]\",\"permits\":%u,\"permits_per_second\":%lf", name, &permits,
                &rate) == 3)
        {
            for (int32_t metric = 0; metric < rateCount; ++metric)
            {
                if (std::strcmp(name, rates[metric]) == 0)
                {
                    if (rateSeen[metric] || permits != (metric == 0 ? 20000000 : 4000000) ||
                        !std::isfinite(rate) || rate <= 0)
                    {
                        code = 125;
                    }
                    rateSeen[metric] = true;
                    const double elapsed = ReadMetricNumber(line, "elapsed_ms");
                    if (elapsed <= 0 || std::fabs(rate * elapsed / 1000.0 / permits - 1) > 0.01)
                    {
                        code = 125;
                    }
                }
            }
        }
        if (std::sscanf(line.c_str(),
                "METRIC "
                "{\"name\":\"%127[^\"]\",\"samples\":%u,\"p50_us\":%lf,\"p95_us\":%lf,\"p99_us\":%lf,"
                "\"max_us\":%lf%c",
                name, &samples, &p50, &p95, &p99, &maximum, &endOfMetric) != 7 || endOfMetric != '}')
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
            }
        }
    }
    const bool missingLatency = std::find(seen, seen + latencyCount, false) != seen + latencyCount;
    const bool missingRate = std::find(rateSeen, rateSeen + rateCount, false) != rateSeen + rateCount;
    const bool missingScalar = std::find(scalarSeen, scalarSeen + scalarCount, false) != scalarSeen + scalarCount;
    return code == 0 && !missingLatency && !missingRate && !missingScalar && !input.bad() &&
        libraryBegin == identity && libraryEnd == identity;
}

/** 用可手算的原始序列检查性能日志验证器，避免错误分位数或短缺样本冒充有效数据
@param [in] output 当前执行器批次的专属目录
@return 正常日志通过，错误分位数和短缺样本均被拒绝时返回true
*/
static bool CheckPerformanceLogReader(const std::string& output)
{
    for (int32_t variant = 0; variant < 6; ++variant)
    {
        const std::string path = output + "/harness-metrics-" + Number(variant) + ".log";
        std::ofstream file;
        OpenReport(file, path);
        file << "IDENTITY_BEGIN fixture\nSAMPLES ipc_roundtrip [";
        const int32_t last = variant == 2 ? 1 : 0;
        for (int32_t sample = 1999; sample >= last; --sample)
        {
            file << (sample == 1999 ? "" : ",") << sample;
        }
        file << "]\nMETRIC {\"name\":\"ipc_roundtrip\",\"samples\":2000,\"p50_us\":"
             << (variant == 1 ? 1000 : 999) << ",\"p95_us\":1899,\"p99_us\":1979,\"max_us\":1999"
             << (variant == 3 ? "" : variant == 4 ? "}junk" : variant == 5 ? "junk}" : "}") << "\nIDENTITY_END fixture\n";
        file.close();
        if (ValidPerformanceLog(74, path, "fixture") != (variant == 0))
        {
            return false;
        }
    }
    return true;
}

int32_t SemaphoreTest::batch(
    const std::vector<int32_t>& selected, const std::string& output, bool combined, bool invalidIdentity)
{
#ifdef SEMAPHORE_TEST_EXTERNAL_LAUNCH
    std::fprintf(stderr, "NOT_RUN mobile host must invoke --execute for each case and collect its exit code\n");
    return 77;
#endif
    if (!SemaphoreTestProcess::makeDirectory(output))
    {
        throw std::runtime_error("Cannot create output directory; create its parent first");
    }
    const std::string executable = SemaphoreTestProcess::executable();
    const std::string library = SemaphoreTestProcess::library();
    const std::string libraryHash = SemaphoreTestProcess::hash(library);
    std::string identity = SemaphoreTestProcess::hash(executable);
    if (invalidIdentity)
    {
        // 只用于执行器反向自检：改变预期身份，不改写正在运行的程序文件。
        identity += "-intentional-mismatch";
    }
    std::ofstream report;
    OpenReport(report, output + "/cases.csv");
    report << "id,name,expected,type,result,exit,elapsed_ms,log,executed\n";
    int32_t passed = 0, failed = 0, skipped = 0, failedToStart = 0, executedStress = 0;
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
        bool processStarted = false;
        int32_t code = SemaphoreTestProcess::run(executable, arguments, log, item->m_stress ? 180000 : 90000, &processStarted);
        if (code == 0 || code == 77)
        {
            std::ifstream completed(log.c_str());
            if (!CompletedCase(completed, item->m_id, code, libraryHash + " " + library))
            {
                code = 125;
            }
        }
#ifndef SEMAPHORE_PERF_ALLOCATION
        if (code == 0 && (item->m_id == 55 || item->m_id == 66 || item->m_id == 72 || item->m_id == 73 ||
            item->m_id == 74 || item->m_id == 79))
        {
            try
            {
                if (!ValidPerformanceLog(item->m_id, log, libraryHash + " " + library))
                {
                    code = 125;
                }
            }
            catch (const std::exception& error)
            {
                std::fprintf(stderr, "Invalid performance log: %s\n", error.what());
                code = 125;
            }
        }
#endif
        if (item->m_id == SemaphoreTestHarness && code == 0)
        {
            if (!CheckFileHash(output) || !CheckPerformanceLogReader(output))
            {
                code = 125;
            }
            arguments.clear();
            arguments.push_back("--self-fail");
            bool fixtureStarted = false;
            const std::string failureLog = output + "/harness-failure.log";
            if (SemaphoreTestProcess::run(executable, arguments, failureLog, 5000, &fixtureStarted) != 19 || !fixtureStarted)
            {
                code = 125;
            }
#ifdef _WIN32
            // 日志已存在，启动前即被拒绝；不能把上一次的退出或旧日志算进实际执行数。
            if (SemaphoreTestProcess::run(executable, arguments, failureLog, 5000, &fixtureStarted) != 125 || fixtureStarted)
            {
                code = 125;
            }
#endif
            arguments[0] = "--self-sleep";
            if (SemaphoreTestProcess::run(executable, arguments, output + "/harness-timeout.log", 20) != 124)
            {
                code = 125;
            }
            arguments[0] = "--self-identity-mismatch";
            arguments.push_back(output + "/harness-identity-results");
            if (SemaphoreTestProcess::run(executable, arguments, output + "/harness-identity.log", 15000) != 1)
            {
                code = 125;
            }
            std::ifstream identityReport((output + "/harness-identity-results/summary.txt").c_str());
            bool batchError = false, batchFailed = false, executed = false, instant = false, stress = false;
            std::string identityLine;
            while (std::getline(identityReport, identityLine))
            {
                batchError = batchError || identityLine == "batch_errors=1";
                batchFailed = batchFailed || identityLine == "result=FAIL";
                executed = executed || identityLine == "executed=1";
                instant = instant || identityLine == "executed_instant=1";
                stress = stress || identityLine == "executed_stress=0";
            }
            if (!batchError || !batchFailed || !executed || !instant || !stress || identityReport.bad())
            {
                code = 125;
            }
            arguments.resize(1);
            arguments[0] = "--self-expire-gate";
            const std::string gateLog = output + "/harness-gate-expired.log";
            if (SemaphoreTestProcess::run(executable, arguments, gateLog, 15000) != 1)
            {
                code = 125;
            }
            std::ifstream gateReport(gateLog.c_str());
            bool expired = false;
            std::string gateLine;
            while (std::getline(gateReport, gateLine))
            {
                expired = expired || gateLine == "WORKER: pause gate expired before release";
            }
            if (!expired || gateReport.bad())
            {
                code = 125;
            }
#ifdef _WIN32
            for (int32_t count = 1; count <= 8; count += 7)
            {
                arguments[0] = count == 1 ? "--self-drop-event" : "--self-drop-broadcast";
                const std::string negativeLog = output + "/harness-dropped-" + Number(count) + ".log";
                if (SemaphoreTestProcess::run(executable, arguments, negativeLog, 15000) != 1)
                {
                    code = 125;
                }
                std::ifstream negative(negativeLog.c_str());
                bool missingNotification = false, samples = false;
                const std::string expected = "FAIL id=" + Number(count == 1 ? -72 : -73) + " event reply";
                std::string line;
                while (std::getline(negative, line))
                {
                    missingNotification = missingNotification || line == expected;
                    samples = samples || line.compare(0, 8, "SAMPLES ") == 0;
                }
                if (!missingNotification || samples || negative.bad())
                {
                    code = 125; // 其他失败不能冒充漏通知检测成功，也不能保留失效样本。
                }
            }
#endif
        }
        const char* result = code == 0 ? "PASS" : (code == 77 ? "NOT_RUN" : "FAIL");
        passed += code == 0 ? 1 : 0;
        skipped += code == 77 ? 1 : 0;
        failed += code != 0 && code != 77 ? 1 : 0;
        const bool executed = processStarted && code != 77;
        failedToStart += !processStarted && code != 77 ? 1 : 0;
        executedStress += executed && item->m_stress ? 1 : 0;
        report << item->m_id << ',' << CsvField(Utf8(item->m_name)) << ',' << CsvField(Utf8(item->m_expected)) << ','
               << (item->m_stress ? "stress" : "instant") << ',' << result << ',' << code << ','
               << SemaphoreTestProcess::now() - before << ',' << CsvField(SemaphoreTestProcess::reportPath(log)) << ',' << (executed ? 1 : 0) << "\n";
        report.flush();
        std::printf("%s id=%d exit=%d\n", result, item->m_id, code);
        std::fflush(stdout);
    }
    report.close();
    // 批次身份错误独立于用例失败计数；必须在写最终状态前完成核对。
    const bool identityChanged = SemaphoreTestProcess::hash(executable) != identity || SemaphoreTestProcess::hash(library) != libraryHash;
    const int32_t result = failed != 0 || identityChanged ? 1 : (passed == 0 ? 77 : 0);
    std::ofstream summary;
    OpenReport(summary, output + "/summary.txt");
    summary << "executable=" << SemaphoreTestProcess::reportPath(executable) << "\nsha256=" << identity << "\n";
    summary << "library=" << SemaphoreTestProcess::reportPath(library) << "\nlibrary_sha256=" << libraryHash << "\n";
    summary << "unique_ids=" << selected.size() << "\nconfiguration_runs=" << selected.size() << "\npassed=" << passed
            << "\nfailed=" << failed << "\nnot_run=" << skipped
            << "\nfailed_to_start=" << failedToStart << "\nexecuted=" << passed + failed - failedToStart
            << "\nexecuted_instant=" << passed + failed - failedToStart - executedStress
            << "\nexecuted_stress=" << executedStress
            << "\ncancelled=0\nbatch_errors=" << (identityChanged ? 1 : 0)
            << "\nresult=" << (result == 0 ? "PASS" : (result == 77 ? "NOT_RUN" : "FAIL"))
            << "\nelapsed_ms=" << SemaphoreTestProcess::now() - started << "\n";
    if (identityChanged)
    {
        summary << "FAIL executable or library identity mismatch\n";
    }
    summary.close();
    return result;
}

int32_t SemaphoreTest::dispatch(int32_t argc, char** argv)
{
    if (argc == 3 && std::strcmp(argv[1], "--self-identity-mismatch") == 0)
    {
        const std::vector<int32_t> selected(1, SemaphoreTestCount);
        return batch(selected, argv[2], false, true);
    }
    if (argc == 2 && std::strcmp(argv[1], "--self-expire-gate") == 0)
    {
        // 故意不放行，超时必须被工作线程异常边界及主入口识别为失败。
        SemaphoreTestGate gate(0);
        SemaphoreTestThreads threads;
        /** 故意等待未释放的门，验证超时异常由线程边界记录并由join上报
        */
        threads.start([&]()
        {
            gate.hit(0);
        });
        SemaphoreTests::check(gate.reached(), "expiry self-check reached the gate");
        threads.join();
        return 0;
    }
#ifdef _WIN32
    if (argc == 2 && (std::strcmp(argv[1], "--self-drop-event") == 0 ||
        std::strcmp(argv[1], "--self-drop-broadcast") == 0))
    {
        return SemaphorePerformance::run(std::strcmp(argv[1], "--self-drop-event") == 0 ? -72 : -73, false);
    }
#endif
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
    if (argc == 5 && std::strcmp(argv[1], "--ipc-session") == 0)
    {
        const int32_t id = std::strcmp(argv[2], "41") == 0 ? 41 : (std::strcmp(argv[2], "60") == 0 ? 60 : 0);
        return SemaphoreTests::session(id, argv[3], argv[4]);
    }
    if (argc == 3 && (std::strcmp(argv[1], "--peer") == 0 || std::strcmp(argv[1], "--exchange-peer") == 0))
    {
        return SemaphoreTests::peer(argv[2], std::strcmp(argv[1], "--exchange-peer") == 0);
    }
    std::string mode = "instant", selection, output;
#ifndef _WIN32
    bool createBatchDirectory = false;
#endif
    bool combined = false;
    int32_t direct = 0;
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
#ifndef _WIN32
                createBatchDirectory = false;
#endif
            }
#ifndef _WIN32
            else if (argument == "--output-parent")
            {
                // CTest 可以反复及并发执行，每个批次独占其报告目录。
                output = value + "/SemaphoreTest-XXXXXX";
                createBatchDirectory = true;
            }
#endif
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
        return 2;
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
        for (int32_t phase = 0; phase < 2; ++phase)
        {
            const bool stress = phase != 0;
            if (mode != "all" && (mode == "stress") != stress)
            {
                continue;
            }
            for (size_t i = 0; i < sizeof(kCases) / sizeof(kCases[0]); ++i)
            {
                if (kCases[i].m_stress == stress)
                {
                    selected.push_back(kCases[i].m_id);
                }
            }
        }
    }
    else
    {
        return 2;
    }
#ifndef _WIN32
    if (createBatchDirectory)
    {
        std::vector<char> pattern(output.begin(), output.end());
        pattern.push_back(0);
        if (::mkdtemp(&pattern[0]) == nullptr)
        {
            throw std::runtime_error("Cannot create private CTest report directory");
        }
        output.assign(&pattern[0]);
    }
#endif
    return batch(selected, output, combined);
}

/** 测试主入口，所有可传播的异常都形成失败退出
@param [in] argc 参数数量
@param [in] argv 参数数组
@return 非零表示失败或明确未执行
*/
int32_t SemaphoreTest::run(int32_t argc, char** argv)
{
    try
    {
        const std::string library = SemaphoreTestProcess::library();
        const std::string identity = SemaphoreTestProcess::hash(library);
        std::printf("IDENTITY_BEGIN %s %s\n", identity.c_str(), library.c_str());
        std::fflush(stdout);
        const int32_t result = dispatch(argc, argv);
        const std::string after = SemaphoreTestProcess::hash(library);
        std::printf("IDENTITY_END %s %s\n", after.c_str(), library.c_str());
        return after == identity ? result : 125;
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

#ifndef SEMAPHORE_TEST_HOST_LIBRARY
/** 独立测试程序入口，与移动宿主共用异常处理及文件身份核验。
@param [in] argc 参数数量
@param [in] argv 参数数组
@return 用例、批次或参数的退出状态
*/
int main(int argc, char** argv)
{
    return SemaphoreTest::run(argc, argv);
}
#endif