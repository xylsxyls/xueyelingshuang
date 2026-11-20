#include "LockFreeHashMapRunner.h"
#include <algorithm>
#include "LockFreeHashMapTestThread.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <thread>
#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp.lib")
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char** environ;
#endif

bool LockFreeHashMapRunner::number(const char* text, int maximum, int& value)
{
    if (text == nullptr || *text == 0 || maximum < 1)
    {
        return false;
    }
    int parsed = 0;
    for (const char* next = text; *next != 0; ++next)
    {
        if (*next < '0' || *next > '9' || parsed > (maximum - (*next - '0')) / 10)
        {
            return false;
        }
        parsed = parsed * 10 + (*next - '0');
        if (parsed > maximum)
        {
            return false;
        }
    }
    if (parsed == 0)
    {
        return false;
    }
    value = parsed;
    return true;
}

/** 严格校验非零检查数及零失败的汇总，拒绝多余或重复字段
@param [in] line 完整汇总行
@param [in] marker RESULT或MODULE_RESULT前缀
@return 完整且通过为true
*/
static bool ValidSummary(const std::string& line, const std::string& marker)
{
    std::istringstream fields(line);
    std::string prefix, checks, failures, tail;
    int count = 0;
    return (fields >> prefix >> checks >> failures) && !(fields >> tail) && prefix == marker &&
        checks.compare(0, 7, "checks=") == 0 &&
        LockFreeHashMapRunner::number(checks.c_str() + 7, (std::numeric_limits<int>::max)(), count) && failures == "failed=0";
}

bool LockFreeHashMapRunner::verifyCase(std::istream& input, int id)
{
    std::string line;
    int stage = 0;
    while (std::getline(input, line))
    {
        if (line.compare(0, 11, "NOT_RUN id=") == 0 || line.compare(0, 5, "FAIL ") == 0 ||
            line.compare(0, 11, "UNEXPECTED ") == 0 || line.compare(0, 14, "MODULE_RESULT ") == 0)
        {
            return false;
        }
        if (line.compare(0, 6, "BEGIN ") == 0)
        {
            if (stage != 0 || line != "BEGIN id=" + std::to_string(id))
            {
                return false;
            }
            stage = 1;
        }
        else if (line.compare(0, 10, "RESOURCES ") == 0)
        {
            if (stage != 1)
            {
                return false;
            }
            std::istringstream fields(line);
            std::string field;
            size_t deltas = 0;
            while (fields >> field)
            {
                if (field.compare(0, 17, "allocation_delta=") == 0)
                {
                    if (++deltas != 1 || field != "allocation_delta=0")
                    {
                        return false;
                    }
                }
            }
            if (deltas != 1)
            {
                return false;
            }
            stage = 2;
        }
        else if (line.compare(0, 4, "END ") == 0)
        {
            if (stage != 2 || line != "END id=" + std::to_string(id) + " result=PASS")
            {
                return false;
            }
            stage = 3;
        }
        else if (line.compare(0, 7, "RESULT ") == 0)
        {
            if (stage != 3 || !ValidSummary(line, "RESULT"))
            {
                return false;
            }
            stage = 4;
        }
    }
    return input.eof() && stage == 4;
}

bool LockFreeHashMapRunner::verifyModule(std::istream& input, const std::string& mode)
{
    std::string line;
    size_t summaries = 0, poolDeltas = 0, rewriteDeltas = 0;
    while (std::getline(input, line))
    {
        // 模块自己的失败记录独立于汇总，不能被后续成功行覆盖。
        if (line.find("EPOCH_FAILURE ") == 0 || line.find("POOL_FAILURE ") == 0 || line.find("REWRITE_FAILURE ") == 0 ||
            line.find("REWRITE_EXCEPTION ") == 0 || line.find("ALIGNMENT_FAILURE ") == 0 || line.find("STD_CHECK_FAILED ") == 0 ||
            line.find("NOT_RUN id=") == 0 || line.find("BEGIN id=") == 0 || line.find("RESULT ") == 0)
        {
            return false;
        }
        if (line.compare(0, 14, "MODULE_RESULT ") == 0)
        {
            const std::string expected = mode == "rewrite" && summaries == 0 ? "pool" : mode;
            const std::string prefix = "MODULE_RESULT id=" + expected + " ";
            if (line.compare(0, prefix.size(), prefix) != 0 || !ValidSummary("MODULE_RESULT " + line.substr(prefix.size()), "MODULE_RESULT"))
            {
                return false;
            }
            ++summaries;
        }
        else if (line.find("POOL_ALLOCATION_DELTA ") == 0)
        {
            if (line != "POOL_ALLOCATION_DELTA 0" || ++poolDeltas != 1)
            {
                return false;
            }
        }
        else if (line.find("REWRITE_ALLOCATION_DELTA ") == 0)
        {
            if (line != "REWRITE_ALLOCATION_DELTA 0" || ++rewriteDeltas != 1)
            {
                return false;
            }
        }
    }
    bool pool = mode == "pool" || mode == "rewrite";
    bool rewrite = mode == "rewrite";
    return (pool || mode == "epoch" || mode == "progress" || mode == "std-checks") &&
        input.eof() && summaries == (rewrite ? 2u : 1u) &&
        poolDeltas == (pool ? 1u : 0u) && rewriteDeltas == (rewrite ? 1u : 0u);
}

LockFreeHashMapRunResult LockFreeHashMapRunner::run(const std::string& executable,
    const std::string& argument, const std::string& log, unsigned timeoutSeconds, unsigned affinityMask, unsigned secondArgument,
    bool aboveNormal)
{
    LockFreeHashMapRunResult result = { -1, 0 };
#if !defined(_WIN32)
    if (affinityMask != 0 || aboveNormal)
    {
        return result;
    }
#endif
    if (executable.empty() || executable.find('"') != std::string::npos ||
        timeoutSeconds == 0 || timeoutSeconds > 3600)
    {
        return result;
    }
    int validated = 0;
    if ((!argument.empty() && !number(argument.c_str(), 64999, validated)) ||
        secondArgument > 256 || (secondArgument != 0 && argument.empty()))
    {
        return result;
    }
    // 数字第二参数单独校验并分配，不能放开第一参数的空白和任意命令文本限制
    std::string second = secondArgument == 0 ? std::string() : std::to_string(secondArgument);
#if defined(_WIN32)
    ULONGLONG begin = GetTickCount64();
    // 先完成可能抛出的命令缓冲分配，再取得需要显式释放的系统句柄
    std::string command = "\"" + executable + "\"" + (argument.empty() ? "" : " " + argument) +
        (second.empty() ? "" : " " + second);
    std::vector<char> buffer(command.begin(), command.end());
    buffer.push_back(0);
    SECURITY_ATTRIBUTES security = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    HANDLE output = CreateFileA(log.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (output == INVALID_HANDLE_VALUE)
    {
        return result;
    }
    HANDLE input = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdOutput = output;
    startup.hStdError = output;
    startup.hStdInput = input;
    PROCESS_INFORMATION child = {};
    bool suspended = affinityMask != 0 || aboveNormal;
    if (input != INVALID_HANDLE_VALUE && CreateProcessA(executable.c_str(), &buffer[0], nullptr,
        nullptr, TRUE, CREATE_NO_WINDOW | (suspended ? CREATE_SUSPENDED : 0) |
        (aboveNormal ? ABOVE_NORMAL_PRIORITY_CLASS : 0), nullptr, nullptr, &startup, &child))
    {
        bool admitted = true;
        if (affinityMask != 0)
        {
            // 在用户代码运行前统一 CPU 集合；设置或复核失败不能退回无约束计时
            DWORD_PTR actual = 0;
            DWORD_PTR system = 0;
            admitted = SetProcessAffinityMask(child.hProcess, affinityMask) != FALSE &&
                GetProcessAffinityMask(child.hProcess, &actual, &system) != FALSE && actual == affinityMask;
        }
        if (aboveNormal)
        {
            DWORD actual = GetPriorityClass(child.hProcess);
            admitted = admitted && actual == ABOVE_NORMAL_PRIORITY_CLASS;
            std::fprintf(stdout, "CHILD_PRIORITY requested=AboveNormal actual=%lu verified=%d\n", actual, admitted != false);
        }
        if (suspended)
        {
            admitted = admitted && ResumeThread(child.hThread) == 1;
        }
        DWORD waited = admitted ? WaitForSingleObject(child.hProcess, timeoutSeconds * 1000UL) : WAIT_FAILED;
        if (waited == WAIT_TIMEOUT || waited == WAIT_FAILED)
        {
            if (waited == WAIT_TIMEOUT)
            {
                try
                {
                    // 普通控制器也保留超时现场，不必依靠调试器改变正常调度后才能复现。
                    std::string dumpPath = log + "-timeout.dmp";
                    HANDLE dump = CreateFileA(dumpPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
                    if (dump != INVALID_HANDLE_VALUE)
                    {
                        BOOL saved = MiniDumpWriteDump(child.hProcess, child.dwProcessId, dump,
                            static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithThreadInfo), nullptr, nullptr, nullptr);
                        DWORD error = saved ? 0 : GetLastError();
                        CloseHandle(dump);
                        std::fprintf(stdout, "TIMEOUT_DUMP saved=%d error=%lu path=%s\n", saved != FALSE, error, dumpPath.c_str());
                    }
                    else
                    {
                        std::fprintf(stdout, "TIMEOUT_DUMP open_error=%lu\n", GetLastError());
                    }
                }
                catch (...)
                {
                    std::fprintf(stdout, "TIMEOUT_DUMP allocation_failed\n");
                }
            }
            // 持有创建时取得的进程句柄，不按可复用 PID 终止其他进程
            if (TerminateProcess(child.hProcess, 124))
            {
                WaitForSingleObject(child.hProcess, 5000);
            }
            result.m_code = waited == WAIT_TIMEOUT ? -2 : -1;
        }
        else if (waited == WAIT_OBJECT_0)
        {
            DWORD code = 0;
            if (GetExitCodeProcess(child.hProcess, &code))
            {
                result.m_code = static_cast<int>(code);
            }
        }
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
    }
    if (input != INVALID_HANDLE_VALUE)
    {
        CloseHandle(input);
    }
    if (!FlushFileBuffers(output))
    {
        result.m_code = -1;
    }
    CloseHandle(output);
#else
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    int output = open(log.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (output < 0)
    {
        return result;
    }
    // 标准描述符可能已被宿主关闭；先复制到3以上，后续关闭原始输出不能关闭子进程stdout。
    if (output <= STDERR_FILENO)
    {
        int duplicate = fcntl(output, F_DUPFD, STDERR_FILENO + 1);
        close(output);
        output = duplicate;
        if (output < 0)
        {
            return result;
        }
    }
    posix_spawn_file_actions_t actions;
    int initialized = posix_spawn_file_actions_init(&actions);
    int configured = initialized;
    if (configured == 0)
    {
        configured = posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    }
    if (configured == 0)
    {
        configured = posix_spawn_file_actions_adddup2(&actions, output, STDOUT_FILENO);
    }
    if (configured == 0)
    {
        configured = posix_spawn_file_actions_adddup2(&actions, output, STDERR_FILENO);
    }
    if (configured == 0)
    {
        configured = posix_spawn_file_actions_addclose(&actions, output);
    }
    char* arguments[] = { const_cast<char*>(executable.c_str()),
        argument.empty() ? nullptr : const_cast<char*>(argument.c_str()),
        second.empty() ? nullptr : const_cast<char*>(second.c_str()), nullptr };
    pid_t child = 0;
    if (configured == 0 && posix_spawn(&child, executable.c_str(), &actions, nullptr, arguments, environ) == 0)
    {
        for (;;)
        {
            int status = 0;
            pid_t waited = waitpid(child, &status, WNOHANG);
            if (waited == child)
            {
                result.m_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                break;
            }
            if (waited < 0 && errno != EINTR)
            {
                break;
            }
            if (std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count() >= timeoutSeconds)
            {
                // 未回收子进程的 PID 尚不可复用，只清理本次创建的子进程
                kill(child, SIGKILL);
                while (waitpid(child, &status, 0) < 0 && errno == EINTR)
                {

                }
                result.m_code = -2;
                break;
            }
            usleep(10000);
        }
    }
    if (initialized == 0)
    {
        posix_spawn_file_actions_destroy(&actions);
    }
    int flushed = fsync(output);
    int closed = close(output);
    if (flushed != 0 || closed != 0)
    {
        result.m_code = -1;
    }
#endif
#if defined(_WIN32)
    result.m_seconds = double(GetTickCount64() - begin) / 1000;
#else
    result.m_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
#endif
    return result;
}

bool LockFreeHashMapRunner::cases(const std::string& executable, const std::string& prefix,
    const std::vector<int>& ids, int repeats)
{
    if (ids.empty() || repeats < 1 || repeats > 100)
    {
        return false;
    }
    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (ids[i] < 1 || ids[i] > 11)
        {
            return false;
        }
    }
    std::ofstream report;
    if (!openReport(prefix + "-cases.csv", report))
    {
        return false;
    }
    report << "id,repeat,exit_code,verified,seconds,log\n";
    bool passed = true;
    for (int repeat = 1; repeat <= repeats; ++repeat)
    {
        for (size_t i = 0; i < ids.size(); ++i)
        {
            std::ostringstream name;
            name << prefix << "-id" << ids[i] << "-r" << repeat << ".log";
            std::ostringstream argument;
            argument << ids[i];
            LockFreeHashMapRunResult result = run(executable, argument.str(), name.str(), 120);
            std::ifstream log(name.str().c_str());
            bool verified = verifyCase(log, ids[i]);
            report << ids[i] << ',' << repeat << ',' << result.m_code << ',' << verified << ',' << result.m_seconds << ',' << name.str() << '\n';
            report.flush();
            passed = result.m_code == 0 && verified && bool(report) && passed;
            std::cout << "CASE id=" << ids[i] << " repeat=" << repeat << " exit=" << result.m_code << std::endl;
        }
    }
    report.close();
    return passed && bool(report);
}

bool LockFreeHashMapRunner::workloads(const std::string& executable,
    const std::string& prefix, int scale, int rounds, unsigned affinityMask)
{
    if (executable.empty() || scale < 1 || scale > 20 || rounds < 1 || rounds > 9)
    {
        return false;
    }
    const char* workloads[10] = { "read", "cold_after_hot_updates", "mixed_95read", "mixed_50read", "collision_95read",
        "zero_hash_read", "zero_hash_mixed", "owned_95read", "owned_50read", "owned_zero_hash" };
    const int iterations[10] = { 200000, 20000, 100000, 100000, 20000, 100000, 20000, 100000, 100000, 20000 };
    const int threads[10] = { 1, 1, 4, 4, 4, 1, 4, 4, 4, 4 };
    const int buckets[10] = { 1024, 1, 1024, 1024, 1, 1, 1, 1024, 1024, 1 };
    const int writeEvery[10] = { 0, 0, 20, 2, 20, 0, 20, 20, 2, 20 };
    std::ofstream report;
    if (!openReport(prefix + "-runs.csv", report))
    {
        return false;
    }
    report << "version,round,exit_code,seconds,affinity_mask,log\n";
    std::map<std::string, std::vector<double>> measurements;
    bool passed = true;
    for (int round = 0; round < rounds; ++round)
    {
        std::ostringstream name;
        name << prefix << '-' << "current" << "-r" << round + 1 << ".log";
        std::ostringstream argument;
        argument << scale;
        LockFreeHashMapRunResult result = run(executable, argument.str(), name.str(), 300, affinityMask);
        report << "current" << ',' << round + 1 << ',' << result.m_code << ',' << result.m_seconds << ',' << affinityMask << ',' << name.str() << '\n';
        report.flush();
        passed = result.m_code == 0 && bool(report) && passed;
        std::cout << "COMPARE version=" << "current" << " round=" << round + 1 << " exit=" << result.m_code << std::endl;
        if (result.m_code != 0)
        {
            continue;
        }
        std::ifstream input(name.str().c_str());
        std::string line;
        size_t count = 0;
        std::map<std::string, bool> seen;
        while (std::getline(input, line))
        {
            if (line.compare(0, 9, "WORKLOAD ") != 0)
            {
                continue;
            }
            std::istringstream fields(line);
            std::string field;
            std::string workload;
            double rate = 0;
            bool valid = true;
            std::map<std::string, double> numbers;
            fields >> field;
            while (fields >> field)
            {
                size_t separator = field.find('=');
                if (separator == std::string::npos)
                {
                    valid = false;
                    break;
                }
                const std::string name = field.substr(0, separator);
                const std::string value = field.substr(separator + 1);
                if (name == "name")
                {
                    if (!workload.empty() || value.empty())
                    {
                        valid = false;
                        break;
                    }
                    workload = value;
                    continue;
                }
                double parsed = 0;
                char tail = 0;
                std::istringstream number(value);
                if (!(number >> parsed) || (number >> tail) || !(parsed >= 0 && parsed < 1e100) || numbers.count(name))
                {
                    valid = false;
                    break;
                }
                numbers[name] = parsed;
            }
            size_t index = 0;
            while (index < 10 && workload != workloads[index])
            {
                ++index;
            }
            const char* required[] = { "threads", "buckets", "iterations", "write_every", "hot_history", "ops_per_second",
                "p50_us", "p95_us", "p99_us", "retained_units", "after_clear_units", "errors" };
            for (size_t i = 0; i < sizeof(required) / sizeof(required[0]); ++i)
            {
                valid = numbers.count(required[i]) != 0 && valid;
            }
            rate = numbers["ops_per_second"];
            if (index == 10 || !valid || seen[workload] || rate <= 0 || numbers["errors"] != 0 ||
                numbers["threads"] != threads[index] || numbers["buckets"] != buckets[index] ||
                numbers["iterations"] != iterations[index] * scale || numbers["write_every"] != writeEvery[index] ||
                numbers["hot_history"] != (index == 1 ? 10000 : 0) ||
                numbers["p50_us"] > numbers["p95_us"] || numbers["p95_us"] > numbers["p99_us"])
            {
                passed = false;
                continue;
            }
            seen[workload] = true;
            measurements[workload].push_back(rate);
            ++count;
        }
        passed = input.eof() && count == 10 && passed;
    }
    report.close();
    if (!report || !passed)
    {
        return false;
    }
    std::ofstream summary;
    if (!openReport(prefix + "-comparison.csv", summary))
    {
        return false;
    }
    summary << "workload,current_ops_s\n";
    for (auto item = measurements.begin(); item != measurements.end(); ++item)
    {
        std::vector<double>& values = item->second;
        if (values.size() != static_cast<size_t>(rounds))
        {
            return false;
        }
        std::sort(values.begin(), values.end());
        summary << std::setprecision(12) << item->first << ',' <<
            (values[(values.size() - 1) / 2] + values[values.size() / 2]) / 2 << '\n';
    }
    summary.close();
    return bool(summary);
}

/** 解析控制器命令并捕获分配等设施异常
@param [in] argc 参数数量
@param [in] argv cases 程序 前缀 重复次数 [编号列表]，或 workloads 程序 前缀 倍率 轮数
@return 0 全部通过，1 测试失败，2 输入错误，3 设施异常
*/
#if !defined(LOCKFREEHASHMAP_RUNNER_NO_MAIN)
int main(int argc, char** argv)
{
    try
    {
        // 控制器自检子进程，不运行 map，不注入主动崩溃
        int fixture = 0;
        int second = 0;
        if (argc == 3 && LockFreeHashMapRunner::number(argv[1], 3, fixture) &&
            LockFreeHashMapRunner::number(argv[2], 256, second))
        {
            std::cout << "FIXTURE first=" << fixture << " second=" << second << std::endl;
            return fixture == 1 && second == 256 ? 0 : 7;
        }
        if (argc == 2 && LockFreeHashMapRunner::number(argv[1], 4, fixture))
        {
            if (fixture == 4)
            {
#if defined(_WIN32)
                DWORD actual = GetPriorityClass(GetCurrentProcess());
                std::cout << "FIXTURE priority_class=" << actual << std::endl;
                return actual == ABOVE_NORMAL_PRIORITY_CLASS ? 0 : 7;
#else
                return 7;
#endif
            }
            if (fixture == 3)
            {
#if defined(_WIN32)
                Sleep(3000);
#else
                sleep(3);
#endif
            }
            std::cout << "FIXTURE completed=" << fixture << std::endl;
            return fixture == 2 ? 7 : 0;
        }
        if (argc == 3 && std::strcmp(argv[1], "selftest") == 0)
        {
            std::string prefix(argv[2]);
            LockFreeHashMapRunResult success = LockFreeHashMapRunner::run(argv[0], "1", prefix + "-success.log", 5);
            LockFreeHashMapRunResult failure = LockFreeHashMapRunner::run(argv[0], "2", prefix + "-failure.log", 5);
            LockFreeHashMapRunResult timeout = LockFreeHashMapRunner::run(argv[0], "3", prefix + "-timeout.log", 1);
            LockFreeHashMapRunResult duplicate = LockFreeHashMapRunner::run(argv[0], "1", prefix + "-success.log", 5);
            LockFreeHashMapRunResult paired = LockFreeHashMapRunner::run(argv[0], "1", prefix + "-paired.log", 5, 0, 256);
            LockFreeHashMapRunResult badSecond = LockFreeHashMapRunner::run(argv[0], "1", prefix + "-bad-second.log", 5, 0, 257);
            LockFreeHashMapRunResult missingFirst = LockFreeHashMapRunner::run(argv[0], "", prefix + "-missing-first.log", 5, 0, 1);
            std::ofstream evidence;
            bool evidenceChecked = LockFreeHashMapRunner::openReport(prefix + "-exclusive.txt", evidence);
            evidence << "preserved";
            evidence.close();
            evidenceChecked = evidenceChecked && bool(evidence);
            std::ofstream duplicateReport;
            evidenceChecked = !LockFreeHashMapRunner::openReport(prefix + "-exclusive.txt", duplicateReport) && evidenceChecked;
            std::ifstream savedEvidence((prefix + "-exclusive.txt").c_str());
            std::string savedText;
            savedEvidence >> savedText;
            evidenceChecked = bool(savedEvidence) && savedText == "preserved" && evidenceChecked;
            int parsed = 99;
            int failuresBefore = LockFreeHashMapTestThread::failures();
            {
                // 无参数线程故意抛整数，包装器记录失败后正常退出
                LockFreeHashMapTestThread failing([]()
                {
                    throw 7;
                });
            }
            std::atomic<int> completed(0);
            try
            {
                // 无参数线程写独立原子计数，模拟所有者在后续创建步骤遇到异常
                LockFreeHashMapTestThread active([&]()
                {
                    completed.fetch_add(1);
                });
                throw 7;
            }
            catch (int)
            {

            }
            std::vector<int> fakeIds(1, 1);
            bool rejectedIncomplete = !LockFreeHashMapRunner::cases(argv[0], prefix + "-incomplete", fakeIds, 1);
            const std::string validCase = "BEGIN id=1\nRESOURCES allocation_delta=0 cpu_seconds=0\nEND id=1 result=PASS\nRESULT checks=1 failed=0\n";
            std::istringstream completeCase(validCase);
            bool parserChecked = LockFreeHashMapRunner::verifyCase(completeCase, 1);
            const std::string invalidCases[] = { validCase + "NOT_RUN id=1 reason=single_thread_host\n",
                validCase + "FAIL business check\n", validCase + "UNEXPECTED exception\n", validCase + "MODULE_RESULT id=epoch checks=2 failed=0\n",
                validCase + "END id=1 result=FAIL\n", validCase + "RESULT checks=1 failed=1\n",
                validCase + "RESULT checks=1 failed=0\n", "BEGIN id=1\nEND id=1 result=PASS\nRESULT checks=1 failed=0\n",
                "BEGIN id=1\nRESOURCES allocation_delta=0 allocation_delta=1\nEND id=1 result=PASS\nRESULT checks=1 failed=0\n",
                "BEGIN id=1\nRESOURCES allocation_delta=0\nEND id=1 result=PASS\nRESULT checks=0 failed=0\n" };
            for (size_t i = 0; i < sizeof(invalidCases) / sizeof(invalidCases[0]); ++i)
            {
                std::istringstream invalid(invalidCases[i]);
                parserChecked = !LockFreeHashMapRunner::verifyCase(invalid, 1) && parserChecked;
            }
            std::istringstream wrongIdentity(validCase);
            std::istringstream wrongModule("MODULE_RESULT id=std-checks checks=1 failed=0\n");
            std::istringstream completeModule("POOL_ALLOCATION_DELTA 0\nREWRITE_ALLOCATION_DELTA 0\nMODULE_RESULT id=pool checks=1 failed=0\nMODULE_RESULT id=rewrite checks=2 failed=0\n");
            std::istringstream missingSummary("POOL_ALLOCATION_DELTA 0\nREWRITE_ALLOCATION_DELTA 0\nMODULE_RESULT id=rewrite checks=2 failed=0\n");
            std::istringstream missingDelta("POOL_ALLOCATION_DELTA 0\nMODULE_RESULT id=pool checks=1 failed=0\nMODULE_RESULT id=rewrite checks=2 failed=0\n");
            std::istringstream contradictedModule("MODULE_RESULT id=pool checks=1 failed=1\nMODULE_RESULT id=rewrite checks=2 failed=0\n");
            std::istringstream failedPayload("POOL_FAILURE missing payload\nPOOL_ALLOCATION_DELTA 0\nMODULE_RESULT id=pool checks=1 failed=0\n");
            std::istringstream mixedSuite("MODULE_RESULT id=epoch checks=2 failed=0\nRESULT checks=2 failed=0\n");
            std::istringstream emptyModule("MODULE_RESULT id=epoch checks=0 failed=0\n");
            parserChecked = !LockFreeHashMapRunner::verifyCase(wrongIdentity, 2) &&
                LockFreeHashMapRunner::verifyModule(completeModule, "rewrite") &&
                !LockFreeHashMapRunner::verifyModule(missingSummary, "rewrite") && !LockFreeHashMapRunner::verifyModule(missingDelta, "rewrite") && !LockFreeHashMapRunner::verifyModule(contradictedModule, "rewrite") &&
                !LockFreeHashMapRunner::verifyModule(failedPayload, "pool") && !LockFreeHashMapRunner::verifyModule(mixedSuite, "epoch") &&
                !LockFreeHashMapRunner::verifyModule(emptyModule, "epoch") && !LockFreeHashMapRunner::verifyModule(wrongModule, "progress") && parserChecked;
            bool affinityChecked = true;
            bool dumpChecked = true;
            bool priorityChecked = true;
#if defined(_WIN32)
            LockFreeHashMapRunResult prioritized = LockFreeHashMapRunner::run(argv[0], "4", prefix + "-priority.log", 5, 0, 0, true);
            priorityChecked = prioritized.m_code == 0;
            std::ifstream timeoutDump((prefix + "-timeout.log-timeout.dmp").c_str(), std::ios::binary);
            MINIDUMP_HEADER dumpHeader = {};
            timeoutDump.read(reinterpret_cast<char*>(&dumpHeader), sizeof(dumpHeader));
            dumpChecked = bool(timeoutDump) && dumpHeader.Signature == MINIDUMP_SIGNATURE && dumpHeader.NumberOfStreams != 0;
            DWORD_PTR available = 0;
            DWORD_PTR system = 0;
            affinityChecked = GetProcessAffinityMask(GetCurrentProcess(), &available, &system) != FALSE;
            DWORD_PTR selected = available & (~available + 1);
            affinityChecked = affinityChecked && selected != 0 && selected <= (std::numeric_limits<unsigned>::max)();
            if (affinityChecked)
            {
                LockFreeHashMapRunResult pinned = LockFreeHashMapRunner::run(argv[0], "1", prefix + "-affinity.log", 5, static_cast<unsigned>(selected));
                affinityChecked = pinned.m_code == 0;
            }
#endif
            bool valid = success.m_code == 0 && failure.m_code == 7 && timeout.m_code == -2 && duplicate.m_code == -1 &&
                paired.m_code == 0 && badSecond.m_code == -1 && missingFirst.m_code == -1 &&
                LockFreeHashMapTestThread::failures() == failuresBefore + 1 && completed.load() == 1 && rejectedIncomplete && parserChecked && evidenceChecked && affinityChecked && dumpChecked && priorityChecked &&
                !LockFreeHashMapRunner::number("1x", 10, parsed) && !LockFreeHashMapRunner::number("", 10, parsed) &&
                !LockFreeHashMapRunner::number("99999999999999999999999", 10, parsed) && parsed == 99;
            std::ofstream report;
            if (!LockFreeHashMapRunner::openReport(prefix + "-selftest.txt", report))
            {
                return 1;
            }
            report << "success=" << success.m_code << " failure=" << failure.m_code << " timeout=" << timeout.m_code
                << " duplicate_log=" << duplicate.m_code << " parser=" << valid
                << " incomplete_rejected=" << rejectedIncomplete << " unwind_joined=" << completed.load()
                << " affinity_checked=" << affinityChecked << " timeout_dump_checked=" << dumpChecked << " priority_checked=" << priorityChecked <<
                " second_argument_checked=" << (paired.m_code == 0 && badSecond.m_code == -1 && missingFirst.m_code == -1) << '\n';
            report.close();
            std::cout << "RUNNER_SELFTEST passed=" << (valid && bool(report)) << std::endl;
            return valid && bool(report) ? 0 : 1;
        }
        if (argc == 6 && std::strcmp(argv[1], "module") == 0)
        {
            int repeats = 0;
            if (!LockFreeHashMapRunner::number(argv[4], 100, repeats))
            {
                return 2;
            }
            bool passed = true;
            for (int i = 1; i <= repeats; ++i)
            {
                std::ostringstream path;
                path << argv[3] << "-module-r" << i << ".log";
                LockFreeHashMapRunResult result = LockFreeHashMapRunner::run(argv[2], "", path.str(), 120);
                std::ifstream report(path.str().c_str());
                bool verified = LockFreeHashMapRunner::verifyModule(report, argv[5]);
                passed = passed && result.m_code == 0 && verified;
                std::cout << "MODULE repeat=" << i << " exit=" << result.m_code << " verified=" << verified << std::endl;
            }
            return passed ? 0 : 1;
        }
        if ((argc == 5 || argc == 6) && std::strcmp(argv[1], "cases") == 0)
        {
            int repeats = 0;
            if (!LockFreeHashMapRunner::number(argv[4], 100, repeats))
            {
                return 2;
            }
            std::vector<int> ids;
            std::istringstream selection(argc == 6 ? argv[5] : "1,2,3,4,5,6,7,8,9,10,11");
            std::string part;
            while (std::getline(selection, part, ','))
            {
                int id = 0;
                if (!LockFreeHashMapRunner::number(part.c_str(), 11, id))
                {
                    return 2;
                }
                ids.push_back(id);
            }
            if (ids.empty() || (argc == 6 && (argv[5][0] == 0 || argv[5][std::strlen(argv[5]) - 1] == ',')))
            {
                return 2;
            }
            return LockFreeHashMapRunner::cases(argv[2], argv[3], ids, repeats) ? 0 : 1;
        }
        if ((argc == 6 || argc == 7) && std::strcmp(argv[1], "workloads") == 0)
        {
            int scale = 0;
            int rounds = 0;
            int affinity = 0;
            if (!LockFreeHashMapRunner::number(argv[4], 20, scale) || !LockFreeHashMapRunner::number(argv[5], 9, rounds) ||
                (argc == 7 && !LockFreeHashMapRunner::number(argv[6], (std::numeric_limits<int>::max)(), affinity)))
            {
                return 2;
            }
            return LockFreeHashMapRunner::workloads(argv[2], argv[3], scale, rounds, static_cast<unsigned>(affinity)) ? 0 : 1;
        }
        std::cerr << "module EXE PREFIX REPEATS MODE | cases EXE PREFIX REPEATS [IDS] | workloads EXE PREFIX SCALE ROUNDS [CPU_MASK]" << std::endl;
        return 2;
    }
    catch (const std::exception& error)
    {
        std::cerr << "RUNNER exception=" << error.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "RUNNER unexpected exception" << std::endl;
    }
    return 3;
}
#endif