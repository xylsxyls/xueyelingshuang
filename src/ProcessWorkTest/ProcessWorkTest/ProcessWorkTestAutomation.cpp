#include "ProcessWorkTestAutomation.h"
#include <algorithm>
#include <cmath>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/stat.h>
#endif

/** 严格解析整数参数
@param [in] text 十进制完整参数
@param [in] maximum 允许的最大值
@return 非负整数，非法输入抛测试错误
*/
static int32_t Integer(const std::string& text, int32_t maximum)
{
    bbCheck(!text.empty() && text.find_first_not_of("0123456789") == std::string::npos, "invalid integer: " + text);
    std::istringstream input(text);
    int64_t value = 0;
    bbCheck((input >> value) && value <= maximum, "integer overflow: " + text);
    return static_cast<int32_t>(value);
}

/** 拆分逗号选项，拒绝空项及重复项
@param [in] text 非空逗号列表
@return 原顺序项目
*/
static std::vector<std::string> Split(const std::string& text)
{
    std::vector<std::string> result;
    std::set<std::string> seen;
    std::istringstream input(text);
    std::string item;
    while (std::getline(input, item, ','))
    {
        bbCheck(!item.empty() && seen.insert(item).second, "empty or duplicate list item");
        result.push_back(item);
    }
    bbCheck(!result.empty() && text[text.size() - 1] != ',', "empty list");
    return result;
}

/** CSV字符串字段，路径中含逗号或引号也能准确往返
@param [in] text 完整字段
@return 已转义且加引号的CSV字段
*/
static std::string Csv(const std::string& text)
{
    std::string result("\"");
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '"')
        {
            result += '"';
        }
        result += text[i];
    }
    return result + '"';
}

void ProcessWorkTestAutomation::directory(const std::string& path)
{
    bbCheck(!path.empty(), "empty output path");
#ifdef _WIN32
    bbCheck(CreateDirectoryA(path.c_str(), nullptr) != FALSE, "output must not exist; parent must exist: " + path);
#else
    bbCheck(mkdir(path.c_str(), 0700) == 0, "output must not exist; parent must exist: " + path);
#endif
}

void ProcessWorkTestAutomation::execute(const std::string& executable, const std::vector<std::string>& arguments,
    const std::string& log, int32_t seconds)
{
    BlackBoxChild child;
    child.startProgram(executable, arguments, log);
    const int32_t result = child.wait(seconds * 1000);
    bbCheck(result == 0, "child exit=" + bbNumber(result) + "; log=" + log);
}

std::string ProcessWorkTestAutomation::fingerprint(const std::string& path)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    bbCheck(input.good(), "cannot fingerprint: " + path);
    uint64_t value = UINT64_C(14695981039346656037), size = 0;
    char buffer[16384];
    while (input.read(buffer, sizeof(buffer)) || input.gcount())
    {
        size += static_cast<uint64_t>(input.gcount());
        for (std::streamsize i = 0; i < input.gcount(); ++i)
        {
            value = (value ^ static_cast<unsigned char>(buffer[i])) * UINT64_C(1099511628211);
        }
    }
    bbCheck(input.eof(), "file read failed: " + path);
    std::ostringstream text;
    text << "fnv1a64=" << std::hex << value << ";bytes=" << std::dec << size;
    return text.str();
}

std::string ProcessWorkTestAutomation::plain(std::string text)
{
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    if (text.compare(0, 3, "\xef\xbb\xbf") == 0)
    {
        text.erase(0, 3);
    }
    return text;
}

void ProcessWorkTestAutomation::writeCode(const std::string& path, const std::string& text)
{
    std::string output("\xef\xbb\xbf");
    const std::string content = plain(text);
    const size_t last = content.find_last_not_of(" \r\n\t");
    bbCheck(last != std::string::npos, "empty generated source");
    for (size_t i = 0; i <= last; ++i)
    {
        if (content[i] == '\n')
        {
            output += '\r';
        }
        output += content[i];
    }
    bbWrite(path, output);
}

void ProcessWorkTestAutomation::replaceOnce(std::string& text, const std::string& before, const std::string& after)
{
    const size_t at = text.find(before);
    bbCheck(at != std::string::npos && text.find(before, at + before.size()) == std::string::npos,
        "source anchor changed; review required: " + before);
    text.replace(at, before.size(), after);
}

std::map<std::string, double> ProcessWorkTestAutomation::parse(const std::string& text)
{
    const size_t start = text.rfind('{');
    bbCheck(start != std::string::npos, "missing benchmark JSON");
    std::istringstream input(text.substr(start));
    std::map<std::string, double> result;
    char token = 0;
    bbCheck((input >> token) && token == '{', "invalid JSON start");
    do
    {
        std::string key, value;
        bbCheck((input >> token) && token == '"' && std::getline(input, key, '"'), "invalid JSON key");
        bbCheck((input >> token) && token == ':', "missing colon");
        input >> std::ws;
        while (input.get(token) && token != ',' && token != '}')
        {
            value += token;
        }
        bbCheck(input.good() && !value.empty(), "truncated JSON");
        double number = 0;
        std::istringstream field(value);
        std::string word;
        field >> word;
        if (word == "true" || word == "false")
        {
            number = word == "true" ? 1 : 0;
        }
        else
        {
            field.clear();
            field.str(value);
            bbCheck((field >> number) && std::isfinite(number), "invalid finite JSON number");
        }
        bbCheck(!(field >> word) && result.insert(std::make_pair(key, number)).second, "duplicate key or extra JSON token");
    } while (token == ',');
    bbCheck(token == '}' && !(input >> token), "trailing JSON content");
    return result;
}

double ProcessWorkTestAutomation::median(std::vector<double> values)
{
    bbCheck(!values.empty(), "empty median");
    for (size_t i = 0; i < values.size(); ++i)
    {
        bbCheck(std::isfinite(values[i]), "nonfinite sample");
    }
    std::sort(values.begin(), values.end());
    const size_t middle = values.size() / 2;
    return values.size() % 2 ? values[middle] : (values[middle - 1] + values[middle]) / 2;
}

std::vector<ProcessWorkTestWorkload> ProcessWorkTestAutomation::workloads()
{
    const int32_t combinations[][2] = { { 1, 1 }, { 1, 4 }, { 1, 8 }, { 1, 16 }, { 1, 32 }, { 2, 4 }, { 4, 4 } };
    std::vector<ProcessWorkTestWorkload> result;
    for (int32_t size = 0; size < 2; ++size)
    {
        for (int32_t i = 0; i < 10; ++i)
        {
            if (size == 1 && i == 9)
            {
                continue;
            }
            ProcessWorkTestWorkload value;
            const int32_t pair = i < 7 ? i : (i == 7 ? 5 : (i == 8 ? 1 : 0));
            value.m_processes = combinations[pair][0];
            value.m_threads = combinations[pair][1];
            value.m_bytes = size == 0 ? 6 : 64;
            value.m_blocks = size == 0 ? 1024 : 512;
            value.m_mode = i < 7 ? 0 : (i == 7 ? 1 : 2);
            value.m_count = (size == 0 ? 10000000 : (i == 8 ? 200000 : 2000000)) / (value.m_processes * value.m_threads);
            value.m_name = std::string(value.m_mode == 0 ? "pid-" : (value.m_mode == 1 ? "name-" : "post-")) +
                bbNumber(value.m_processes) + "x" + bbNumber(value.m_threads) + "-" + bbNumber(value.m_bytes);
            result.push_back(value);
        }
    }
    const int32_t extra[][6] = { { 2, 4, 62500, 4096, 128, 0 }, { 2, 4, 12500, 65536, 64, 0 },
        { 1, 4, 500000, 0, 512, 0 }, { 1, 4, 2500000, 64, 1024, 0 } };
    for (int32_t i = 0; i < 4; ++i)
    {
        ProcessWorkTestWorkload value;
        value.m_processes = extra[i][0]; value.m_threads = extra[i][1]; value.m_count = extra[i][2];
        value.m_bytes = extra[i][3]; value.m_blocks = extra[i][4]; value.m_mode = extra[i][5];
        value.m_name = "pid-" + bbNumber(value.m_processes) + "x" + bbNumber(value.m_threads) + "-" + bbNumber(value.m_bytes);
        if (i == 3)
        {
            value.m_name += "-10million";
        }
        result.push_back(value);
    }
    return result;
}

void ProcessWorkTestAutomation::cross(const std::string& output, const std::string& first, const std::string& second)
{
    directory(output);
    bbWrite(output + "/identity.txt", first + " " + fingerprint(first) + "\n" + second + " " + fingerprint(second));
    const int32_t modes[] = { 0, 1, 4, 5, 6 };
    std::ofstream report((output + "/cross.csv").c_str());
    bbCheck(report.good(), "cannot create cross report");
    report << "mode,deliveries,passed\n";
    for (size_t mode = 0; mode < sizeof(modes) / sizeof(modes[0]); ++mode)
    {
        const std::string prefix = output + "/mesh-" + bbNumber(modes[mode]);
        std::vector<std::shared_ptr<BlackBoxChild>> children;
        const int32_t count = modes[mode] == 4 ? 1000 : 3000;
        for (int32_t i = 0; i < 3; ++i)
        {
            std::shared_ptr<BlackBoxChild> child(new BlackBoxChild);
            std::vector<std::string> args = { "--node", prefix, bbNumber(i), "3", "4", bbNumber(count), bbNumber(modes[mode]), "0", "128" };
            child->startProgram(i == 1 ? second : first, args, prefix + ".log" + bbNumber(i));
            children.push_back(child);
        }
        // 输入为屏障后缀，观察三个自有节点全部发布；提前退出立即失败。
        const std::function<void(const std::string&)> waitFiles = [&](const std::string& suffix)
        {
            bbWait([&]()
            {
                bool ready = true;
                for (size_t i = 0; i < children.size(); ++i)
                {
                    bbCheck(children[i]->poll() == -1, "cross peer exited before " + suffix);
                    ready = !bbRead(prefix + suffix + bbNumber(i)).empty() && ready;
                }
                return ready;
            }, suffix.c_str(), 120000);
        };
        waitFiles(".ready");
        bbWrite(prefix + ".pids.tmp", bbNumber(children[0]->pid()) + " " + bbNumber(children[1]->pid()) + " " + bbNumber(children[2]->pid()));
        bbCheck(std::rename((prefix + ".pids.tmp").c_str(), (prefix + ".pids").c_str()) == 0, "cannot publish complete PID list");
        waitFiles(".armed");
        bbWrite(prefix + ".go", "1");
        waitFiles(".done");
        bbWrite(prefix + ".finish", "1");
        for (size_t i = 0; i < children.size(); ++i)
        {
            bbCheck(children[i]->wait(15000) == 0, "cross peer failure");
        }
        report << modes[mode] << ',' << 3 * 2 * 4 * count << ",1\n" << std::flush;
        bbCheck(report.good(), "cross report write failed");
        std::cout << "cross mode " << modes[mode] << " passed" << std::endl;
    }
}

int32_t ProcessWorkTestAutomation::matrix(int32_t argc, char** argv)
{
    bbCheck(argc >= 5, "matrix OUTPUT ROUNDS BASELINE_OR_- CURRENT [--cases names] [--messages total] [--blocks count] [--custom processes,threads,bytes,blocks,mode]");
    const std::string output(argv[1]);
    const int32_t rounds = Integer(argv[2], 1000);
    bbCheck(rounds > 0, "rounds must be positive");
    std::vector<ProcessWorkTestWorkload> selected = workloads();
    int32_t messages = 0, blocks = 0;
    for (int32_t i = 5; i < argc; i += 2)
    {
        bbCheck(i + 1 < argc, "missing option value");
        const std::string key(argv[i]);
        if (key == "--messages")
        {
            messages = Integer(argv[i + 1], INT32_MAX - 1);
            bbCheck(messages > 0, "messages must be positive");
        }
        else if (key == "--blocks")
        {
            blocks = Integer(argv[i + 1], 4194304);
            bbCheck(blocks > 0, "blocks must be positive");
        }
        else if (key == "--cases")
        {
            const std::vector<std::string> names = Split(argv[i + 1]);
            std::vector<ProcessWorkTestWorkload> found;
            for (size_t n = 0; n < names.size(); ++n)
            {
                for (size_t j = 0; j < selected.size(); ++j)
                {
                    if (selected[j].m_name == names[n])
                    {
                        found.push_back(selected[j]);
                    }
                }
            }
            bbCheck(found.size() == names.size(), "unknown workload");
            selected.swap(found);
        }
        else if (key == "--custom")
        {
            // 自定义五个字段允许值重复，不能使用拒绝重复名称的Split。
            std::istringstream fields(argv[i + 1]);
            std::vector<int32_t> numbers;
            std::string field;
            while (std::getline(fields, field, ','))
            {
                numbers.push_back(Integer(field, INT32_MAX - 1));
            }
            bbCheck(numbers.size() == 5 && numbers[0] >= 1 && numbers[0] <= 8 && numbers[1] >= 1 &&
                numbers[1] <= 32 && numbers[2] <= 65536 && numbers[3] >= 1 && numbers[3] <= 4194304 && numbers[4] <= 3,
                "invalid custom workload");
            ProcessWorkTestWorkload value;
            value.m_name = "custom"; value.m_processes = numbers[0]; value.m_threads = numbers[1];
            value.m_bytes = numbers[2]; value.m_blocks = numbers[3]; value.m_mode = numbers[4]; value.m_count = 10000;
            selected.assign(1, value);
        }
        else
        {
            bbCheck(false, "unknown matrix option: " + key);
        }
    }
    std::vector<std::string> programs, labels;
    if (std::string(argv[3]) != "-")
    {
        programs.push_back(argv[3]); labels.push_back("baseline");
    }
    programs.push_back(argv[4]); labels.push_back("current");
    directory(output);
    std::ostringstream identity;
    for (size_t i = 0; i < programs.size(); ++i)
    {
        identity << labels[i] << ' ' << programs[i] << ' ' << fingerprint(programs[i]) << '\n';
    }
    identity << "PROCESS_WORK_BENCH_UNLIMITED_LOCAL=" << (std::getenv("PROCESS_WORK_BENCH_UNLIMITED_LOCAL") ? "1" : "0");
    bbWrite(output + "/identity.txt", identity.str());
    std::ofstream raw((output + "/samples.csv").c_str());
    std::ofstream summary((output + "/summary.csv").c_str());
    bbCheck(raw.good() && summary.good(), "cannot open matrix reports");
    raw << "case,version,round,passed,produced_per_second,consumed_per_second,log\n";
    summary << "case,version,passed_rounds,failed_rounds,production_median,production_min,production_max,consumption_median,consumption_min,consumption_max,production_change_percent,consumption_change_percent\n";
    bool failed = false;
    for (size_t c = 0; c < selected.size(); ++c)
    {
        ProcessWorkTestWorkload value = selected[c];
        if (messages)
        {
            bbCheck(messages % (value.m_processes * value.m_threads) == 0, "total must divide evenly across producers");
            value.m_count = messages / (value.m_processes * value.m_threads);
        }
        if (blocks)
        {
            value.m_blocks = blocks;
        }
        std::vector<std::vector<double>> production(programs.size()), consumption(programs.size());
        std::vector<int32_t> errors(programs.size(), 0);
        for (int32_t round = 0; round < rounds; ++round)
        {
            for (size_t order = 0; order < programs.size(); ++order)
            {
                const size_t p = round % 2 == 0 ? order : programs.size() - 1 - order;
                const std::string log = output + "/" + value.m_name + "-" + labels[p] + "-" + bbNumber(round + 1) + ".log";
                try
                {
                    std::vector<std::string> args = { bbNumber(value.m_processes), bbNumber(value.m_threads),
                        bbNumber(value.m_count), bbNumber(value.m_bytes), bbNumber(value.m_blocks), bbNumber(value.m_mode) };
                    execute(programs[p], args, log, 240);
                    const std::map<std::string, double> row = parse(bbRead(log));
                    bbCheck(row.at("verified") == 1 && row.at("processes") == value.m_processes &&
                        row.at("threads_per_process") == value.m_threads && row.at("bytes") == value.m_bytes &&
                        row.at("messages") == int64_t(value.m_processes) * value.m_threads * value.m_count &&
                        row.at("blocks") == value.m_blocks && row.at("mode") == value.m_mode &&
                        row.at("produced_per_second") > 0 && row.at("consumed_per_second") > 0, "benchmark verification mismatch");
                    production[p].push_back(row.at("produced_per_second"));
                    consumption[p].push_back(row.at("consumed_per_second"));
                    raw << value.m_name << ',' << labels[p] << ',' << round + 1 << ",1," << std::setprecision(12) <<
                        production[p].back() << ',' << consumption[p].back() << ',' << Csv(log) << '\n' << std::flush;
                }
                catch (const std::exception& error)
                {
                    ++errors[p]; failed = true;
                    raw << value.m_name << ',' << labels[p] << ',' << round + 1 << ",0,,," << Csv(log) << '\n' << std::flush;
                    bbWrite(log + ".failure.txt", error.what());
                    std::cerr << error.what() << std::endl;
                }
                bbCheck(raw.good(), "sample report write failed");
                std::cout << value.m_name << ' ' << labels[p] << " round=" << round + 1 << " finished" << std::endl;
            }
        }
        for (size_t p = 0; p < programs.size(); ++p)
        {
            summary << value.m_name << ',' << labels[p] << ',' << production[p].size() << ',' << errors[p];
            if (errors[p] == 0)
            {
                summary << std::setprecision(12) << ',' << median(production[p]) << ',' << *std::min_element(production[p].begin(), production[p].end()) <<
                    ',' << *std::max_element(production[p].begin(), production[p].end()) << ',' << median(consumption[p]) <<
                    ',' << *std::min_element(consumption[p].begin(), consumption[p].end()) << ',' << *std::max_element(consumption[p].begin(), consumption[p].end());
                if (p > 0 && errors[0] == 0)
                {
                    summary << ',' << (median(production[p]) / median(production[0]) - 1) * 100 <<
                        ',' << (median(consumption[p]) / median(consumption[0]) - 1) * 100;
                }
                else
                {
                    summary << ",,";
                }
            }
            else
            {
                summary << ",,,,,,,,";
            }
            summary << '\n' << std::flush;
            bbCheck(summary.good(), "summary write failed");
        }
    }
    return failed ? 1 : 0;
}

void ProcessWorkTestAutomation::selfTest()
{
    bbCheck(workloads().size() == 23, "workload loss");
    bbCheck(median(std::vector<double>{ 9, 1, 3 }) == 3 && median(std::vector<double>{ 4, 2 }) == 3, "median");
    bbCheck(parse("noise\n{\"verified\":true,\"x\":1.25}\n").at("x") == 1.25, "valid JSON");
    const char* invalid[] = { "", "{\"x\":NaN}", "{\"x\":1,\"x\":2}", "{\"x\":1", "{\"x\":true wrong}", "{\"x\":1}junk" };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        bool rejected = false;
        try
        {
            parse(invalid[i]);
        }
        catch (...)
        {
            rejected = true;
        }
        bbCheck(rejected, "accepted malformed report");
    }
    std::string source("a\nb");
    replaceOnce(source, "a", "c");
    bbCheck(source == "c\nb", "anchor replacement");
    const std::string log = bbTempPath("automation.log");
    execute(bbExecutable(), std::vector<std::string>{ "--exit-peer" }, log, 10);
    bool failed = false;
    try
    {
        execute(bbExecutable(), std::vector<std::string>{ "--automation-fail" }, log, 10);
    }
    catch (...)
    {
        failed = true;
    }
    bbCheck(failed, "nonzero child counted as success");
    std::remove(log.c_str());
}

int32_t ProcessWorkTestAutomation::run(int32_t argc, char** argv)
{
    bbCheck(argc >= 1, "missing tool command");
    const std::string command(argv[0]);
    if (command == "shell" && argc == 2)
    {
        return shell(argv[1]);
    }
    if (command == "matrix")
    {
        return matrix(argc, argv);
    }
    if (command == "cross" && argc == 4)
    {
        cross(argv[1], argv[2], argv[3]);
        return 0;
    }
    if (command == "build" && argc == 5)
    {
        build(argv[1], argv[2], argv[3], argv[4]);
        return 0;
    }
    if (command == "workloads" && argc == 1)
    {
        const std::vector<ProcessWorkTestWorkload> values = workloads();
        for (size_t i = 0; i < values.size(); ++i)
        {
            std::cout << values[i].m_name << std::endl;
        }
        return 0;
    }
    bbCheck(false, "tool commands: workloads | matrix OUTPUT ROUNDS BASELINE_OR_- CURRENT [options] | cross OUTPUT FIRST SECOND | build ROOT OUTPUT x64|x86 tests|dll|benchmark|memory|profile|demo|map");
    return 1;
}