#include "NetTestBatch.h"
#include "NetTestProcess.h"
#include "NetRunnerSelfTest.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <set>
#include <sstream>
#include <stdexcept>
#include <cerrno>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

NetTestBatchCase::NetTestBatchCase(const std::string& name, const std::string& executable,
    const std::vector<std::string>& arguments, uint32_t deadlineMs) :
m_name(name), m_executable(executable), m_arguments(arguments), m_deadlineMs(deadlineMs)
{

}

std::string NetTestBatch::quote(const std::string& value)
{
    std::string result("\"");
    const char* hex = "0123456789abcdef";
    for (size_t index = 0; index < value.size(); ++index)
    {
        const uint8_t byte = static_cast<uint8_t>(value[index]);
        if (byte == '"' || byte == '\\') { result += '\\'; result += static_cast<char>(byte); }
        else if (byte < 32)
        {
            result += "\\u00";
            result += hex[byte >> 4];
            result += hex[byte & 15];
        }
        else { result += static_cast<char>(byte); }
    }
    result += '"';
    return result;
}

bool NetTestBatch::write(const std::string& filename, const std::string& value)
{
    if (value.size() > UINT32_MAX) { return false; }
    uint32_t error = 0;
    intptr_t file = CSystem::openFileHandle(filename.c_str(), 2, &error);
    if (file == -1)
    {
#ifdef _WIN32
        const bool exists = error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS;
#else
        const bool exists = error == EEXIST;
#endif
        if (exists) { file = CSystem::openFileHandle(filename.c_str(), 3); }
    }
    if (file == -1) { return false; }
    const bool written = CSystem::writeFileHandle(file, 0, reinterpret_cast<const uint8_t*>(value.data()),
        static_cast<uint32_t>(value.size())) && CSystem::resizeFileHandle(file, value.size());
    const bool closed = CSystem::closeFileHandle(&file);
    return written && closed;
}

void NetTestBatch::catalog(const std::map<std::string, std::string>& options, std::vector<NetTestBatchCase>& cases)
{
    // 配置只读；可选参数缺省为空，不偷偷指向工作区旧产物。
    const auto option = [&options](const std::string& key) -> std::string
    {
        const auto found = options.find(key);
        return found == options.end() ? std::string() : found->second;
    };
    const std::string root = option("--repository");
    const std::string directory = root + "/src/NetWorkTest/NetWorkTest";
    const std::string output = option("--output");
    const std::string binaries = option("--bin");
    const std::string node = option("--node");
    const std::string module = option("--module");
    const std::string protocolModule = option("--protocol-module");
#ifdef _WIN32
    const std::string suffix = ".exe";
#else
    const std::string suffix;
#endif
    const std::string test = binaries + "/NetWorkTest1.0" + suffix;
    const std::string modules = option("--modules-bin").empty() ? binaries : option("--modules-bin");
    for (const std::string& name : { "crypto", "proto", "interface", "sdk", "tls", "protobuf", "wire" })
    {
        std::vector<std::string> parameters;
        if (name == "wire")
        {
            parameters.push_back(output + "/wire-samples.bin");
            parameters.push_back(option("--wire-baseline"));
        }
        cases.push_back(NetTestBatchCase("module-" + name, modules + "/NetWorkTest-" + name + suffix, parameters, 180000));
        if (!CSystem::fileExists(CStringManager::Utf8ToUnicode(cases.back().m_executable)))
        {
            cases.back().m_missing = "Build the selected NetWorkTestModules target first";
        }
        else if (name == "wire" && (option("--wire-baseline").empty() ||
            !CSystem::fileExists(CStringManager::Utf8ToUnicode(option("--wire-baseline")))))
        {
            cases.back().m_missing = "Generate an independent version's wire baseline before comparing compatibility";
        }
    }
    const std::string hosts = option("--test-hosts");
    const std::string server = (hosts.empty() ? binaries + "/NetServerManager1.1" : hosts + "/NetWorkTestNode") + suffix;
    const std::string client = (hosts.empty() ? binaries + "/NetClientManager1.1" : hosts + "/NetWorkTestClient") + suffix;
    cases.push_back(NetTestBatchCase("native", test, { "--all", "--report=" + output + "/native.csv" }, 300000));
    for (const std::string& name : { "singleton", "replacement", "client-background" })
    {
        cases.push_back(NetTestBatchCase(name, test, { "--process-fixture=" + (name == "client-background" ? "background" : name),
            "--bin=" + binaries, "--output=" + output + "/" + name }, 90000));
    }
    // 只生成真实命令，不在批次层复刻JS用例或跳过其中的断言。
    const auto script = [&](const std::string& name, const std::string& filename,
        const std::vector<std::string>& parameters, uint32_t deadlineMs)
    {
        std::vector<std::string> arguments(1, directory + "/" + filename);
        arguments.insert(arguments.end(), parameters.begin(), parameters.end());
        cases.push_back(NetTestBatchCase(name, node, arguments, deadlineMs));
    };
    script("schedule", "wx_schedule_test.js", {}, 80000);
    script("recovery-callback", "wx_recovery_callback_test.js", {}, 80000);
    script("process-host", "process_host_test.js", { "--output=" + output + "/process-host" }, 20000);
    script("performance-assertions", "performance_assertions_test.js", {}, 10000);
    script("deadline-assertions", "deadline_test.js", {}, 10000);
    script("tls", "tls_interop_test.js", { "--module=" + module, "--report=" + output + "/tls.json", "--instant" }, 80000);
    script("legacy-gateway", "gateway_process_test.js", { "--module=" + module, "--executable=" + server,
        "--report=" + output + "/legacy-gateway.json", "--instant" }, 80000);
    script("identity", "gateway_identity_process_test.js", { "--server=" + server, "--output=" + output + "/identity" }, 80000);
    for (const std::string& name : { "link", "service", "internal_service", "managed_service", "compression", "mesh", "slow_channel",
        "service_capacity", "managed_cancel", "channel_capacity", "window_rtt" })
    {
        const bool process = name == "slow_channel" || name == "service_capacity" || name == "managed_cancel" ||
            name == "channel_capacity" || name == "window_rtt";
        std::vector<std::string> arguments = { "--server=" + server, "--module=" + module, "--output=" + output + "/" + name };
        if (name == "service")
        {
            arguments.push_back("--client=" + (hosts.empty() ? test : client));
            arguments.push_back("--test=" + test);
        }
        script(name, name + (process ? "_process_test.js" : "_gateway_process_test.js"), arguments, 80000);
    }
    script("local", "local_link_gateway_process_test.js",
        { "--server=" + server, "--client=" + client, "--test=" + test, "--output=" + output + "/local" }, 80000);
    script("sender-facade", "sender_c_api_process_test.js",
        { "--server=" + server, "--client=" + client, "--test=" + test, "--output=" + output + "/sender-facade" }, 80000);
    for (const std::string& fault : { "entry", "service" })
    {
        script("recovery-" + fault, "client_recovery_process_test.js", { "--server=" + server, "--client=" + client,
            "--test=" + test, "--module=" + module, "--output=" + output + "/recovery-" + fault, "--fault=" + fault }, 80000);
    }
    script("file-host", "wasm_file_flow_host_test.js", { "--module=" + module, "--report=" + output + "/file-host.json" }, 80000);
    script("peer-contract", "run_wasm_test.js", { "--module=" + module, "--peer-contract",
        "--report=" + output + "/peer-contract.json" }, 80000);
    script("peer-memory", "peer_memory_test.js", { "--module=" + module,
        "--report=" + output + "/peer-memory.json" }, 30000);
    script("peer-exchange", "NetWorkProcessTests.js", { server, module, output + "/peer-exchange" }, 45000);
    script("peer-host", "NetWorkHostTests.js", { module, output + "/peer-host.json" }, 30000);
    script("wx-host", "wx_sample_host_test.js", { module, output + "/wx-host.json" }, 45000);
    script("peer-file-host", "run_wasm_test.js", { "--module=" + module, "--peer-file-host",
        "--report=" + output + "/peer-file-host.json" }, 80000);
    script("sender-delivery", "run_wasm_test.js", { "--module=" + module, "--server=" + server,
        "--sender-delivery", "--report=" + output + "/sender-delivery/result.json" }, 60000);
    script("account-bridge", "run_wasm_test.js", { "--module=" + module, "--account-module=" + option("--account-module"),
        "--account-bridge", "--report=" + output + "/account-bridge.json" }, 80000);
    script("balanced-failover", "balanced_failover_process_test.js",
        { "--server=" + server, "--module=" + module, "--output=" + output + "/balanced-failover" }, 110000);
    script("recovery-budget", "recovery_budget_process_test.js",
        { "--server=" + server, "--test=" + test, "--module=" + module, "--output=" + output + "/recovery-budget" }, 80000);
    for (const std::string& name : { "legacy-desktop", "legacy-file" })
    {
        std::vector<std::string> arguments = { "--gateway=" + server, "--client-manager=" + client,
            "--probe=" + test, "--report=" + output + "/" + name + ".json", "--instant" };
        if (name == "legacy-file") { arguments.push_back("--files"); }
        script(name, "desktop_gateway_process_test.js", arguments, 80000);
    }
    script("cluster", "cluster_stress_test.js", { "--run", "--runtime=network", "--server=" + server,
        "--module=" + module, "--output=" + output + "/cluster" }, 150000);
    cases.push_back(NetTestBatchCase("runner", test,
        { "--runner-selfcheck", "--repository=" + root, "--output=" + output + "/runner" }, 60000));
    script("directory-cache", "directory_cache_wasm_test.js",
        { "--server=" + server, "--module=" + module, "--output=" + output + "/directory-cache.json" }, 100000);
    script("history", "NetGatewayHistoryProcessTests.js", { server, protocolModule, output + "/history" }, 35000);
    script("wasm", "run_wasm_test.js", { "--module=" + option("--wasm-tests"), "--report=" + output + "/wasm.csv", "--all" }, 100000);
    script("wx", "wx_smoke_test.js", { "--module=" + module, "--output=" + output + "/wx", "--cli=" + option("--wx-cli"),
        "--helper=" + (option("--wx-helper").empty() ? root + "/src/CookWxTest/lib/automationConnection.js" : option("--wx-helper")) }, 140000);
    std::vector<std::string> account = { "--bin=" + binaries, "--module=" + module, "--output=" + output + "/luma-account",
        "--mysql-config=" + option("--mysql-config"), "--account-module=" + option("--account-module") };
    if (!option("--gui-test").empty())
    {
        account.push_back("--gui-test=" + option("--gui-test"));
        account.push_back("--qt=" + option("--qt"));
    }
    script("luma-account", "luma_account_process_test.js", account, 100000);
    cases.push_back(NetTestBatchCase("luma-server", test, { "--process-fixture=luma", "--bin=" + binaries,
        "--luma-server=" + option("--luma-server"), "--luma-server-test=" + option("--luma-server-test"),
        "--output=" + output + "/luma-server" }, 60000));
    const std::set<std::string> protocolSuites = { "tls", "legacy-gateway", "link", "service", "internal_service", "managed_service",
        "compression", "mesh", "service_capacity", "managed_cancel", "channel_capacity",
        "recovery-entry", "recovery-service", "recovery-budget", "directory-cache", "luma-account" };
    for (size_t index = 0; index < cases.size(); ++index)
    {
        NetTestBatchCase& item = cases[index];
        if (item.m_name == "history" && protocolModule.empty())
        {
            item.m_missing = "Requires the separately built test-only protocol module";
        }
        if (protocolSuites.count(item.m_name))
        {
            if (protocolModule.empty())
            {
                item.m_missing = "Requires the separately built test-only protocol module";
            }
            else
            {
                for (std::string& argument : item.m_arguments)
                {
                    if (argument.compare(0, 9, "--module=") == 0)
                    {
                        argument = "--module=" + protocolModule;
                    }
                }
            }
        }
        if (item.m_name == "wasm" && option("--wasm-tests").empty()) { item.m_missing = "Requires separately built Wasm test executable"; }
        if (item.m_name == "wx" && option("--wx-cli").empty()) { item.m_missing = "Requires official WeChat tools and dependencies"; }
        if (item.m_name == "account-bridge" && option("--account-module").empty()) { item.m_missing = "Requires independent Account module"; }
        if (item.m_name == "luma-account" && (option("--account-module").empty() || option("--mysql-config").empty()))
        {
            item.m_missing = "Requires independent Account module and isolated test MySQL configuration";
        }
        if (item.m_name == "luma-server")
        {
            if (option("--luma-server").empty() || option("--luma-server-test").empty() ||
                !CSystem::fileExists(CStringManager::Utf8ToUnicode(option("--luma-server"))) ||
                !CSystem::fileExists(CStringManager::Utf8ToUnicode(option("--luma-server-test"))))
            {
                item.m_missing = "Requires matching LumaPlayerServer and LumaPlayerServerTest candidates";
            }
        }
        if (hosts.empty() && (item.m_name == "identity" || item.m_name == "mesh" || item.m_name == "slow_channel" ||
            item.m_name == "cluster" || item.m_name == "directory-cache" || item.m_name == "balanced-failover" ||
            item.m_name == "recovery-entry" || item.m_name == "recovery-service"))
        {
            // 同一测试EXE通过--node-host承载真实管理类，租约隔离只存在于测试翻译单元。
            for (std::string& argument : item.m_arguments)
            {
                if (argument.compare(0, 9, "--server=") == 0)
                {
                    argument = "--server=" + test;
                }
            }
        }
    }
}

int NetTestBatch::run(int argc, char** argv)
{
    std::vector<std::string> arguments;
    if (!CSystem::commandArguments(argc, argv, arguments) || arguments.empty() || arguments[0] != "--batch") { return 2; }
    const std::set<std::string> allowed = { "--bin", "--module", "--node", "--repository", "--output", "--only", "--account-module",
        "--test-hosts", "--wasm-tests", "--wx-cli", "--wx-helper", "--mysql-config", "--gui-test", "--luma-server",
        "--luma-server-test", "--qt", "--protocol-module", "--modules-bin", "--wire-baseline" };
    std::map<std::string, std::string> options;
    for (size_t index = 1; index < arguments.size(); ++index)
    {
        const size_t split = arguments[index].find('=');
        if (split == std::string::npos || split + 1 == arguments[index].size()) { return 2; }
        const std::string key = arguments[index].substr(0, split);
        if (!allowed.count(key) || options.count(key)) { return 2; }
        const std::string value = arguments[index].substr(split + 1);
        options[key] = key == "--only" ? value : CSystem::canonicalPathUtf8(value, false);
        if (options[key].empty()) { return 2; }
    }
    for (const std::string& key : { "--bin", "--module", "--node", "--repository", "--output" })
    {
        if (!options.count(key)) { return 2; }
    }
    if (options.count("--gui-test") && !options.count("--qt")) { return 2; }
    std::vector<NetTestBatchCase> cases;
    catalog(options, cases);
    std::set<std::string> names;
    for (size_t index = 0; index < cases.size(); ++index)
    {
        if (!names.insert(cases[index].m_name).second) { throw std::runtime_error("Duplicate suite registration"); }
    }
    std::set<std::string> selected;
    if (!options.count("--only")) { selected = names; }
    else
    {
        const std::string& selection = options["--only"];
        size_t begin = 0;
        do
        {
            const size_t comma = selection.find(',', begin);
            std::string name = selection.substr(begin, comma == std::string::npos ? std::string::npos : comma - begin);
            const size_t first = name.find_first_not_of(" \t");
            if (first == std::string::npos) { return 2; }
            name = name.substr(first, name.find_last_not_of(" \t") - first + 1);
            if (!names.count(name)) { return 2; }
            selected.insert(name);
            if (comma == std::string::npos) { break; }
            begin = comma + 1;
        } while (begin <= selection.size());
    }
    const std::string output = options["--output"];
#ifdef _WIN32
    if (!CreateDirectoryW(CStringManager::Utf8ToUnicode(output).c_str(), nullptr)) { return 2; }
#else
    if (mkdir(output.c_str(), 0700) != 0) { return 2; }
#endif
    std::vector<std::string> results;
    bool allPassed = true;
    bool complete = true;
    const auto save = [&]() -> bool
    {
        const bool finished = results.size() == selected.size();
        std::string payload = "{\"pass\":" + std::string(finished && complete && allPassed ? "true" : "false") +
            ",\"complete\":" + std::string(finished && complete ? "true" : "false") + ",\"results\":[";
        for (size_t index = 0; index < results.size(); ++index) { if (index != 0) { payload += ','; } payload += results[index]; }
        return write(output + "/result.json", payload + "]}\n");
    };
    if (!save()) { return 3; }
    for (size_t index = 0; index < cases.size(); ++index)
    {
        const NetTestBatchCase& item = cases[index];
        if (!selected.count(item.m_name)) { continue; }
        if (!item.m_missing.empty())
        {
            allPassed = false;
            complete = false;
            results.push_back("{\"suite\":" + quote(item.m_name) + ",\"state\":\"NOT_RUN\",\"pass\":false,\"reason\":" + quote(item.m_missing) + "}");
        }
        else
        {
            std::string error;
            std::string listing;
            int32_t code = -1;
            bool passed = false;
            const std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            try
            {
                if (item.m_name == "native" || item.m_name == "wasm")
                {
                    std::vector<std::string> listArguments;
                    if (item.m_name == "wasm")
                    {
                        listArguments.push_back(item.m_arguments[0]);
                        listArguments.push_back(item.m_arguments[1]);
                    }
                    listArguments.push_back("--list");
                    NetTestProcess selection;
                    int32_t listCode = -1;
                    if (!selection.start(item.m_executable, listArguments, output + "/" + item.m_name + "-selection.log") ||
                        selection.wait(15000, listCode) != 1 || listCode != 0 || !selection.output(listing))
                    {
                        error = "Cannot obtain the selected C++ case catalog";
                    }
#ifdef _WIN32
                    else if (selection.waitTree(1000) != 1)
                    {
                        error = "Case listing left a descendant process";
                    }
#endif
                }
                NetTestProcess process;
                if (!error.empty())
                {
                    // 目录取得失败时不得运行一个无法核对完整性的全量批次。
                }
                else if (!process.start(item.m_executable, item.m_arguments, output + "/" + item.m_name + ".log"))
                {
                    error = "Cannot start the selected executable";
                }
                else
                {
                    const int32_t state = process.wait(item.m_deadlineMs, code);
                    if (state != 1) { error = state == 0 ? "Suite deadline exceeded" : "Cannot observe suite exit"; }
                    else
                    {
                        passed = code == 0;
                        if (!passed)
                        {
                            error = "Suite returned a nonzero exit code";
                        }
#ifdef _WIN32
                        if (process.waitTree(1000) != 1)
                        {
                            passed = false;
                            if (!error.empty())
                            {
                                error += "; ";
                            }
                            error += "Suite did not clean up its process tree";
                        }
#endif
                        if (passed && (item.m_name == "native" || item.m_name == "wasm"))
                        {
                            std::vector<uint8_t> report;
                            if (!CSystem::readBinaryFile(CStringManager::Utf8ToUnicode(output + "/" + item.m_name + ".csv"),
                                16 * 1024 * 1024, report) ||
                                !NetRunnerSelfTest::nativeReport(listing, std::string(report.begin(), report.end())))
                            {
                                passed = false;
                                error = "C++ CSV does not contain every selected case exactly once as PASS";
                            }
                        }
                    }
                }
            }
            catch (const std::exception& exception) { passed = false; error = exception.what(); }
            catch (...) { passed = false; error = "Unexpected test runner exception"; }
            const int64_t elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
            allPassed = allPassed && passed;
            results.push_back("{\"suite\":" + quote(item.m_name) + ",\"state\":" + quote(passed ? "PASS" : "FAIL") +
                ",\"pass\":" + std::string(passed ? "true" : "false") + ",\"exitCode\":" + std::to_string(code) +
                ",\"durationMs\":" + std::to_string(elapsed) + ",\"error\":" + quote(error) +
                ",\"executable\":" + quote(item.m_executable) + ",\"arguments\":[");
            for (size_t argument = 0; argument < item.m_arguments.size(); ++argument)
            {
                if (argument != 0)
                {
                    results.back() += ',';
                }
                results.back() += quote(item.m_arguments[argument]);
            }
            results.back() += "]}";
        }
        if (!save()) { return 3; }
        std::printf("%s %s\n", item.m_name.c_str(), results.back().c_str());
        std::fflush(stdout);
    }
    return complete && allPassed ? 0 : 1;
}