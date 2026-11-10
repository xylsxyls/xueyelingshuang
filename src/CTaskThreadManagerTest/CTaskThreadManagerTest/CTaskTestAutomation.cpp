#include "CTaskTestAutomation.h"
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
                cells["cpus"] + "," + cells["queue_producers"];
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
    summary << "mode,producers,levels,cpus,queue_producers,metric,label,samples,median,baseline_median,change_percent\n";
    summary << std::fixed << std::setprecision(6);
    for (std::map<std::string, std::map<std::string, std::vector<double>>>::const_iterator group = groups.begin(); group != groups.end(); ++group)
    {
        const std::map<std::string, std::vector<double>>& runs = group->second;
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
    bbCheck(median({3,1,2}) == 2 && median({10,2,4,6}) == 5, "median mismatch");
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
    bbCheck(summary.size() == 3 && summary[2][6] == "current" && Number(summary[2][8]) == 30 &&
        Number(summary[2][9]) == 20 && Number(summary[2][10]) == 50, "comparison percentage mismatch");
    const char* badResults[] = {"throughput,4,1,1,10,10,10,30\n", "throughput,4,1,0,10,9,10,30\n",
        "throughput,4,1,0,10,10,10,nan\n"};
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
    do
    {
        if ((entry.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0)
        {
            names.push_back(entry.cFileName);
        }
    } while (FindNextFileA(find, &entry));
    FindClose(find);
#else
    DIR* dir = opendir(source.c_str());
    bbCheck(dir != nullptr, "Queue source not found");
    for (dirent* entry = readdir(dir); entry != nullptr; entry = readdir(dir))
    {
        names.push_back(entry->d_name);
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
            bbCheck(argc == 5 || argc == 6, "usage: --matrix throughput|latency|prepared|auxiliary rounds tasks new-output [baseline-test.exe]");
            const std::string phase = argv[1];
            bbCheck(phase == "throughput" || phase == "latency" || phase == "prepared" || phase == "auxiliary", "unknown matrix phase");
            const int32_t rounds = Positive(argv[2], 10);
            const int32_t tasks = Positive(argv[3], 40000000);
            bbCheck(tasks >= 1024 && tasks % 32 == 0, "tasks must be >=1024 and divisible by32");
            bbCheck(phase != "auxiliary" || tasks <= 10000000, "auxiliary total must be <=10000000");
            const std::string output = argv[4];
            directory(output);
            const std::vector<std::string> binaries = argc == 6 ? std::vector<std::string>{argv[5], bbExecutable()} : std::vector<std::string>{bbExecutable()};
            const std::vector<std::string> kinds = argc == 6 ? std::vector<std::string>{"baseline", "current"} : std::vector<std::string>{"current"};
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
                            for (size_t order = 0; order < binaries.size(); ++order)
                            {
                                const size_t version = (order + scenario + round) % binaries.size();
                                const std::string name = kinds[version] + "-" + modes[mode] + "-r" + bbNumber(round) + "-p" + bbNumber(single ? 1 : producers) + "-l" + bbNumber(levels);
                                const std::string report = Join(output, name + ".csv");
                                std::vector<std::string> args = {phase == "auxiliary" ? "--benchmark" : "--prepared-benchmark",
                                    modes[mode], bbNumber(single ? 1 : producers),
                                    bbNumber(single ? 1000 : (phase == "auxiliary" ? tasks / producers : tasks)), bbNumber(levels)};
                                if (phase == "auxiliary")
                                {
                                    args.push_back("0");
                                }
                                args.push_back(report);
                                execute(binaries[version], args, Join(output, name + ".log"), 900);
                                reports.push_back(report);
                                labels.push_back(kinds[version]);
                                std::cout << "PASS " << name << std::endl;
                            }
                            ++scenario;
                        }
                    }
                }
            }
            // 第一标签决定基线；执行顺序交错但汇总不能随第一条先执行程序而反转。
            if (argc == 6 && labels[0] != "baseline")
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
            summarize(output, reports, labels);
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