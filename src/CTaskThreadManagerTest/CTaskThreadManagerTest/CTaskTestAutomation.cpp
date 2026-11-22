#include "CTaskTestAutomation.h"
#include "CTaskThreadBenchmark.h"
#include "CTaskFairnessTests.h"
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

/** 核对同一计时窗口内的操作数、毫秒数与每秒速率，容许 CSV 六位小数的舍入
@param [in] cells 报告字段
@param [in] count 与该计时终点相对应的实际操作数
@param [in] duration 毫秒列名
@param [in] rate 每秒操作数列名
@param [in] required 正式矩阵必须提供完整计量；通用汇总允许仅比较已提供的指标
*/
static void CheckReportedRate(const std::map<std::string, std::string>& cells, uint64_t count,
    const char* duration, const char* rate, bool required = false)
{
    const auto timeCell = cells.find(duration);
    const auto rateCell = cells.find(rate);
    if (timeCell == cells.end() || rateCell == cells.end())
    {
        bbCheck(!required, std::string("missing matrix measurement: ") + duration + "/" + rate);
        return;
    }
    const double ms = Number(timeCell->second);
    const double perSecond = Number(rateCell->second);
    const double expected = static_cast<double>(count) * 1000.0;
    const double product = ms * perSecond;
    // 两个字段各有半个末位的舍入误差，再允许乘法的浮点误差。
    const double tolerance = 0.00000051 * (ms + perSecond + 1.0) + expected * 1e-14;
    bbCheck(ms > 0.0 && perSecond >= 0.0 && std::isfinite(product) &&
        std::fabs(product - expected) <= tolerance, "rate differs from count and duration");
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

/** 按实际启动参数验证单次矩阵结果，避免两个程序同时报告错误负载仍配对成功
@param [in] csv 已通过表头唯一性和行宽检查的单次报告
@param [in] mode 本次命令的测量模式
@param [in] producers 实际发起请求的生产线程数
@param [in] levels 命令中指定的优先级数
@param [in] count 预建任务入口为任务总数，辅助入口为每个生产者的操作数
@param [in] auxiliary 是否使用带 count 和 work 的辅助入口，矩阵的 work 固定为零
*/
static void CheckMatrixRun(const std::vector<std::vector<std::string>>& csv, const std::string& mode,
    int32_t producers, int32_t levels, int32_t count, bool auxiliary)
{
    bbCheck(csv.size() == 2, "matrix invocation must produce exactly one scenario");
    std::map<std::string, std::string> fields;
    for (size_t i = 0; i < csv[0].size(); ++i)
    {
        fields[csv[0][i]] = csv[1][i];
    }
    const uint64_t total = static_cast<uint64_t>(count) * (auxiliary ? producers : 1);
    bbCheck(fields["mode"] == mode && bbUnsigned(fields["producers"].c_str(), UINT64_MAX) == static_cast<uint64_t>(producers) &&
        bbUnsigned(fields["levels"].c_str(), UINT64_MAX) == static_cast<uint64_t>(levels), "matrix workload differs from command");
    bbCheck(bbUnsigned(fields["total"].c_str(), UINT64_MAX) == total && bbUnsigned(fields["accepted"].c_str(), UINT64_MAX) == total &&
        bbUnsigned(fields["completed"].c_str(), UINT64_MAX) == total && bbUnsigned(fields["errors"].c_str(), UINT64_MAX) == 0,
        "matrix task ledger differs from command");
    bbCheck(bbUnsigned(fields["retries"].c_str(), UINT64_MAX) == 0, "matrix workload reported external retries");
    const double submitMs = Number(fields["submit_ms"]);
    bbCheck(submitMs > 0, "invalid submission duration");
    if (auxiliary)
    {
        CheckReportedRate(fields, total, "complete_ms", "throughput_ops_s", true);
        if (mode == "roundtrip" || mode == "lifecycle" || mode == "lookup")
        {
            bbCheck(Number(fields["complete_ms"]) >= submitMs, "synchronous completion precedes submission");
        }
        const double p50 = Number(fields["p50_us"]);
        const double p95 = Number(fields["p95_us"]);
        const double p99 = Number(fields["p99_us"]);
        bbCheck(0 <= p50 && p50 <= p95 && p95 <= p99, "invalid auxiliary latency quantiles");
        bbCheck(bbUnsigned(fields["count"].c_str(), UINT64_MAX) == static_cast<uint64_t>(count) &&
            bbUnsigned(fields["work"].c_str(), UINT64_MAX) == 0, "auxiliary workload differs from command");
        return;
    }
    // 正式矩阵使用固定的预建对象和预留容量，避免两端同时测错场景仍形成有效对照。
    const bool distinctTasks = mode == "throughput" || mode == "throughput-separated" || mode == "latency";
    bbCheck(bbUnsigned(fields["task_objects"].c_str(), UINT64_MAX) == (distinctTasks ? total : 2000) &&
        bbUnsigned(fields["hash_slots"].c_str(), UINT64_MAX) == total + 4096 &&
        bbUnsigned(fields["queue_nodes_per_level"].c_str(), UINT64_MAX) == (total + levels - 1) / levels + 4096,
        "prepared task population or reserved capacity differs from command");
    bbUnsigned(fields["consumer_mask"].c_str(), UINT64_MAX);
    bbUnsigned(fields["producer_mask"].c_str(), UINT64_MAX);
    CheckReportedRate(fields, total, "submit_ms", "submit_ops_s", true);
    CheckReportedRate(fields, total, "complete_ms", "complete_ops_s", true);
    CheckReportedRate(fields, total, "drained_ms", "drained_ops_s", true);
    const uint64_t overlap = bbUnsigned(fields["consumed_at_submit_lower_bound"].c_str(), total);
    CheckReportedRate(fields, overlap, "submit_ms", "overlap_consume_ops_s_lower_bound", true);
    const double completeMs = Number(fields["complete_ms"]);
    const double drainMs = Number(fields["drain_ms"]);
    // 三个六位小数的独立舍入可能相差一个末位。
    bbCheck(drainMs >= 0 && std::fabs(drainMs - (std::max)(0.0, completeMs - submitMs)) <= 0.00000151 &&
        Number(fields["drained_ms"]) >= (std::max)(submitMs, completeMs), "inconsistent drain timestamps");
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
    std::map<std::string, std::string> configurations;
    for (size_t file = 0; file < reports.size(); ++file)
    {
        const std::vector<std::vector<std::string>> csv = readCsv(reports[file]);
        // 一轮可含不同场景，同一场景重复会改变中位数中这一轮的权重。
        std::set<std::string> scenarios;
        for (size_t row = 1; row < csv.size(); ++row)
        {
            std::map<std::string, std::string> cells;
            for (size_t col = 0; col < csv[0].size(); ++col)
            {
                cells[csv[0][col]] = csv[row][col];
            }
            bbCheck(cells.count("errors") && bbUnsigned(cells["errors"].c_str(), UINT64_MAX) == 0, "measurement has errors");
            // 核对整数账本再计算速率，避免缺列或浮点舍入把不同的任务数量判成相等。
            const uint64_t total = bbUnsigned(cells["total"].c_str(), UINT64_MAX);
            bbCheck(!cells["mode"].empty() && total > 0 && bbUnsigned(cells["accepted"].c_str(), UINT64_MAX) == total &&
                bbUnsigned(cells["completed"].c_str(), UINT64_MAX) == total, "incomplete workload");
            CheckReportedRate(cells, total, "submit_ms", "submit_ops_s");
            CheckReportedRate(cells, total, "complete_ms", "complete_ops_s");
            CheckReportedRate(cells, total, "complete_ms", "throughput_ops_s");
            CheckReportedRate(cells, total, "drained_ms", "drained_ops_s");
            if (cells.count("consumed_at_submit_lower_bound"))
            {
                const uint64_t consumed = bbUnsigned(cells["consumed_at_submit_lower_bound"].c_str(), total);
                CheckReportedRate(cells, consumed, "submit_ms", "overlap_consume_ops_s_lower_bound");
            }
            if (cells.count("count"))
            {
                const uint64_t producers = bbUnsigned(cells["producers"].c_str(), 32);
                bbCheck(producers > 0 && total % producers == 0 &&
                    bbUnsigned(cells["count"].c_str(), UINT64_MAX) == total / producers, "workload differs from producer count");
            }
            const std::string key = cells["mode"] + "," + cells["producers"] + "," + cells["levels"] + "," +
                cells["cpus"] + "," + cells["queue_producers"] + "," + cells["total"] + "," +
                cells["count"] + "," + cells["work"] + "," + cells["task_objects"];
            bbCheck(scenarios.insert(key).second, "duplicate workload in the same run");
            // 相同业务负载的轮次须采用同样的预留和处理器放置，避免把不同条件合成一个中位数。
            const std::string settings = cells["hash_slots"] + "," + cells["queue_nodes_per_level"] + "," +
                (cells.count("hash_buckets") ? cells["hash_buckets"] : "0") + "," + cells["consumer_mask"] + "," + cells["producer_mask"];
            const std::pair<std::map<std::string, std::string>::iterator, bool> entry =
                configurations.insert(std::make_pair(key, settings));
            bbCheck(entry.first->second == settings, "incompatible pool capacity or CPU placement across runs");
            for (size_t col = 0; col < csv[0].size(); ++col)
            {
                const std::string& name = csv[0][col];
                if (name.find("ops_s") != std::string::npos || name.find("_us") != std::string::npos ||
                    name.find("_ms") != std::string::npos || name.find("_mib") != std::string::npos ||
                    name.find("_mb") != std::string::npos || name.find("handles_") == 0 || name == "submitted" || name == "retries")
                {
                    if (csv[row][col] == "NA" && (name == "peak_mib" || name == "cpu_ms" || name == "producer_cpu_ms" ||
                        name == "peak_working_mb" || name == "private_before_mb" || name == "private_after_mb" ||
                        name == "handles_before" || name == "handles_after"))
                    {
                        continue;
                    }
                    const double measurement = Number(csv[row][col]);
                    bbCheck(measurement >= 0, "negative workload measurement");
                    groups[key + "," + name][labels[file]].push_back(measurement);
                }
            }
            if (cells["mode"].find("latency") == 0)
            {
                const std::vector<std::vector<std::string>> latency = readCsv(reports[file] + ".latency.csv");
                const std::vector<std::string> expected = {"level", "samples", "post_p50_us", "post_p99_us", "response_p50_us", "response_p99_us"};
                bbCheck(latency[0] == expected, "latency schema changed");
                const size_t levels = static_cast<size_t>(bbUnsigned(cells["levels"].c_str(), 10));
                bbCheck(levels > 0 && latency.size() == levels + 2, "missing latency priority group");
                const uint64_t samples = bbUnsigned(latency[1][1].c_str(), UINT64_MAX);
                const uint64_t producers = bbUnsigned(cells["producers"].c_str(), 32);
                const uint64_t total = bbUnsigned(cells["total"].c_str(), UINT64_MAX);
                bbCheck(producers != 0 && total > 0 && total % producers == 0, "invalid sampled workload");
                // 每生产者从序号零开始，每 1009 项取样；同时丢失总组和分组中的样本也要拒绝。
                const uint64_t samplesPerProducer = 1 + (total / producers - 1) / 1009;
                bbCheck(samples == producers * samplesPerProducer, "latency observation count does not match workload");
                // 1009 与支持的 1 至 10 级均互质，每个完整周期各级各取一次；尾周期按实际序号轮转。
                std::vector<uint64_t> expectedCounts(levels, (samplesPerProducer / levels) * producers);
                for (size_t extra = 0; extra < samplesPerProducer % levels; ++extra)
                {
                    expectedCounts[(extra * 1009) % levels] += producers;
                }
                for (size_t item = 1; item < latency.size(); ++item)
                {
                    const uint64_t count = bbUnsigned(latency[item][1].c_str(), samples);
                    bbCheck(bbUnsigned(latency[item][0].c_str(), levels) == item - 1 && count > 0, "invalid latency samples");
                    if (item > 1)
                    {
                        bbCheck(count == expectedCounts[item - 2], "latency sample count does not match priority");
                    }
                    bbCheck(Number(latency[item][2]) >= 0 && Number(latency[item][3]) >= Number(latency[item][2]) &&
                        Number(latency[item][4]) >= 0 && Number(latency[item][5]) >= Number(latency[item][4]), "invalid latency quantiles");
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
    summary << "mode,producers,levels,cpus,queue_producers,total,count,work,task_objects,metric,label,samples,median,baseline_median,change_percent,min,max\n";
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
            summary << ',' << *std::min_element(version->second.begin(), version->second.end())
                << ',' << *std::max_element(version->second.begin(), version->second.end()) << '\n';
        }
    }
    bbWrite(Join(output, "summary.csv"), summary.str());
}

/** 核对公平性报告中每个生产者的身份、实际进展及所属负载
@param [in] csv 已通过表头唯一性和行宽检查的完整报告
@param [in] scenario 生产者数、优先级数、CPU 数和队列登记容量
@param [in] minimumMs 必须实际完成的观察时长，单位毫秒
*/
static void CheckFairnessReport(const std::vector<std::vector<std::string>>& csv, const int32_t (&scenario)[4], uint64_t minimumMs)
{
    const std::vector<std::string> header = {"producer", "producers", "levels", "cpus", "queue_producers", "submitted", "consumed",
        "max_submit_gap_ms", "consumer_max_gap_ms", "producer_cpu_ms", "capacity_failures", "external_retries", "submit_ms",
        "complete_ms", "errors", "resource_failures", "observation_ms"};
    const size_t producers = static_cast<size_t>(scenario[0]);
    bbCheck(csv.size() == producers + 1 && csv[0] == header, "incomplete fairness report or unexpected columns");
    std::vector<bool> seen(producers, false);
    for (size_t i = 1; i < csv.size(); ++i)
    {
        const std::vector<std::string>& row = csv[i];
        const size_t producer = static_cast<size_t>(bbUnsigned(row[0].c_str(), producers - 1));
        bbCheck(!seen[producer], "duplicate fairness producer");
        seen[producer] = true;
        for (size_t j = 0; j < 4; ++j)
        {
            bbCheck(bbUnsigned(row[j + 1].c_str(), UINT64_MAX) == static_cast<uint64_t>(scenario[j]), "fairness workload mismatch");
        }
        const uint64_t observed = bbUnsigned(row[16].c_str(), UINT64_MAX);
        const uint64_t submittedAt = bbUnsigned(row[12].c_str(), UINT64_MAX);
        const uint64_t completedAt = bbUnsigned(row[13].c_str(), UINT64_MAX);
        bbCheck(observed >= minimumMs && submittedAt >= observed && completedAt >= submittedAt,
            "incomplete fairness observation or reversed completion times");
        bbCheck(row[8] == csv[1][8] && row[12] == csv[1][12] && row[13] == csv[1][13] && row[16] == csv[1][16],
            "inconsistent shared fairness measurements");
        const uint64_t submitted = bbUnsigned(row[5].c_str(), UINT64_MAX);
        bbCheck(submitted >= 1000 && bbUnsigned(row[6].c_str(), UINT64_MAX) == submitted, "fairness producer failed to make progress or lost work");
        bbCheck(bbUnsigned(row[7].c_str(), UINT64_MAX) < 2000 && bbUnsigned(row[8].c_str(), UINT64_MAX) < 2000,
            "fairness progress gap exceeded limit");
        bbCheck(bbUnsigned(row[10].c_str(), UINT64_MAX) == 0 && bbUnsigned(row[11].c_str(), UINT64_MAX) == 0 &&
            bbUnsigned(row[14].c_str(), UINT64_MAX) == 0 && bbUnsigned(row[15].c_str(), UINT64_MAX) == 0, "fairness run reported errors");
    }
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
    const uint32_t seconds = 10;
    const std::string report = bbTempPath("-fairness.csv");
    const std::vector<std::string> args = {"--fairness", bbNumber(scenarios[index][0]), bbNumber(scenarios[index][1]),
        bbNumber(seconds), bbNumber(scenarios[index][2]), report, bbNumber(scenarios[index][3])};
#ifdef CTTM_TEST_SINGLE_PROCESS
    std::vector<std::string> arguments(args);
    std::vector<char*> argv;
    for (size_t i = 0; i < arguments.size(); ++i)
    {
        argv.push_back(&arguments[i][0]);
    }
    bbCheck(runTaskFairnessTests(static_cast<int>(argv.size()), argv.data()) == 0, "fairness case failed");
#else
    execute(bbExecutable(), args, report + ".log", 120);
#endif
    CheckFairnessReport(readCsv(report), scenarios[index], seconds * 1000);
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
    // 64 次各隔 40ms 的真实成功覆盖 2560ms，但没有一次停顿超过 40ms。
    CTaskFairnessCounter progress;
    progress.m_eventLast = 100;
    for (uint64_t i = 1; i <= 64; ++i)
    {
        ++progress.m_count;
        progress.publish(100 + i * 40);
    }
    bbCheck(progress.m_maxGap == 40 && progress.m_published.load() == 64, "regular progress reported as starvation");
    ++progress.m_count;
    progress.publish(4661);
    bbCheck(progress.m_maxGap == 2001 && progress.m_maxGapAt == 4661, "actual producer stall was missed");
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
    // 报告自身可以完全自洽，但仍需与本次真实启动的负载逐项匹配。
    const std::vector<std::string> workloadHeader = {"mode", "producers", "levels", "total", "accepted", "completed", "errors", "count", "work", "submit_ms", "complete_ms", "throughput_ops_s", "p50_us", "p95_us", "p99_us", "retries"};
    const std::vector<std::string> correct = {"throughput", "4", "10", "1024", "1024", "1024", "0", "256", "0", "2", "4", "256000", "1", "2", "3", "0"};
    CheckMatrixRun({workloadHeader, correct}, "throughput", 4, 10, 256, true);
    // 异步消费可早于最后一次投递返回；同步往返、生命周期和查询须在投递终点之后完成。
    std::vector<std::string> early = correct;
    early[9] = "5";
    CheckMatrixRun({workloadHeader, early}, "throughput", 4, 10, 256, true);
    const char* synchronous[] = {"roundtrip", "lifecycle", "lookup"};
    for (size_t i = 0; i < sizeof(synchronous) / sizeof(synchronous[0]); ++i)
    {
        const bool single = i < 2;
        const int32_t producers = single ? 1 : 4;
        const int32_t levels = single ? 1 : 10;
        const int32_t count = 1024 / producers;
        early[0] = synchronous[i];
        early[1] = bbNumber(producers);
        early[2] = bbNumber(levels);
        early[7] = bbNumber(count);
        early[9] = "2";
        CheckMatrixRun({workloadHeader, early}, synchronous[i], producers, levels, count, true);
        early[9] = "5";
        bool rejected = false;
        try
        {
            CheckMatrixRun({workloadHeader, early}, synchronous[i], producers, levels, count, true);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "matrix accepted reversed synchronous endpoints");
    }
    const std::vector<std::string> preparedHeader = {"mode", "producers", "levels", "total", "accepted", "completed", "errors",
        "submit_ms", "complete_ms", "submit_ops_s", "complete_ops_s", "drained_ms", "drained_ops_s",
        "consumed_at_submit_lower_bound", "overlap_consume_ops_s_lower_bound", "drain_ms", "task_objects",
        "hash_slots", "queue_nodes_per_level", "consumer_mask", "producer_mask", "retries"};
    const std::vector<std::string> preparedRow = {"throughput-reused", "4", "10", "1024", "1024", "1024", "0",
        "2", "4", "512000", "256000", "8", "128000", "256", "128000", "2", "2000", "5120", "4199", "0", "0", "0"};
    CheckMatrixRun({preparedHeader, preparedRow}, "throughput-reused", 4, 10, 1024, false);
    std::vector<std::string> distinct = preparedRow;
    distinct[0] = "throughput-separated";
    distinct[16] = "1024";
    CheckMatrixRun({preparedHeader, distinct}, distinct[0], 4, 10, 1024, false);
    // 最后一次 doTask 可以早于最后一次 post 返回，仍须以退出时刻结束排空计时。
    std::vector<std::string> finishedEarly = preparedRow;
    finishedEarly[8] = "1";
    finishedEarly[10] = "1024000";
    finishedEarly[15] = "0";
    CheckMatrixRun({preparedHeader, finishedEarly}, "throughput-reused", 4, 10, 1024, false);
    for (int32_t scenario = 0; scenario < 8; ++scenario)
    {
        std::vector<std::string> inconsistent = preparedRow;
        if (scenario == 0)
        {
            inconsistent[11] = "2";
            inconsistent[12] = "512000";
        }
        else if (scenario == 1)
        {
            inconsistent[15] = "3";
        }
        else
        {
            const size_t column = 16 + scenario - 2;
            inconsistent[column] = column == 19 || column == 20 ? "invalid" : "1";
        }
        bool rejected = false;
        try
        {
            CheckMatrixRun({preparedHeader, inconsistent}, "throughput-reused", 4, 10, 1024, false);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "matrix accepted inconsistent timing, task population, pool capacity, affinity or external retries");
    }
    for (size_t column = 7; column < preparedHeader.size(); ++column)
    {
        std::vector<std::vector<std::string>> rows = {preparedHeader, preparedRow};
        rows[0].erase(rows[0].begin() + column);
        rows[1].erase(rows[1].begin() + column);
        bool rejected = false;
        try
        {
            CheckMatrixRun(rows, "throughput-reused", 4, 10, 1024, false);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "matrix accepted a missing measurement");
    }
    for (size_t column = 0; column <= correct.size(); ++column)
    {
        std::vector<std::vector<std::string>> rows = {workloadHeader, correct};
        if (column == correct.size())
        {
            rows.push_back(correct);
        }
        else
        {
            // 中间分位数超出 P99 时，即使每个值单独合法也应拒收。
            rows[1][column] = workloadHeader[column] == "p95_us" ? "4" : (workloadHeader[column] == "retries" ? "1" : "-1");
        }
        bool rejected = false;
        try
        {
            CheckMatrixRun(rows, "throughput", 4, 10, 256, true);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "matrix accepted a different workload or multiple result rows");
    }
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
    const int32_t fairnessScenario[4] = {2, 1, 0, 4};
    const std::string fairnessHeader = "producer,producers,levels,cpus,queue_producers,submitted,consumed,max_submit_gap_ms,consumer_max_gap_ms,producer_cpu_ms,capacity_failures,external_retries,submit_ms,complete_ms,errors,resource_failures,observation_ms\n";
    const std::string firstProducer = "0,2,1,0,4,1000,1000,1,2,NA,0,0,3000,3010,0,0,3000\n";
    const std::string secondProducer = "1,2,1,0,4,1200,1200,3,2,NA,0,0,3000,3010,0,0,3000\n";
    bbWrite(csv, fairnessHeader + secondProducer + firstProducer);
    const std::vector<std::vector<std::string>> validFairness = readCsv(csv);
    CheckFairnessReport(validFairness, fairnessScenario, 3000);
    // 两行报告完全一致时也须拒绝观察时间不足，不能用等待生产者退出的时间补足。
    bool tooShort = false;
    try
    {
        CheckFairnessReport(validFairness, fairnessScenario, 3001);
    }
    catch (const std::exception&)
    {
        tooShort = true;
    }
    bbCheck(tooShort, "fairness report passed with less time than requested");
    const std::string invalidProducers[] = {firstProducer, "1,2,1,0,4,1200,1199,3,2,NA,0,0,3000,3010,0,0,3000\n",
        "1,2,1,0,4,1200,1200,3,2000,NA,0,0,3000,3010,0,0,3000\n", "1,2,2,0,4,1200,1200,3,2,NA,0,0,3000,3010,0,0,3000\n",
        "1,2,1,0,4,1200,1200,3,2,NA,0,0,3000,3010,0,0,2999\n",
        "1,2,1,0,4,1200,1200,3,2,NA,0,0,3000,2999,0,0,3000\n",
        "1,2,1,0,4,1200,1200,3,2,NA,0,0,3000,3011,0,0,3000\n"};
    for (size_t i = 0; i < sizeof(invalidProducers) / sizeof(invalidProducers[0]); ++i)
    {
        bbWrite(csv, fairnessHeader + firstProducer + invalidProducers[i]);
        bool rejected = false;
        try
        {
            CheckFairnessReport(readCsv(csv), fairnessScenario, 3000);
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "invalid producer fairness report accepted");
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
        Number(summary[2][13]) == 20 && Number(summary[2][14]) == 50 &&
        Number(summary[2][15]) == 30 && Number(summary[2][16]) == 30, "comparison percentage mismatch");
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
        "throughput,4,1,0,10,10,10,nan\n", "throughput,4,1,0,10,10,10,-1\n", "throughput,4,1,0,20,20,20,30\n"};
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
    // 单份损坏报告也须被拒绝，避免只因两版输入条件不同而间接报错。
    bbWrite(candidate, "mode,errors,total,accepted,completed,complete_ms,complete_ops_s\nthroughput,0,10,10,10,0.003333,3000000.000000\n");
    summarize(output, {candidate}, {"current"});
    const char* invalidLedgers[] = {
        "mode,errors,total,accepted,completed,submit_ms,submit_ops_s\nthroughput,0,10,10,10,1,1000\n",
        "mode,errors,total,accepted,completed,complete_ms,complete_ops_s\nthroughput,0,10,10,10,2,10000\n",
        "mode,errors,total,accepted,completed,complete_ms,throughput_ops_s\nlookup,0,10,10,10,0,1000\n",
        "mode,errors,total,accepted,completed,drained_ms,drained_ops_s\nthroughput,0,10,10,10,1,2000\n",
        "mode,errors,total,accepted,completed,submit_ms,consumed_at_submit_lower_bound,overlap_consume_ops_s_lower_bound\nthroughput,0,10,10,10,1,8,10000\n",
        "mode,errors,total,accepted,completed,submit_ops_s\nthroughput,0,10,10,10,30\nthroughput,0,10,10,10,30\n",
        "mode,errors,total,accepted,completed,submit_ops_s,consumed_at_submit_lower_bound\nthroughput,0,10,10,10,30,11\n",
        "mode,errors,submit_ops_s\nthroughput,0,30\n",
        "mode,errors,total,completed,submit_ops_s\nthroughput,0,10,10,30\n",
        "mode,errors,submitted,consumed,submit_ops_s\nthroughput,0,1,1,30\n",
        "mode,errors,total,accepted,completed,submit_ops_s\nthroughput,0,1.5,1.5,1.5,30\n",
        "mode,errors,total,accepted,completed,submit_ops_s\nthroughput,0,0,0,0,30\n",
        "mode,errors,total,accepted,completed,submit_ops_s\nthroughput,0,9007199254740993,9007199254740993,9007199254740992,30\n",
        "mode,errors,producers,count,total,accepted,completed,submit_ops_s\nthroughput,0,4,3,10,10,10,30\n"};
    for (size_t i = 0; i < sizeof(invalidLedgers) / sizeof(invalidLedgers[0]); ++i)
    {
        bbWrite(candidate, invalidLedgers[i]);
        bool rejected = false;
        try
        {
            summarize(output, {candidate}, {"current"});
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "invalid task count ledger entered benchmark summary");
    }
    // 无操作系统计量后端时应保留吞吐并跳过未采集值，不能把它算作零。
    bbWrite(base, "mode,errors,total,accepted,completed,submit_ops_s,peak_mib\nthroughput,0,10,10,10,20,NA\n");
    summarize(output, {base}, {"current"});
    const std::vector<std::vector<std::string>> optional = readCsv(Join(output, "summary.csv"));
    bbCheck(optional.size() == 2 && optional[1][9] == "submit_ops_s", "missing resource metric counted as zero");
    // 总采样数精确等于各级之和，分位数必须有序；损坏的附表不能生成成功汇总。
    bbWrite(base, header + "latency-window,2,1,0,10,10,10,20\n");
    const std::string distribution = "level,samples,post_p50_us,post_p99_us,response_p50_us,response_p99_us\n0,2,1,2,3,4\n";
    const char* rows[] = {"1,2,1,2,3,4\n", "1,1,1,2,3,4\n", "1,2,2,1,3,4\n"};
    for (size_t i = 0; i < 3; ++i)
    {
        bbWrite(base + ".latency.csv", distribution + rows[i]);
        bool valid = true;
        try
        {
            summarize(output, {base}, {"current"});
        }
        catch (const std::exception&)
        {
            valid = false;
        }
        bbCheck(valid == (i == 0), "invalid latency summary oracle");
    }
    bbWrite(base + ".latency.csv", "level,samples,post_p50_us,post_p99_us,response_p50_us,response_p99_us\n0,1,1,2,3,4\n1,1,1,2,3,4\n");
    bool missingSampleRejected = false;
    try
    {
        summarize(output, {base}, {"current"});
    }
    catch (const std::exception&)
    {
        missingSampleRejected = true;
    }
    bbCheck(missingSampleRejected, "internally consistent but incomplete latency report accepted");
    // 两个生产者各投递 2019 项，在序号 0、1009、2018 取样；两级分别应有 4 和 2 个样本。
    bbWrite(base, header + "latency-window,2,2,0,4038,4038,4038,20\n");
    const std::string twoLevels = "level,samples,post_p50_us,post_p99_us,response_p50_us,response_p99_us\n0,6,1,2,3,4\n";
    bbWrite(base + ".latency.csv", twoLevels + "1,4,1,2,3,4\n2,2,1,2,3,4\n");
    summarize(output, {base}, {"current"});
    bbWrite(base + ".latency.csv", twoLevels + "1,2,1,2,3,4\n2,4,1,2,3,4\n");
    bool swappedRejected = false;
    try
    {
        summarize(output, {base}, {"current"});
    }
    catch (const std::exception&)
    {
        swappedRejected = true;
    }
    bbCheck(swappedRejected, "swapped priority sample counts accepted");
    const std::string configuredHeader = "mode,producers,levels,errors,accepted,completed,total,submit_ops_s,queue_nodes_per_level,consumer_mask,producer_mask\n";
    bbWrite(base, configuredHeader + "throughput,4,1,0,10,10,10,20,100,1,12\n");
    const char* conflicting[] = {"101,1,12", "100,0,12", "100,1,0"};
    for (size_t i = 0; i < sizeof(conflicting) / sizeof(conflicting[0]); ++i)
    {
        bbWrite(candidate, configuredHeader + "throughput,4,1,0,10,10,10,30," + conflicting[i] + "\n");
        bool rejected = false;
        try
        {
            summarize(output, {base, candidate}, {"baseline", "current"});
        }
        catch (const std::exception&)
        {
            rejected = true;
        }
        bbCheck(rejected, "pool or placement mismatch entered benchmark summary");
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
    BlackBoxChild mixedModes;
    mixedModes.start({"--list", "--tests", "1"}, csv + ".mode.log");
    bbCheck(mixedModes.wait(5000) == 1 && bbRead(csv + ".mode.log").find("conflicting test selection modes") != std::string::npos,
        "listing silently suppressed the requested test");
    // 拒绝不会实际执行的负载维度，避免报表写成多个优先级或带计算量的查询。
    const char* modes[] = {"roundtrip", "lifecycle", "lookup"};
    for (size_t i = 0; i < 3; ++i)
    {
        BlackBoxChild rejected;
        rejected.start({"--benchmark", modes[i], "1", "1", i == 2 ? "1" : "2", i == 2 ? "1" : "0", csv + ".unused.csv"},
            csv + ".unused.log");
        bbCheck(rejected.wait(5000) == 1 && bbRead(csv + ".unused.log").find("invalid benchmark input") != std::string::npos,
            "ineffective benchmark setting accepted or failed for another reason");
    }
    // 规模参数必须完整解析，不能把带后缀或溢出的数字静默变成另一组测试。
    const char* invalidCounts[] = {"4x", "-1", "18446744073709551616", ""};
    for (size_t i = 0; i < sizeof(invalidCounts) / sizeof(invalidCounts[0]); ++i)
    {
        BlackBoxChild fairness;
        fairness.start({"--fairness", invalidCounts[i], "1", "3", "0", csv + ".fairness.csv"}, csv + ".fairness.log");
        bbCheck(fairness.wait(5000) != 0, "invalid fairness count accepted");
        BlackBoxChild prepared;
        prepared.start({"--prepared-benchmark", "throughput-reused", invalidCounts[i], "3200", "1", csv + ".prepared.csv"},
            csv + ".prepared.log");
        const char* reason = i == 2 ? "integer argument out of range" : (i == 3 ? "empty integer argument" : "invalid integer argument");
        bbCheck(prepared.wait(5000) == 1 && bbRead(csv + ".prepared.log").find(reason) != std::string::npos,
            "invalid prepared count accepted or rejected for another reason");
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
                execute(bbExecutable(), {"--tests", argv[1], "--require-all", "--report", report}, report + ".log", 900);
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
                                    CheckMatrixRun(readCsv(report), modes[mode], single ? 1 : producers, levels, scenarioCount, phase == "auxiliary");
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