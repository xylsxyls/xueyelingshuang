#include "CTaskTestAutomation.h"
#include "CTaskThreadBenchmark.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

/** 严格解析有限实数，不接受部分数字或 NaN
@param [in] text 完整字段
@return 有限数值，非法值抛测试错误
*/
static double Number(const std::string& text)
{
    std::istringstream input(text);
    double value = 0;
    char extra = 0;
    bbCheck((input >> value) && !(input >> extra) && std::isfinite(value), "invalid finite number: " + text);
    return value;
}

/** 解析有上下界的正整数
@param [in] text 完整十进制参数
@param [in] maximum 允许的最大值
@return 经检查的整数
*/
static int32_t Positive(const std::string& text, int32_t maximum)
{
    bbCheck(!text.empty() && text.find_first_not_of("0123456789") == std::string::npos, "invalid integer");
    const double value = Number(text);
    bbCheck(value >= 1 && value <= maximum, "integer outside allowed range");
    return static_cast<int32_t>(value);
}

/** 在建立批次前校验任务规模，保证延迟矩阵能够覆盖全部级别
@param [in] phase 已识别的矩阵阶段
@param [in] tasks 全体生产者任务总数
*/
static void ValidateMatrixTasks(const std::string& phase, int32_t tasks)
{
    bbCheck(tasks >= 1024 && tasks % 32 == 0, "tasks must be >=1024 and divisible by32");
    bbCheck(phase != "auxiliary" || tasks <= 10000000, "auxiliary total must be <=10000000");
    // 最多32个生产者，每1009项采样一次；十级完整轮转需要每人至少10个样本。
    const int32_t minimumLatencyTasks = (9 * 1009 + 1) * 32;
    bbCheck((phase != "latency" && phase != "prepared") || tasks >= minimumLatencyTasks,
        "latency/prepared matrices require >=290624 tasks to sample every priority");
}

/** 拼接批次目录和本工具生成的文件名
@param [in] directory 批次目录
@param [in] name 不含路径跳转的文件名
@return 输出路径
*/
static std::string Join(const std::string& directory, const std::string& name)
{
    return directory + "/" + name;
}

/** 取得可重复的文件内容指纹，明确采用 FNV-1a 而非安全签名
@param [in] path 已存在的程序或源码文件
@return 十六进制 FNV-1a-64 与字节长度
*/
static std::string Fingerprint(const std::string& path)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    bbCheck(input.good(), "cannot fingerprint " + path);
    uint64_t value = UINT64_C(14695981039346656037), bytes = 0;
    char buffer[16384];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() != 0)
    {
        const std::streamsize count = input.gcount();
        bytes += static_cast<uint64_t>(count);
        for (std::streamsize i = 0; i < count; ++i)
        {
            value = (value ^ static_cast<unsigned char>(buffer[i])) * UINT64_C(1099511628211);
        }
    }
    bbCheck(input.eof(), "fingerprint read failed");
    std::ostringstream output;
    output << "fnv1a64=" << std::hex << value << ";bytes=" << std::dec << bytes;
    return output.str();
}

/** 在规范化源码中替换唯一上下文，拒绝悄悄漏掉测试交错
@param [in] source 完整源码
@param [in] before 唯一原文
@param [in] after 替换文本
@return 替换后文本
*/
static std::string ReplaceOnce(const std::string& source, const std::string& before, const std::string& after)
{
    const size_t found = source.find(before);
    bbCheck(found != std::string::npos && source.find(before, found + before.size()) == std::string::npos,
        "review changed Queue hook context: " + before);
    std::string result = source;
    result.replace(found, before.size(), after);
    return result;
}

/** 规范化测试私有源码的换行
@param [in] text 完整 UTF8 文本
@return 去除 CR 后的文本，BOM 保持
*/
static std::string Plain(std::string text)
{
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    return text;
}

void CTaskTestAutomation::directory(const std::string& path)
{
    bbCheck(!path.empty(), "empty output directory");
#ifdef _WIN32
    bbCheck(CreateDirectoryA(path.c_str(), nullptr) != FALSE, "output must be a new directory with an existing parent: " + path);
#else
    bbCheck(mkdir(path.c_str(), 0700) == 0, "output must be a new directory with an existing parent: " + path);
#endif
}

void CTaskTestAutomation::execute(const std::string& program, const std::vector<std::string>& arguments,
    const std::string& log, int32_t seconds)
{
    BlackBoxChild child;
    child.startProgram(program, arguments, log);
    const int32_t code = child.wait(seconds * 1000);
    if (code != 0)
    {
        std::cerr << bbRead(log) << std::endl;
    }
    bbCheck(code == 0, "child failed exit=" + bbNumber(code) + "; log=" + log);
}

bool CTaskTestAutomation::retainCompleteGroup(size_t begin, size_t expected, std::vector<std::string>& reports,
    std::vector<std::string>& labels)
{
    bbCheck(reports.size() == labels.size() && begin <= reports.size() && expected != 0,
        "invalid measurement group checkpoint");
    if (reports.size() - begin == expected)
    {
        return true;
    }
    reports.resize(begin);
    labels.resize(begin);
    return false;
}

std::vector<std::vector<std::string>> CTaskTestAutomation::readCsv(const std::string& path)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    bbCheck(input.good(), "cannot open CSV " + path);
    std::vector<std::vector<std::string>> result;
    std::string line;
    while (std::getline(input, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
        {
            line.resize(line.size() - 1);
        }
        if (result.empty() && line.compare(0, 3, "\xef\xbb\xbf") == 0)
        {
            line.erase(0, 3);
        }
        bbCheck(!line.empty() && line.find('"') == std::string::npos, "expected unquoted numeric workload CSV");
        std::vector<std::string> row;
        size_t begin = 0, end = 0;
        do
        {
            end = line.find(',', begin);
            row.push_back(line.substr(begin, end == std::string::npos ? end : end - begin));
            begin = end + 1;
        } while (end != std::string::npos);
        bbCheck(result.empty() || row.size() == result[0].size(), "CSV field count mismatch");
        result.push_back(row);
    }
    bbCheck(input.eof() && result.size() >= 2, "CSV is incomplete or empty");
    std::set<std::string> columns(result[0].begin(), result[0].end());
    bbCheck(columns.size() == result[0].size() && columns.count("") == 0, "duplicate/empty CSV header");
    return result;
}

double CTaskTestAutomation::median(std::vector<double> values)
{
    bbCheck(!values.empty(), "cannot summarize zero runs");
    for (size_t i = 0; i < values.size(); ++i)
    {
        bbCheck(std::isfinite(values[i]), "nonfinite measurement");
    }
    std::sort(values.begin(), values.end());
    const size_t middle = values.size() / 2;
    return values.size() % 2 ? values[middle] : values[middle - 1] / 2 + values[middle] / 2;
}

void CTaskTestAutomation::summarize(const std::string& output, const std::vector<std::string>& reports,
    const std::vector<std::string>& labels)
{
    bbCheck(reports.size() == labels.size() && !reports.empty(), "missing report labels");
    const std::set<std::string> expectedLabels(labels.begin(), labels.end());
    std::map<std::string, std::map<std::string, std::vector<double>>> groups;
    for (size_t file = 0; file < reports.size(); ++file)
    {
        const std::vector<std::vector<std::string>> csv = readCsv(reports[file]);
        for (size_t row = 1; row < csv.size(); ++row)
        {
            std::map<std::string, std::string> cells;
            for (size_t col = 0; col < csv[0].size(); ++col)
            {
                cells[csv[0][col]] = csv[row][col];
            }
            bbCheck(cells.count("errors") && Number(cells["errors"]) == 0, "measurement has errors");
            if (cells.count("accepted"))
            {
                bbCheck(Number(cells["accepted"]) == Number(cells["completed"]) &&
                    Number(cells["accepted"]) == Number(cells["total"]), "incomplete workload");
            }
            else if (cells.count("submitted"))
            {
                bbCheck(Number(cells["submitted"]) == Number(cells["consumed"]), "fairness lost accepted work");
            }
            const std::string key = cells["mode"] + "," + cells["producers"] + "," + cells["levels"] + "," +
                cells["cpus"] + "," + cells["queue_producers"] + "," + cells["total"] + "," +
                cells["count"] + "," + cells["work"] + "," + cells["task_objects"];
            for (size_t col = 0; col < csv[0].size(); ++col)
            {
                const std::string& name = csv[0][col];
                if (name.find("ops_s") != std::string::npos || name.find("_us") != std::string::npos ||
                    name.find("_ms") != std::string::npos || name.find("_mib") != std::string::npos ||
                    name.find("_mb") != std::string::npos || name.find("handles_") == 0 || name == "submitted" || name == "retries")
                {
                    groups[key + "," + name][labels[file]].push_back(Number(csv[row][col]));
                }
            }
            if (cells["mode"].find("latency") == 0)
            {
                const std::vector<std::vector<std::string>> latency = readCsv(reports[file] + ".latency.csv");
                const std::vector<std::string> expected = {"level", "samples", "post_p50_us", "post_p99_us", "response_p50_us", "response_p99_us"};
                bbCheck(latency[0] == expected, "latency schema changed");
                bbCheck(latency.size() == static_cast<size_t>(Number(cells["levels"])) + 2, "missing latency priority group");
                for (size_t item = 1; item < latency.size(); ++item)
                {
                    bbCheck(Number(latency[item][0]) == item - 1 && Number(latency[item][1]) > 0, "invalid latency samples");
                    for (size_t metric = 2; metric < expected.size(); ++metric)
                    {
                        groups[key + "," + expected[metric] + "_level" + latency[item][0]][labels[file]].push_back(Number(latency[item][metric]));
                    }
                }
            }
        }
    }
    bbCheck(!groups.empty(), "no comparable measurements");
    std::ostringstream summary;
    summary << "mode,producers,levels,cpus,queue_producers,total,count,work,task_objects,metric,label,samples,median,baseline_median,change_percent\n";
    summary << std::fixed << std::setprecision(6);
    for (std::map<std::string, std::map<std::string, std::vector<double>>>::const_iterator group = groups.begin(); group != groups.end(); ++group)
    {
        const std::map<std::string, std::vector<double>>& runs = group->second;
        // 某版本整项缺失时，不能把只有基线的指标也当成完整比较。
        bbCheck(runs.size() == expectedLabels.size(), "missing same-scenario comparison version");
        const std::map<std::string, std::vector<double>>::const_iterator base = runs.find(labels[0]);
        bbCheck(base != runs.end(), "missing same-scenario baseline");
        const double baseline = median(base->second);
        for (std::map<std::string, std::vector<double>>::const_iterator version = runs.begin(); version != runs.end(); ++version)
        {
            bbCheck(version->second.size() == base->second.size(), "unbalanced comparison sample count");
            const double measured = median(version->second);
            summary << group->first << ',' << version->first << ',' << version->second.size() << ',' << measured << ',' << baseline << ',';
            if (baseline != 0)
            {
                summary << (measured / baseline - 1) * 100;
            }
            else
            {
                summary << "NA";
            }
            summary << '\n';
        }
    }
    bbWrite(Join(output, "summary.csv"), summary.str());
}

void CTaskTestAutomation::fairness(size_t index)
{
    const int32_t scenarios[][4] = {
        {4,1,0,64}, {4,10,0,64}, {8,1,0,64}, {8,10,0,64},
        {16,1,0,64}, {16,10,0,64}, {32,1,0,64}, {32,10,0,64},
        {32,1,4,64}, {32,10,4,64}, {64,1,4,64}, {64,10,4,64}, {8,1,1,64}, {8,10,1,64},
        {8,10,4,1}, {32,1,4,1}, {32,10,4,1}
    };
    bbCheck(index < 17, "unknown fairness scenario");
    const std::string report = bbTempPath("-fairness.csv");
    const std::vector<std::string> args = {"--fairness", bbNumber(scenarios[index][0]), bbNumber(scenarios[index][1]),
        "10", bbNumber(scenarios[index][2]), report, bbNumber(scenarios[index][3])};
    execute(bbExecutable(), args, report + ".log", 120);
    const std::vector<std::vector<std::string>> csv = readCsv(report);
    bbCheck(csv.size() == static_cast<size_t>(scenarios[index][0]) + 1, "missing producer fairness row");
}

void CTaskTestAutomation::selfTest()
{
    {
        CTaskBenchmarkState state(2, 4, 2, 0);
        const size_t order[] = {1, 5, 3, 7, 0, 4, 2, 6};
        for (size_t i = 0; i < sizeof(order) / sizeof(order[0]); ++i)
        {
            CTaskBenchmarkItem item(state, order[i] / 4, order[i] % 4, 0);
            item.doTask();
        }
        bbCheck(state.m_errors.load() == 0 && state.m_completed.load() == 8,
            "benchmark oracle rejected independent priority streams");
    }
    {
        CTaskBenchmarkState state(1, 4, 2, 0);
        const size_t order[] = {2, 0, 1, 3};
        for (size_t i = 0; i < sizeof(order) / sizeof(order[0]); ++i)
        {
            CTaskBenchmarkItem item(state, 0, order[i], 0);
            item.doTask();
        }
        bbCheck(state.m_errors.load() == 2 && state.m_completed.load() == 4,
            "benchmark oracle missed same-priority FIFO violations");
    }
    {
        CTaskBenchmarkState state(1, 4, 2, 0);
        state.m_stop.store(true);
        CTaskBenchmarkProducer producer(state, std::shared_ptr<CTaskThread>(), 0, true);
        producer.doTask();
        bbCheck(state.m_errors.load() == 1 && state.m_accepted.load() == 0 && state.m_completed.load() == 0,
            "cancelled benchmark producer counted work that never ran");
    }
    bbCheck(median({3,1,2}) == 2 && median({10,2,4,6}) == 5, "median mismatch");
    ValidateMatrixTasks("throughput", 1024);
    ValidateMatrixTasks("auxiliary", 1024);
    ValidateMatrixTasks("latency", 290624);
    ValidateMatrixTasks("prepared", 290624);
    const char* sampledPhases[] = {"latency", "prepared"};
    for (size_t i = 0; i < 2; ++i)
    {
        bool rejected = false;
        try
        {
            ValidateMatrixTasks(sampledPhases[i], 290592);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "matrix accepted too few samples for all priorities");
    }
    const std::string csv = bbTempPath("-automation.csv");
    bbWrite(csv, "mode,errors,accepted,completed,total,submit_ops_s\nthroughput,0,10,10,10,20\n");
    bbCheck(readCsv(csv).size() == 2, "valid CSV rejected");
    const char* invalid[] = {"a,a\n1,2\n", "a,b\n1\n", "a,b\n", "a,b\n1,2,3\n"};
    for (size_t i = 0; i < 4; ++i)
    {
        bbWrite(csv, invalid[i]);
        bool rejected = false;
        try
        {
            readCsv(csv);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "invalid CSV accepted");
    }
    std::remove(csv.c_str());
    const std::string output = bbTempPath("-automation-summary");
    directory(output);
    const std::string base = Join(output, "base.csv"), candidate = Join(output, "candidate.csv");
    const std::string header = "mode,producers,levels,errors,accepted,completed,total,submit_ops_s\n";
    bbWrite(base, header + "throughput,4,1,0,10,10,10,20\n");
    bbWrite(candidate, header + "throughput,4,1,0,10,10,10,30\n");
    summarize(output, {base, candidate}, {"baseline", "current"});
    const std::vector<std::vector<std::string>> summary = readCsv(Join(output, "summary.csv"));
    bbCheck(summary.size() == 3 && summary[2][10] == "current" && Number(summary[2][12]) == 30 &&
        Number(summary[2][13]) == 20 && Number(summary[2][14]) == 50, "comparison percentage mismatch");
    // 一方失败后保留此前完整组，撤回本轮孤立样本；失败轮不能改变配对中位数。
    for (size_t passed = 0; passed <= 2; ++passed)
    {
        std::vector<std::string> reports = {base, candidate};
        std::vector<std::string> labels = {"baseline", "current"};
        for (size_t i = 0; i < passed; ++i)
        {
            reports.push_back(i == 0 ? candidate : base);
            labels.push_back(i == 0 ? "current" : "baseline");
        }
        bbCheck(retainCompleteGroup(2, 2, reports, labels) == (passed == 2) &&
            reports.size() == (passed == 2 ? 4 : 2) && labels.size() == reports.size() &&
            reports[0] == base && reports[1] == candidate, "incomplete pair changed prior valid measurements");
        summarize(output, reports, labels);
        const std::vector<std::vector<std::string>> paired = readCsv(Join(output, "summary.csv"));
        bbCheck(Number(paired[2][11]) == (passed == 2 ? 2 : 1) && Number(paired[2][14]) == 50,
            "unpaired sample entered the performance comparison");
    }
    const char* badResults[] = {"throughput,4,1,1,10,10,10,30\n", "throughput,4,1,0,10,9,10,30\n",
        "throughput,4,1,0,10,10,10,nan\n", "throughput,4,1,0,20,20,20,30\n"};
    for (size_t i = 0; i < sizeof(badResults) / sizeof(badResults[0]); ++i)
    {
        bbWrite(candidate, header + badResults[i]);
        bool rejected = false;
        try
        {
            summarize(output, {base, candidate}, {"baseline", "current"});
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "failed benchmark was accepted by summary");
    }
    // 候选报告仍然合法，但缺少基线中的一个场景，必须拒绝不完整比较。
    bbWrite(base, header + "throughput,4,1,0,10,10,10,20\nthroughput,8,1,0,10,10,10,25\n");
    bbWrite(candidate, header + "throughput,4,1,0,10,10,10,30\n");
    bool missingScenarioRejected = false;
    try
    {
        summarize(output, {base, candidate}, {"baseline", "current"});
    }
    catch (const std::exception&)
    {
        missingScenarioRejected = true;
    }
    bbCheck(missingScenarioRejected, "missing candidate scenario was accepted by summary");
    // 空闲所有者不启动监控或创建子进程；随后启动的自有进程才能被处置。
    {
        BlackBoxChild inactive;
        bbCheck(inactive.pid() == 0, "inactive owner created a process");
    }
    BlackBoxChild child;
    child.start({"--automation-child"}, csv + ".child.log");
    bbCheck(child.pid() != 0 && child.poll() < 0, "late child not running");
    bool timedOut = false;
    try
    {
        child.wait(50);
    }
    catch (const std::exception&)
    {
        timedOut = true;
    }
    bbCheck(timedOut, "timeout path not observed");
    child.stop(1);
    bbCheck(child.poll() != 0, "owned child termination was not observed");
    child.stop(1);
    bbCheck(Fingerprint(bbExecutable()) == Fingerprint(bbExecutable()), "unstable executable identity");
    const char* selections[] = {"0", "1,,2", "2-1", "999999", "1,"};
    for (size_t i = 0; i < sizeof(selections) / sizeof(selections[0]); ++i)
    {
        BlackBoxChild rejected;
        rejected.start({"--tests", selections[i], "--report", csv + ".invalid.tsv"}, csv + ".invalid.log");
        bbCheck(rejected.wait(5000) != 0, "invalid selection accepted");
    }
    // 规模参数必须完整解析，不能把带后缀或溢出的数字静默变成另一组测试。
    const char* invalidCounts[] = {"4x", "-1", "18446744073709551616", ""};
    for (size_t i = 0; i < sizeof(invalidCounts) / sizeof(invalidCounts[0]); ++i)
    {
        BlackBoxChild fairness;
        fairness.start({"--fairness", invalidCounts[i], "1", "3", "0", csv + ".fairness.csv"}, csv + ".fairness.log");
        bbCheck(fairness.wait(5000) != 0, "invalid fairness count accepted");
        BlackBoxChild prototype;
        prototype.start({"--intrusive-prototype", "intrusive", invalidCounts[i], "3200", "1", csv + ".prototype.csv"},
            csv + ".prototype.log");
        bbCheck(prototype.wait(5000) != 0, "invalid prototype count accepted");
    }
}

void CTaskTestAutomation::queueRace(const std::string& root, const std::string& output, const std::string& compiler)
{
    directory(output);
    const std::string includes = Join(output, "include");
    directory(includes);
    directory(Join(includes, "LockFreeQueue"));
    const std::string source = Join(root, "src/LockFreeQueue/LockFreeQueue/src");
    std::vector<std::string> names;
#ifdef _WIN32
    WIN32_FIND_DATAA entry = {};
    HANDLE find = FindFirstFileA(Join(source, "*").c_str(), &entry);
    bbCheck(find != INVALID_HANDLE_VALUE, "Queue source not found");
    try
    {
        do
        {
            if ((entry.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0)
            {
                names.push_back(entry.cFileName);
            }
        } while (FindNextFileA(find, &entry));
    }
    catch (...)
    {
        FindClose(find);
        throw;
    }
    FindClose(find);
#else
    DIR* dir = opendir(source.c_str());
    bbCheck(dir != nullptr, "Queue source not found");
    try
    {
        for (dirent* entry = readdir(dir); entry != nullptr; entry = readdir(dir))
        {
            names.push_back(entry->d_name);
        }
    }
    catch (...)
    {
        closedir(dir);
        throw;
    }
    closedir(dir);
#endif
    std::ostringstream manifest;
    for (size_t i = 0; i < names.size(); ++i)
    {
        const std::string& name = names[i];
        const size_t dot = name.find_last_of('.');
        if (dot == std::string::npos || (name.substr(dot) != ".h" && name.substr(dot) != ".inl"))
        {
            continue;
        }
        const std::string original = bbRead(Join(source, name));
        bbCheck(!original.empty(), "empty Queue source");
        std::string patched = Plain(original);
        if (name == "LockFreeQueueArenaAllocation.inl")
        {
            const std::string before = "        if (available() != 0)\n";
            const std::string after = "        CTaskQueueResourceRace::afterEmptyScan();\n" + before;
            const std::string returned = "    block.m_externalReturns.store(block.m_externalReturns.load(std::memory_order_relaxed) + count, std::memory_order_release);\n";
            const std::string hooked = "    CTaskQueueResourceRace::afterReturnPublished();\n" + returned;
            patched = ReplaceOnce(patched, before, after);
            patched = ReplaceOnce(patched, returned, hooked);
            bbCheck(ReplaceOnce(ReplaceOnce(patched, after, before), hooked, returned) == Plain(original), "irreversible instrumentation");
        }
        bbWrite(Join(Join(includes, "LockFreeQueue"), name), patched);
        manifest << name << '\t' << Fingerprint(Join(source, name)) << '\n';
    }
    const std::string tests = Join(root, "src/CTaskThreadManagerTest/CTaskThreadManagerTest");
    bbWrite(Join(output, "CTaskQueueResourceRace.h"), bbRead(Join(tests, "CTaskQueueResourceRace.h")));
    bbWrite(Join(output, "CTaskQueueResourceRace.cpp"), bbRead(Join(tests, "CTaskQueueResourceRace.cpp")));
    bbWrite(Join(output, "source-identity.tsv"), manifest.str());
    std::string executable = Join(output, "queue-race");
    std::vector<std::string> arguments;
#ifdef _WIN32
    executable += ".exe";
    arguments = {"/nologo", "/EHsc", "/O2", "/MT", "/DSTATIC_LIB", "/DNOMINMAX", "/DCTTM_QUEUE_RESOURCE_RACE_MAIN",
        "/I" + includes, "/Fe:" + executable, "/Fo:" + Join(output, "race.obj"), "/Fd:" + Join(output, "race.pdb"),
        Join(output, "CTaskQueueResourceRace.cpp")};
#else
    arguments = {"-std=c++11", "-O2", "-pthread", "-DSTATIC_LIB", "-DCTTM_QUEUE_RESOURCE_RACE_MAIN",
        "-I" + includes, Join(output, "CTaskQueueResourceRace.cpp"), "-o", executable};
#endif
    execute(compiler, arguments, Join(output, "build.log"), 180);
    execute(executable, {}, Join(output, "result.log"), 30);
    std::cout << bbRead(Join(output, "result.log"));
}

int32_t CTaskTestAutomation::run(int32_t argc, char** argv)
{
    try
    {
        bbCheck(argc >= 1, "missing automation command");
        const std::string command = argv[0];
        if (command == "--automation-self-test")
        {
            bbCheck(argc == 1, "self-test takes no arguments");
            selfTest();
        }
        else if (command == "--queue-resource-race")
        {
            bbCheck(argc == 4, "usage: --queue-resource-race root new-output compiler");
            queueRace(argv[1], argv[2], argv[3]);
        }
        else if (command == "--repeat")
        {
            bbCheck(argc == 4, "usage: --repeat test-selection rounds new-output");
            const int32_t rounds = Positive(argv[2], 1000);
            directory(argv[3]);
            for (int32_t round = 1; round <= rounds; ++round)
            {
                const std::string report = Join(argv[3], "repeat-" + bbNumber(round) + ".tsv");
                execute(bbExecutable(), {"--tests", argv[1], "--report", report}, report + ".log", 900);
                std::cout << "repeat " << round << '/' << rounds << " passed\n";
            }
        }
        else if (command == "--matrix")
        {
            bbCheck(argc >= 5 && argc <= 7, "usage: --matrix throughput|latency|prepared|auxiliary rounds tasks new-output [baseline-test.exe [current-test.exe]]");
            const std::string phase = argv[1];
            bbCheck(phase == "throughput" || phase == "latency" || phase == "prepared" || phase == "auxiliary", "unknown matrix phase");
            const int32_t rounds = Positive(argv[2], 10);
            const int32_t tasks = Positive(argv[3], 40000000);
            ValidateMatrixTasks(phase, tasks);
            const std::string output = argv[4];
            directory(output);
            // 可显式指定两个独立基准；调度器本身的分配故障替身不能混入其中一方。
            const std::string current = argc == 7 ? argv[6] : bbExecutable();
            const std::vector<std::string> binaries = argc >= 6 ? std::vector<std::string>{argv[5], current} : std::vector<std::string>{current};
            const std::vector<std::string> kinds = argc >= 6 ? std::vector<std::string>{"baseline", "current"} : std::vector<std::string>{"current"};
            std::ostringstream identity;
            for (size_t i = 0; i < binaries.size(); ++i)
            {
                identity << kinds[i] << '\t' << binaries[i] << '\t' << Fingerprint(binaries[i]) << '\n';
            }
            bbWrite(Join(output, "executables.tsv"), identity.str());
            std::vector<std::string> modes;
            if (phase == "prepared")
            {
                modes = {"throughput", "throughput-reused", "throughput-counted", "throughput-separated", "throughput-reused-separated", "latency", "latency-reused", "latency-window"};
            }
            else if (phase == "auxiliary")
            {
                modes = {"throughput", "lookup", "lifecycle", "roundtrip"};
            }
            else
            {
                modes.push_back(phase == "throughput" ? "throughput-reused" : "latency-window");
            }
            std::vector<std::string> reports, labels;
            size_t scenario = 0;
            size_t failures = 0;
            std::ostringstream runs;
            runs << "name\tresult\n";
            for (int32_t round = 1; round <= rounds; ++round)
            {
                for (size_t mode = 0; mode < modes.size(); ++mode)
                {
                    for (int32_t producers = 4; producers <= 32; producers *= 2)
                    {
                        for (int32_t levels = 1; levels <= 10; levels += 9)
                        {
                            const bool single = modes[mode] == "lifecycle" || modes[mode] == "roundtrip";
                            if (single && (producers != 4 || levels != 1))
                            {
                                continue;
                            }
                            int32_t scenarioCount = phase == "auxiliary" ? tasks / producers : tasks;
                            if (single)
                            {
                                // 千次往返只有几毫秒，容易把一次调度抖动放大为明显退化。
                                // 小规模入口仍用于设施自检，正式矩阵以百万往返和万次生命周期测量。
                                scenarioCount = (std::min)(tasks, modes[mode] == "lifecycle" ? 10000 : 1000000);
                            }
                            const size_t begin = reports.size();
                            for (size_t order = 0; order < binaries.size(); ++order)
                            {
                                const size_t version = (order + scenario + round) % binaries.size();
                                const std::string name = kinds[version] + "-" + modes[mode] + "-r" + bbNumber(round) + "-p" + bbNumber(single ? 1 : producers) + "-l" + bbNumber(levels);
                                const std::string report = Join(output, name + ".csv");
                                std::vector<std::string> args = {phase == "auxiliary" ? "--benchmark" : "--prepared-benchmark",
                                    modes[mode], bbNumber(single ? 1 : producers),
                                    bbNumber(scenarioCount), bbNumber(levels)};
                                if (phase == "auxiliary")
                                {
                                    args.push_back("0");
                                }
                                args.push_back(report);
                                try
                                {
                                    execute(binaries[version], args, Join(output, name + ".log"), 900);
                                    reports.push_back(report);
                                    labels.push_back(kinds[version]);
                                    runs << name << "\tPASS\n";
                                    std::cout << "PASS " << name << std::endl;
                                }
                                catch (const std::exception& error)
                                {
                                    ++failures;
                                    runs << name << "\tFAIL\n";
                                    std::cerr << "FAIL " << name << ": " << error.what() << std::endl;
                                }
                            }
                            // 两个程序各自仍执行；任何一方失败就不以单边结果形成比较。
                            retainCompleteGroup(begin, binaries.size(), reports, labels);
                            bbWrite(Join(output, "runs.tsv"), runs.str());
                            ++scenario;
                        }
                    }
                }
            }
            // 第一标签决定基线；执行顺序交错但汇总不能随第一条先执行程序而反转。
            if (argc >= 6 && !labels.empty() && labels[0] != "baseline")
            {
                size_t first = 1;
                while (first < labels.size() && labels[first] != "baseline")
                {
                    ++first;
                }
                bbCheck(first < labels.size(), "baseline was not run");
                std::swap(labels[0], labels[first]);
                std::swap(reports[0], reports[first]);
            }
            if (!reports.empty())
            {
                summarize(output, reports, labels);
            }
            if (failures != 0)
            {
                std::cerr << "AUTOMATION_FAIL failed_runs=" << failures
                    << "; summary contains complete successful groups only; see runs.tsv" << std::endl;
                return 1;
            }
        }
        else
        {
            throw std::runtime_error("unknown native automation command");
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "AUTOMATION_FAIL " << error.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "AUTOMATION_FAIL unknown exception\n";
    }
    return 1;
}