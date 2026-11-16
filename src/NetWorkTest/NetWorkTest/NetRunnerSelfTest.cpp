#include "NetRunnerSelfTest.h"
#include "NetTestProcess.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#endif

/** 读取受控小文本，保留UTF8路径和内容，不把读取失败当作空文本
@param [in] path UTF8路径
@param [out] text 完整文本
@return 文件可完整读取且不超过16MiB返回true
*/
static bool ReadRunnerText(const std::string& path, std::string& text)
{
    std::vector<uint8_t> bytes;
    if (!CSystem::readBinaryFile(CStringManager::Utf8ToUnicode(path), 16 * 1024 * 1024, bytes))
    {
        return false;
    }
    text.assign(bytes.begin(), bytes.end());
    return true;
}

bool NetRunnerSelfTest::declarations(const std::string& source, std::vector<std::string>& result)
{
    std::string clean;
    for (size_t index = 0; index < source.size();)
    {
        if (source.compare(index, 2, "/*") == 0)
        {
            const size_t end = source.find("*/", index + 2);
            if (end == std::string::npos)
            {
                return false;
            }
            clean += ' ';
            index = end + 2;
        }
        else if (source.compare(index, 2, "//") == 0)
        {
            const size_t end = source.find('\n', index + 2);
            clean += ' ';
            index = end == std::string::npos ? source.size() : end + 1;
        }
        else
        {
            clean += source[index++];
        }
    }
    const size_t begin = clean.find("public:");
    const size_t end = begin == std::string::npos ? std::string::npos : clean.find("private:", begin);
    if (begin == std::string::npos || end == std::string::npos)
    {
        return false;
    }
    clean = clean.substr(begin + 7, end - begin - 7);
    const char* owners[] = { "NetPeerNetSender", "NetWorkNetSender" };
    for (size_t owner = 0; owner < 2; ++owner)
    {
        const std::string from(owners[owner]);
        for (size_t position = clean.find(from); position != std::string::npos; position = clean.find(from, position + 9))
        {
            clean.replace(position, from.size(), "NetSender");
        }
    }
    std::vector<std::string> parsed;
    size_t position = 0;
    for (;;)
    {
        const size_t semicolon = clean.find(';', position);
        const std::string section = clean.substr(position, semicolon == std::string::npos ?
            std::string::npos : semicolon - position);
        std::istringstream words(section);
        std::string word;
        std::string item;
        while (words >> word)
        {
            if (!item.empty())
            {
                item += ' ';
            }
            item += word;
        }
        if (semicolon == std::string::npos)
        {
            if (!item.empty())
            {
                return false;
            }
            break;
        }
        if (item.empty())
        {
            return false;
        }
        parsed.push_back(item);
        position = semicolon + 1;
    }
    result.swap(parsed);
    return !result.empty();
}

bool NetRunnerSelfTest::contracts(const std::string& repository)
{
    const char* owners[] = { "NetSender", "NetPeer", "NetWork" };
    std::vector<std::string> reference;
    for (size_t owner = 0; owner < 3; ++owner)
    {
        const std::string name(owners[owner]);
        std::string source;
        std::vector<std::string> actual;
        if (!ReadRunnerText(repository + "/src/" + name + "/" + name + "/src/NetSender.h", source) ||
            !declarations(source, actual) || (owner != 0 && actual != reference))
        {
            return false;
        }
        reference.swap(actual);
    }
    // 反例检查独立于生产头，防止比较器漏掉参数类型、默认值或限定符
    std::vector<std::string> first;
    std::vector<std::string> same;
    std::vector<std::string> changed;
    return declarations("public: int run(int count = 1) const; private:", first) &&
        declarations("public: /* comment */ int run(int count = 1) const; // comment\nprivate:", same) &&
        declarations("public: int run(unsigned count = 1) const; private:", changed) &&
        first == same && first != changed &&
        declarations("public: int run(int count = 2) const; private:", changed) && first != changed &&
        declarations("public: int run(int count = 1); private:", changed) && first != changed;
}

bool NetRunnerSelfTest::reportMatches(const std::string& text, const std::vector<std::string>& expectedRows)
{
    std::istringstream lines(text);
    std::string line;
    if (!std::getline(lines, line) || line != "id,name,state,duration_ms,detail")
    {
        return false;
    }
    std::vector<std::string> rows;
    const char* states[] = { "PASS", "FAIL", "CANCELLED", "NOT_RUN" };
    size_t counts[] = { 0, 0, 0, 0 };
    bool total = false;
    while (std::getline(lines, line))
    {
        if (total || line.empty())
        {
            return false;
        }
        if (line.compare(0, 6, "TOTAL,") == 0)
        {
            std::string expected = "TOTAL," + std::to_string(rows.size());
            for (size_t state = 0; state < 4; ++state)
            {
                expected += std::string(",") + states[state] + "," + std::to_string(counts[state]);
            }
            if (line != expected)
            {
                return false;
            }
            total = true;
            continue;
        }
        std::vector<std::string> fields;
        size_t position = 0;
        for (;;)
        {
            std::string field;
            if (position < line.size() && line[position] == '"')
            {
                ++position;
                bool closed = false;
                while (position < line.size())
                {
                    const char byte = line[position++];
                    if (byte != '"')
                    {
                        field += byte;
                    }
                    else if (position < line.size() && line[position] == '"')
                    {
                        field += '"';
                        ++position;
                    }
                    else
                    {
                        closed = true;
                        break;
                    }
                }
                if (!closed || (position < line.size() && line[position] != ','))
                {
                    return false;
                }
            }
            else
            {
                while (position < line.size() && line[position] != ',')
                {
                    if (line[position] == '"')
                    {
                        return false;
                    }
                    field += line[position++];
                }
            }
            fields.push_back(field);
            if (position == line.size())
            {
                break;
            }
            ++position;
        }
        if (fields.size() != 5 || fields[0].empty() || fields[1].empty() || fields[3].empty() ||
            fields[0].find_first_not_of("0123456789") != std::string::npos ||
            fields[3].find_first_not_of("0123456789") != std::string::npos)
        {
            return false;
        }
        size_t state = 0;
        while (state < 4 && fields[2] != states[state])
        {
            ++state;
        }
        if (state == 4)
        {
            return false;
        }
        ++counts[state];
        rows.push_back(fields[0] + ":" + fields[2]);
    }
    return total && rows == expectedRows;
}

bool NetRunnerSelfTest::nativeReport(const std::string& listing, const std::string& report)
{
    std::istringstream lines(listing);
    std::vector<std::string> expected;
    std::string line;
    uint32_t previous = 0;
    while (std::getline(lines, line))
    {
        std::istringstream fields(line);
        std::string id;
        std::string name;
        uint32_t value = 0;
        if (!(fields >> id >> name) || id.empty() || id.find_first_not_of("0123456789") != std::string::npos)
        {
            return false;
        }
        std::istringstream number(id);
        if (!(number >> value) || value <= previous || std::to_string(value) != id)
        {
            return false;
        }
        previous = value;
        expected.push_back(id + ":PASS");
    }
    return !expected.empty() && reportMatches(report, expected);
}

bool NetRunnerSelfTest::reportChecks()
{
    const std::string prefix = "id,name,state,duration_ms,detail\n";
    const std::string row = "1,\"comma, escaped \"\"quote\"\"\",PASS,3,\"\"\n";
    const std::string total = "TOTAL,1,PASS,1,FAIL,0,CANCELLED,0,NOT_RUN,0\n";
    const std::vector<std::string> expected = { "1:PASS" };
    if (!reportMatches(prefix + row + total, expected))
    {
        return false;
    }
    const std::string invalid[] = {
        prefix + total, prefix + row, prefix + row + total + total, prefix + row + row + total,
        prefix + row + "TOTAL,1,PASS,0,FAIL,1,CANCELLED,0,NOT_RUN,0\n",
        prefix + "1,name,FAIL,3,\"\"\n" + total,
        prefix + "1,name,RUNNING,3,\"\"\n" + total,
        prefix + "1,name,PASS,-1,\"\"\n" + total,
        prefix + "1,\"unclosed,PASS,3,\n" + total,
        prefix + "1,na\"me,PASS,3,\"\"\n" + total,
        prefix + "1,\"name\"suffix,PASS,3,\"\"\n" + total,
        prefix + row + total + "unexpected\n"
    };
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
    {
        if (reportMatches(invalid[index], expected))
        {
            return false;
        }
    }
    const std::string valid = prefix + row + total;
    return reportMatches(prefix + "2,name,CANCELLED,0,reason\n" +
        "TOTAL,1,PASS,0,FAIL,0,CANCELLED,1,NOT_RUN,0\n", { "2:CANCELLED" }) &&
        nativeReport("1 case\n", valid) && !nativeReport("", valid) &&
        !nativeReport("1 case\n2 other\n", valid) && !nativeReport("1 case\n1 duplicate\n", valid) &&
        !nativeReport("2 case\n1 unsorted\n", valid) && !nativeReport("+1 signed\n", valid) &&
        !nativeReport("4294967296 overflow\n", valid) && !nativeReport("01 leading\n", valid);
}

int NetRunnerSelfTest::processProbe(int argc, char** argv)
{
    std::vector<std::string> arguments;
    if (!CSystem::commandArguments(argc, argv, arguments) || arguments.size() < 2 ||
        arguments[0] != "--test-process-probe")
    {
        return 2;
    }
    if (arguments[1] == "wait" && arguments.size() == 2)
    {
        std::fputs("PROCESS_PROBE_READY\n", stdout);
        std::fflush(stdout);
        CSystem::Sleep(30000);
        std::fputs("PROCESS_PROBE_COMPLETED\n", stdout);
        return 0;
    }
#ifdef _WIN32
    if (arguments[1] == "console" && arguments.size() == 3)
    {
        uint32_t pid = 0;
        std::istringstream input(arguments[2]);
        if (!(input >> pid) || pid == 0 || input.peek() != std::char_traits<char>::eof()) { return 2; }
        FreeConsole();
        SetLastError(0);
        const bool attached = AttachConsole(pid) != FALSE;
        const DWORD error = GetLastError();
        if (attached) { FreeConsole(); }
        std::printf("CONSOLE_PROBE pid=%u attached=%d error=%u\n", pid, attached ? 1 : 0, error);
        return !attached && error == ERROR_INVALID_HANDLE ? 0 : 1;
    }
    if ((arguments[1] == "tree" || arguments[1] == "orphan") && arguments.size() == 3)
    {
        uint32_t pid = 0;
        uint64_t monitor = 0;
        if (CSystem::startCapturedProcess(CSystem::currentExecutablePathUtf8(),
            { "--test-process-probe", "wait" }, arguments[2], pid, monitor) != 0)
        {
            return 1;
        }
        std::printf("PROCESS_PROBE_CHILD=%u\n", pid);
        std::fflush(stdout);
        if (arguments[1] == "tree")
        {
            CSystem::Sleep(30000);
        }
        CSystem::closeProcessMonitor(monitor);
        return 0;
    }
#endif
    if (arguments[1] != "echo")
    {
        return 2;
    }
    for (size_t index = 2; index < arguments.size(); ++index)
    {
        std::printf("ARG%u=", static_cast<unsigned>(index - 2));
        for (size_t byte = 0; byte < arguments[index].size(); ++byte)
        {
            std::printf("%02X", static_cast<unsigned>(static_cast<uint8_t>(arguments[index][byte])));
        }
        std::fputc('\n', stdout);
    }
    return 0;
}

void NetRunnerSelfTest::processChecks(const std::string& executable, const std::string& output,
    std::vector<std::pair<int, bool>>& results)
{
    int32_t code = 0;
    NetTestProcess echo;
    std::string text;
    const std::string log = output + "/960-UTF8-\xe4\xb8\xad\xe6\x96\x87.log";
    const bool launched = echo.start(executable,
        { "--test-process-probe", "echo", "", "a b", "q\"r", "tail\\", "a\\\"b", "\xe4\xb8\xad" }, log);
    const bool echoed = launched && echo.wait(15000, code) == 1 && code == 0 && echo.output(text);
    const std::string originalLog = text;
    // Windows CRT写文本stdout使用CRLF，比较前仅统一换行，不丢弃任何参数字节
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    results.push_back(std::make_pair(960, echoed &&
        text == "ARG0=\nARG1=612062\nARG2=712272\nARG3=7461696C5C\nARG4=615C2262\nARG5=E4B8AD\n"));
    results.push_back(std::make_pair(961, launched && echo.poll(code) == 1 && code == 0 &&
        echo.wait(0, code) == 1 && echo.terminate() &&
        !echo.start(executable, { "--list" }, output + "/961-reuse.log")));
    NetTestProcess duplicate;
    std::string unchanged;
    results.push_back(std::make_pair(962, echoed &&
        !duplicate.start(executable, { "--list" }, log) && echo.output(unchanged) &&
        unchanged == originalLog));
    NetTestProcess absent;
    results.push_back(std::make_pair(963, !absent.start(output + "/absent.exe", {}, output + "/963.log") &&
        absent.pid() == 0 && absent.poll(code) == -1 && absent.terminate()));
    NetTestProcess invalid;
    results.push_back(std::make_pair(964,
        !invalid.start(executable, { std::string("a\0b", 3) }, output + "/964.log") && invalid.pid() == 0));
    NetTestProcess waiting;
    const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    const bool started = waiting.start(executable, { "--test-process-probe", "wait" }, output + "/965.log");
    const bool timeout = started && waiting.wait(100, code) == 0 &&
        std::chrono::steady_clock::now() - begin >= std::chrono::milliseconds(100);
    const bool stopped = waiting.terminate() && waiting.poll(code) == 1 && code != 0;
    results.push_back(std::make_pair(965, timeout && stopped));
#ifdef _WIN32
    results.push_back(std::make_pair(968, echo.waitTree(1000) == 1 && absent.waitTree(0) == -1));
    {
        NetTestProcess orphan;
        const bool launchedOrphan = orphan.start(executable,
            { "--test-process-probe", "orphan", output + "/969-descendant.log" }, output + "/969.log");
        const bool parentExited = launchedOrphan && orphan.wait(5000, code) == 1 && code == 0;
        // 父测试正常返回0，后代仍在运行；汇总器必须检测到，不能靠析构清理后冒充正常收尾。
        results.push_back(std::make_pair(969, parentExited && orphan.waitTree(100) == 0));
    }
    {
        NetTestProcess owner;
        NetTestProcess outsider;
        std::vector<uint32_t> owned;
        const bool launchedBoth = owner.start(executable, { "--test-process-probe", "wait" }, output + "/967-owned.log") &&
            outsider.start(executable, { "--test-process-probe", "wait" }, output + "/967-outsider.log");
        const bool isolated = launchedBoth && owner.ownedProcesses(executable, owned) && owned.size() == 1 &&
            owned[0] == owner.pid() && !owner.terminateOwned(outsider.pid(), executable) && outsider.poll(code) == 0 &&
            !owner.terminateOwned(owner.pid(), output + "/missing-image.exe") && owner.poll(code) == 0;
        const bool stoppedOwn = isolated && owner.terminateOwned(owner.pid(), executable) &&
            owner.wait(5000, code) == 1 && code == 125 && owner.ownedProcesses(executable, owned) && owned.empty();
        results.push_back(std::make_pair(967, stoppedOwn && outsider.poll(code) == 0 && outsider.terminate()));
        if (!results.back().second)
        {
            std::fprintf(stderr, "Owned process isolation: started=%d isolated=%d stopped=%d matches=%u exit=%d\n",
                launchedBoth ? 1 : 0, isolated ? 1 : 0, stoppedOwn ? 1 : 0, static_cast<unsigned>(owned.size()), code);
        }
    }
    uint64_t descendant = 0;
    bool ownedTree = false;
    const std::chrono::steady_clock::time_point treeBegin = std::chrono::steady_clock::now();
    {
        NetTestProcess tree;
        const bool treeStarted = tree.start(executable,
            { "--test-process-probe", "tree", output + "/966-descendant.log" }, output + "/966.log");
        const std::chrono::steady_clock::time_point end =
            std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (treeStarted && std::chrono::steady_clock::now() < end && tree.poll(code) == 0)
        {
            if (tree.output(text) && text.find('\n') != std::string::npos &&
                text.compare(0, 20, "PROCESS_PROBE_CHILD=") == 0)
            {
                std::istringstream identity(text.substr(20));
                uint32_t pid = 0;
                if (identity >> pid && pid != 0)
                {
                    descendant = CSystem::openProcessMonitor(pid);
                }
                break;
            }
            CSystem::Sleep(1);
        }
        ownedTree = descendant != 0 && CSystem::pollProcessMonitor(descendant, code) == 0;
    }
    // 回收原内核句柄，不能按PID重新查找并结束任何无关进程。
    const std::chrono::steady_clock::time_point end =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    int32_t state = -1;
    while (descendant != 0 && (state = CSystem::pollProcessMonitor(descendant, code)) == 0 &&
        std::chrono::steady_clock::now() < end)
    {
        CSystem::Sleep(1);
    }
    // KILL_ON_JOB_CLOSE可以给出0退出码；必须用存活转退出、期限和未完成标记证明被回收。
    const bool childLog = ReadRunnerText(output + "/966-descendant.log", text);
    results.push_back(std::make_pair(966, ownedTree && state == 1 && childLog &&
        text.find("PROCESS_PROBE_COMPLETED") == std::string::npos &&
        std::chrono::steady_clock::now() - treeBegin < std::chrono::seconds(10)));
    if (!results.back().second)
    {
        std::fprintf(stderr, "Owned tree observation: live=%d state=%d exit=%d\n", ownedTree ? 1 : 0, state, code);
    }
    CSystem::closeProcessMonitor(descendant);
#endif
}

void NetRunnerSelfTest::batchChecks(const std::string& executable, const std::string& repository,
    const std::string& output, std::vector<std::pair<int, bool>>& results)
{
    const std::string selections[] = { "", ",", "unknown", "wasm", "native", "luma-server" };
    for (uint32_t index = 0; index < 6; ++index)
    {
        const std::string id = std::to_string(950 + index);
        const std::string destination = output + "/batch-" + id;
        NetTestProcess child;
        int32_t code = 0;
        bool passed = child.start(executable, { "--batch", "--bin=" + output + "/absent-bin",
            "--module=" + output + "/absent.js", "--node=" + executable, "--repository=" + repository,
            "--output=" + destination, "--only=" + selections[index] }, output + "/" + id + ".log") &&
            child.wait(15000, code) == 1 && code == (index < 3 ? 2 : 1);
        if (index < 3)
        {
            passed = passed && !CSystem::fileExists(CStringManager::Utf8ToUnicode(destination));
        }
        else
        {
            std::string report;
            const bool missing = index != 4;
            passed = passed && ReadRunnerText(destination + "/result.json", report);
            const std::string prefix = "{\"pass\":false,\"complete\":" + std::string(missing ? "false" : "true") +
                ",\"results\":[{\"suite\":\"" + selections[index] + "\",\"state\":\"" +
                (missing ? "NOT_RUN" : "FAIL") + "\",\"pass\":false,";
            passed = passed && report.compare(0, prefix.size(), prefix) == 0 &&
                report.find("\"suite\":", report.find("\"suite\":") + 1) == std::string::npos &&
                report.size() >= 4 && report.substr(report.size() - 4) == "}]}\n";
        }
        results.push_back(std::make_pair(950 + index, passed));
    }
}

bool NetRunnerSelfTest::check(const std::string& executable, const std::string& output, int id,
    const std::vector<std::string>& arguments, int expectedCode,
    const std::vector<std::string>& expectedRows, bool listed)
{
    const std::string stem = output + "/" + std::to_string(id);
    std::vector<std::string> command(arguments);
    if (!expectedRows.empty())
    {
        command.push_back("--report=" + stem + ".csv");
    }
    NetTestProcess child;
    int32_t code = 0;
    std::string text;
    if (!child.start(executable, command, stem + ".log") || child.wait(15000, code) != 1 ||
        code != expectedCode || !child.output(text))
    {
        return false;
    }
    if (listed)
    {
        std::istringstream lines(text);
        std::string line;
        int previous = 0;
        while (std::getline(lines, line))
        {
            std::istringstream fields(line);
            int value = 0;
            std::string name;
            if (!(fields >> value >> name) || value <= previous)
            {
                return false;
            }
            previous = value;
        }
        if (previous == 0)
        {
            return false;
        }
    }
    if (!expectedRows.empty() && (!ReadRunnerText(stem + ".csv", text) || !reportMatches(text, expectedRows)))
    {
        return false;
    }
    return true;
}

int NetRunnerSelfTest::run(int argc, char** argv)
{
    std::vector<std::string> arguments;
    if (!CSystem::commandArguments(argc, argv, arguments))
    {
        return 2;
    }
    std::string repository;
    std::string output;
    for (size_t index = 1; index < arguments.size(); ++index)
    {
        const std::string& value = arguments[index];
        if (value.compare(0, 13, "--repository=") == 0 && repository.empty() && value.size() > 13)
        {
            repository = value.substr(13);
        }
        else if (value.compare(0, 9, "--output=") == 0 && output.empty() && value.size() > 9)
        {
            output = value.substr(9);
        }
        else
        {
            return 2;
        }
    }
    if (repository.empty() || output.empty() ||
        CSystem::fileExists(CStringManager::Utf8ToUnicode(output)) || !CSystem::createDirectoriesUtf8(output))
    {
        return 2;
    }
    const std::string executable = CSystem::currentExecutablePathUtf8();
    std::vector<std::pair<int, bool>> results;
    results.push_back(std::make_pair(900, contracts(repository)));
    results.push_back(std::make_pair(901, check(executable, output, 901, { "--list" }, 0, {}, true)));
    results.push_back(std::make_pair(902, check(executable, output, 902, { "--cases=63,1,63" }, 0, { "1:PASS", "63:PASS" })));
    const std::vector<std::vector<std::string>> invalid = {
        {}, { "--cases=" }, { "--cases=1," }, { "--cases=1,,2" }, { "--cases=3-1" },
        { "--cases=-1" }, { "--cases=0" }, { "--cases=9999" }, { "--cases=1x" }, { "--cases=1-1000000000" },
        { "--all", "--instant" }, { "--cases=1", "--cases=2" }, { "--list", "--all" },
        { "--runner-probe=cancel", "--all" }, { "--unknown" }, { "--cases=1", "--report=" },
        { "--cases=1", "--report=x", "--report=y" }, { "--runner-probe=fail", "--runner-probe=cancel" }
    };
    for (size_t index = 0; index < invalid.size(); ++index)
    {
        const int id = 910 + static_cast<int>(index);
        results.push_back(std::make_pair(id, check(executable, output, id, invalid[index], 2, {})));
    }
    results.push_back(std::make_pair(940, check(executable, output, 940, { "--runner-probe=fail" }, 1, { "9001:FAIL" })));
    results.push_back(std::make_pair(941, check(executable, output, 941, { "--runner-probe=timeout" }, 1, { "9001:FAIL" })));
    results.push_back(std::make_pair(942, check(executable, output, 942, { "--runner-probe=cancel" }, 1, { "9001:CANCELLED" })));
    results.push_back(std::make_pair(943, check(executable, output, 943,
        { "--cases=1", "--report=" + output + "/absent/report.csv" }, 3, {})));
    results.push_back(std::make_pair(903, reportChecks()));
    const std::vector<std::vector<std::string>> invalidBuild = {
        {}, { "--target" }, { "--target=" }, { "--target=protocol", "--target=wasm-tests" },
        { "--unknown=value" }, { "--bits=32", "--bits=64" }, { "--output=" },
        { "--target=protocol", "--bits=32" }
    };
    for (size_t index = 0; index < invalidBuild.size(); ++index)
    {
        const int id = 980 + static_cast<int>(index);
        std::vector<std::string> command(1, "--build-tests");
        command.insert(command.end(), invalidBuild[index].begin(), invalidBuild[index].end());
        results.push_back(std::make_pair(id, check(executable, output, id, command, 2, {})));
    }
    processChecks(executable, output, results);
    batchChecks(executable, repository, output, results);
    std::string report("id,state\n");
    bool passed = true;
    for (size_t index = 0; index < results.size(); ++index)
    {
        const char* state = results[index].second ? "PASS" : "FAIL";
        passed = passed && results[index].second;
        report += std::to_string(results[index].first) + "," + state + "\n";
        std::printf("%d %s\n", results[index].first, state);
    }
    intptr_t file = CSystem::openFileHandle((output + "/runner.csv").c_str(), 2);
    if (file == -1)
    {
        return 3;
    }
    const bool written = CSystem::writeFileHandle(file, 0, reinterpret_cast<const uint8_t*>(report.data()),
        static_cast<uint32_t>(report.size()));
    const bool closed = CSystem::closeFileHandle(&file);
    return !written || !closed ? 3 : passed ? 0 : 1;
}