#include "LockFreeQueueTestRunner.h"
#include "LockFreeQueueTestResults.h"
#include "LockFreeQueueTestWhitebox.h"
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

LockFreeQueueTestRunner::LockFreeQueueTestRunner(const LockFreeQueueTestTool& tool) :
m_tool(&tool), m_batch(tool.batch()), m_plan(Json::arrayValue), m_commands(Json::arrayValue),
m_inheritedAffinity(LockFreeQueueTestTool::affinity())
{

}

bool LockFreeQueueTestRunner::core(int32_t id)
{
    const std::set<int32_t> separate = {5, 9, 22, 23, 24, 33, 35, 38, 40, 46};
    return id >= 1 && id <= 46 && !separate.count(id);
}

bool LockFreeQueueTestRunner::pressure(int32_t id)
{
    const std::set<int32_t> ids = {2, 4, 5, 7, 22, 25, 26, 28, 33, 35, 36, 37, 38, 40, 46};
    return ids.count(id) != 0;
}

std::set<int32_t> LockFreeQueueTestRunner::select(const std::string& text)
{
    std::set<int32_t> result;
    if (text == "all" || text == "instant" || text == "stress")
    {
        for (int32_t i = 1; i <= 46; ++i)
        {
            if (text == "all" || (text == "stress" ? pressure(i) : !pressure(i)))
            {
                result.insert(i);
            }
        }
        return result;
    }
    std::vector<std::string> tokens = LockFreeQueueTestTool::split(text, ',');
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        std::vector<std::string> bounds = LockFreeQueueTestTool::split(tokens[i], '-');
        if (bounds.empty() || bounds.size() > 2)
        {
            throw std::runtime_error("Expected IDs/ranges within 1..46");
        }
        for (size_t j = 0; j < bounds.size(); ++j)
        {
            if (bounds[j].empty() || bounds[j].size() > 2 || bounds[j].find_first_not_of("0123456789") != std::string::npos)
            {
                throw std::runtime_error("Invalid case ID: " + tokens[i]);
            }
        }
        int32_t first = std::atoi(bounds[0].c_str()), last = std::atoi(bounds.back().c_str());
        if (first < 1 || last > 46 || first > last)
        {
            throw std::runtime_error("Case range outside 1..46 or descending");
        }
        for (int32_t id = first; id <= last; ++id)
        {
            result.insert(id);
        }
    }
    return result;
}

std::vector<std::string> LockFreeQueueTestRunner::validatedList(const std::string& option, const std::string& defaults, const std::string& allowed) const
{
    std::vector<std::string> values = m_tool->list(option, defaults), choices = LockFreeQueueTestTool::split(allowed, ',');
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (std::find(choices.begin(), choices.end(), values[i]) == choices.end())
        {
            throw std::runtime_error("Invalid " + option + ": " + values[i]);
        }
    }
    return values;
}

std::vector<std::string> LockFreeQueueTestRunner::arches(const std::string& fallback) const
{
    std::string value = m_tool->option("arch", fallback);
    if (value == "both")
    {
        return std::vector<std::string>({"x64", "Win32"});
    }
    if (value != "x64" && value != "Win32")
    {
        throw std::runtime_error("Expected --arch both|x64|Win32");
    }
    return std::vector<std::string>(1, value);
}

uint64_t LockFreeQueueTestRunner::cpuMask(const std::string& text)
{
    if (text.empty() || text.find_first_of("-+ \t\r\n") != std::string::npos)
    {
        throw std::runtime_error("Invalid CPU mask");
    }
    size_t used = 0;
    uint64_t value = std::stoull(text, &used, 0);
    if (!value || used != text.size())
    {
        throw std::runtime_error("Invalid CPU mask");
    }
    return value;
}

std::string LockFreeQueueTestRunner::buildDirectory(const std::string& arch, const std::string& version) const
{
    return LockFreeQueueTestTool::join(m_batch, "build/" + arch + "/" + version);
}

std::string LockFreeQueueTestRunner::input(const std::string& version, const std::string& file) const
{
    return LockFreeQueueTestTool::join(m_batch, "input/" + version + "/" + file);
}

void LockFreeQueueTestRunner::freeze(const std::string& file, const std::string& bytes)
{
    typedef LockFreeQueueTestTool IO;
    IO::inside(file, m_batch);
    std::string relative = file.substr(m_batch.size() + 1), hash = IO::digest(bytes);
    if (m_hashes.isMember(relative) && m_hashes[relative].asString() != hash)
    {
        throw std::runtime_error("Frozen input changed: " + file);
    }
    if (IO::exists(file) && IO::read(file) != bytes)
    {
        throw std::runtime_error("Existing evidence changed: " + file);
    }
    if (!IO::exists(file))
    {
        IO::write(file, bytes);
    }
    m_hashes[relative] = hash;
}

void LockFreeQueueTestRunner::verifyFrozen() const
{
    std::vector<std::string> names = m_hashes.getMemberNames();
    for (size_t i = 0; i < names.size(); ++i)
    {
        std::string file = LockFreeQueueTestTool::join(m_batch, names[i]);
        LockFreeQueueTestTool::inside(file, m_batch);
        if (LockFreeQueueTestTool::digest(LockFreeQueueTestTool::read(file)) != m_hashes[names[i]].asString())
        {
            throw std::runtime_error("Frozen input/binary changed: " + names[i]);
        }
    }
}

void LockFreeQueueTestRunner::snapshot(const std::string& version, const std::string& directory)
{
    typedef LockFreeQueueTestTool IO;
    if (!IO::exists(IO::join(directory, "LockFreeQueue.h")))
    {
        throw std::runtime_error("Missing Queue input: " + directory);
    }
    std::vector<std::string> files = IO::sources(directory), tests = IO::sources(m_tool->tests());
    files.insert(files.end(), tests.begin(), tests.end());
    for (size_t i = 0; i < files.size(); ++i)
    {
        freeze(input(version, IO::name(files[i])), IO::compiled(IO::read(files[i])));
    }
}

void LockFreeQueueTestRunner::baseline()
{
    typedef LockFreeQueueTestTool IO;
    const std::string commit = "17652486ea92ed9f6e10b47f25c2d5883f7e3ed8";
    const char* names[] = {"LockFreeQueue.h", "LockFreeQueue.inl"};
    for (size_t i = 0; i < 2; ++i)
    {
        std::string raw;
        if (!m_tool->option("baseline-dir").empty())
        {
            raw = IO::read(IO::join(m_tool->option("baseline-dir"), names[i]));
        }
        else
        {
            std::string log = IO::join(m_batch, std::string("git-") + names[i] + ".log");
            if (!IO::exists(log))
            {
                std::string gitRoot = m_tool->root();
                std::replace(gitRoot.begin(), gitRoot.end(), '\\', '/');
                if (gitRoot.size() > 1 && gitRoot[1] == ':' && gitRoot[0] >= 'a' && gitRoot[0] <= 'z')
                {
                    gitRoot[0] -= 'a' - 'A';
                }
                std::vector<std::string> command = {m_tool->option("git", "C:/Program Files/Git/mingw64/bin/git.exe"), "-C", gitRoot, "-c", "safe.directory=" + gitRoot, "show", commit + ":src/LockFreeQueue/LockFreeQueue/src/" + names[i]};
                Json::Value result = IO::process(command, m_tool->root(), log, 30);
                IO::save(log + ".json", result);
                if (result["exit"].asInt())
                {
                    throw std::runtime_error("Cannot read baseline commit; exit=" + IO::integer(result["exit"].asInt()) + "; log=" + log);
                }
            }
            raw = IO::read(log);
        }
        freeze(input("original", names[i]), raw);
    }
    std::vector<std::string> tests = IO::sources(m_tool->tests());
    for (size_t i = 0; i < tests.size(); ++i)
    {
        freeze(input("original", IO::name(tests[i])), IO::compiled(IO::read(tests[i])));
    }
}

std::vector<std::string> LockFreeQueueTestRunner::flags(const std::string& configuration) const
{
    bool debug = configuration == "Debug";
    if (!debug && configuration != "Release")
    {
        throw std::runtime_error("Expected Debug or Release");
    }
    std::vector<std::string> result = {"cl", "/nologo", "/EHsc", "/W4", debug ? "/MDd" : "/MD", debug ? "/Od" : "/O2", "/Zi"};
    if (debug)
    {
        result.push_back("/D_DEBUG");
    }
    return result;
}

void LockFreeQueueTestRunner::build(const std::string& arch, const std::string& config, const std::string& version,
    const std::string& target, const std::vector<std::vector<std::string>>& commands)
{
    typedef LockFreeQueueTestTool IO;
    std::string folder = buildDirectory(arch, version), script = "@echo off\r\nchcp 65001 >nul\r\nsetlocal DisableDelayedExpansion\r\ncall ";
    script += IO::quote(m_tool->option("vcvars", "C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/vcvarsall.bat"));
    script += arch == "x64" ? " amd64\r\n" : " x86\r\n";
    script += "if errorlevel 1 exit /b 2\r\ncd /d " + IO::quote(folder) + "\r\nif errorlevel 1 exit /b 2\r\n";
    for (size_t i = 0; i < commands.size(); ++i)
    {
        for (size_t j = 0; j < commands[i].size(); ++j)
        {
            if (j)
            {
                script += ' ';
            }
            script += IO::quote(commands[i][j]);
        }
        script += "\r\nif errorlevel 1 exit /b 1\r\n";
    }
    script += "exit /b 0\r\n";
    std::string path = IO::join(folder, "build-" + target + ".cmd");
    freeze(path, script);
    Json::Value meta;
    meta["kind"] = "build";
    meta["arch"] = arch;
    meta["configuration"] = config;
    meta["binary"] = IO::join(folder, target + ".exe");
    add("build/" + arch + "/" + version + "/" + target,
        std::vector<std::string>({IO::join(m_batch, "host/LockFreeQueueTestTool.exe"), "run-script", path}), folder, meta);
}

void LockFreeQueueTestRunner::add(const std::string& label, const std::vector<std::string>& command, const std::string& cwd,
    const Json::Value& meta, int32_t timeout)
{
    Json::Value row;
    row["label"] = label;
    row["cwd"] = cwd;
    row["meta"] = meta;
    row["timeout_seconds"] = timeout;
    for (size_t i = 0; i < command.size(); ++i)
    {
        row["command"].append(command[i]);
    }
    for (Json::Value::UInt i = 0; i < m_plan.size(); ++i)
    {
        if (m_plan[i]["label"] == row["label"])
        {
            throw std::runtime_error("Duplicate plan label: " + label);
        }
    }
    m_plan.append(row);
}

void LockFreeQueueTestRunner::functional()
{
    typedef LockFreeQueueTestTool IO;
    std::set<int32_t> selected = select(m_tool->option("cases", "all"));
    int32_t repeat = m_tool->number("repeat", 3, 1, 1000), seconds = m_tool->number("stress-seconds", 0, 0, 86400);
    if (seconds)
    {
        selected.insert(25);
        selected.insert(36);
    }
    snapshot("current", m_tool->source());
    if (selected.count(5) || selected.count(22) || selected.count(24) || selected.count(33) || selected.count(35) || selected.count(38) || selected.count(40))
    {
        baseline();
    }
    if (selected.count(24))
    {
        freeze(input("baseline-pause", "LockFreeQueue.h"), IO::read(input("original", "LockFreeQueue.h")));
        std::string needle = "QueueNode<QueueElementType>* prev = m_tail.exchange(newNode, std::memory_order_acq_rel);";
        freeze(input("baseline-pause", "LockFreeQueue.inl"), IO::compiled(IO::replaceOnce(IO::plain(IO::read(input("original", "LockFreeQueue.inl"))), needle, needle + "\n\tLockFreeQueueBaselinePause();")));
    }
    if (selected.count(9) || selected.count(23))
    {
        const char* libraries[] = {"LockFreeMap", "LockFreeHashMap", "Semaphore"};
        for (size_t i = 0; i < 3; ++i)
        {
            std::string source = m_tool->option("peer-snapshot-dir").empty() ? IO::join(m_tool->root(), std::string("src/") + libraries[i] + "/" + libraries[i] + "/src") : IO::join(m_tool->option("peer-snapshot-dir"), libraries[i]);
            std::vector<std::string> files = IO::sources(source);
            for (size_t j = 0; j < files.size(); ++j)
            {
                freeze(input(std::string("peers/") + libraries[i], IO::name(files[j])), IO::compiled(IO::read(files[j])));
            }
        }
    }
    std::string whitebox = IO::join(m_batch, "whitebox");
    std::vector<std::string> library = IO::sources(m_tool->source()), testFiles = IO::sources(m_tool->tests());
    for (size_t i = 0; i < library.size(); ++i)
    {
        freeze(input("library", IO::name(library[i])), IO::read(library[i]));
    }
    for (size_t i = 0; i < testFiles.size(); ++i)
    {
        freeze(input("core", IO::name(testFiles[i])), IO::compiled(IO::read(testFiles[i])));
    }
    LockFreeQueueTestWhitebox::prepare(*m_tool, input("library", ""), whitebox);
    std::vector<std::string> generated = IO::sources(whitebox);
    for (size_t i = 0; i < generated.size(); ++i)
    {
        freeze(generated[i], IO::read(generated[i]));
    }
    std::vector<std::string> platforms = m_tool->flag("matrix") ? std::vector<std::string>({"Win32", "x64"}) : arches("x64");
    std::vector<std::string> configs = m_tool->flag("matrix") ? std::vector<std::string>({"Debug", "Release"}) : std::vector<std::string>(1, m_tool->option("configuration", "Release"));
    for (size_t a = 0; a < platforms.size(); ++a)
    {
        for (size_t c = 0; c < configs.size(); ++c)
        {
            std::string arch = platforms[a], config = configs[c], version = "tests-" + config;
            std::string folder = buildDirectory(arch, version);
            Json::Value delayedPressure(Json::arrayValue);
            std::vector<std::string> base = flags(config), coreCommand = base;
            coreCommand.insert(coreCommand.end(), {"/DLOCK_FREE_QUEUE_TESTING", "/I" + whitebox, "/I" + input("core", ""), "/Fetests.exe", input("core", "LockFreeQueueTest.cpp")});
            bool hasCore = false;
            for (std::set<int32_t>::const_iterator id = selected.begin(); id != selected.end(); ++id)
            {
                hasCore = hasCore || core(*id);
            }
            if (hasCore)
            {
                build(arch, config, version, "tests", std::vector<std::vector<std::string>>(1, coreCommand));
            }
            for (int32_t stress = 0; stress < 2; ++stress)
            {
                std::string ids;
                Json::Value meta;
                meta["kind"] = "core";
                for (std::set<int32_t>::const_iterator id = selected.begin(); id != selected.end(); ++id)
                {
                    if (core(*id) && pressure(*id) == (stress != 0))
                    {
                        if (!ids.empty())
                        {
                            ids += ',';
                        }
                        ids += IO::integer(*id);
                        meta["ids"].append(*id);
                    }
                }
                if (!ids.empty())
                {
                    add(arch + "/" + config + "/core-" + IO::integer(stress), {IO::join(folder, "tests.exe"), ids}, folder, meta);
                    if (stress)
                    {
                        delayedPressure.append(m_plan[m_plan.size() - 1]);
                        m_plan.resize(m_plan.size() - 1);
                    }
                }
            }
            if (seconds && config == "Release" && arch == "x64")
            {
                for (int32_t id = 25; id <= 36; id += 11)
                {
                    Json::Value meta;
                    meta["kind"] = "core";
                    meta["ids"].append(id);
                    add(arch + "/" + config + "/steady-" + IO::integer(id), {IO::join(folder, "tests.exe"), IO::integer(id), IO::integer(seconds)}, folder, meta, seconds + 30);
                    delayedPressure.append(m_plan[m_plan.size() - 1]);
                    m_plan.resize(m_plan.size() - 1);
                }
            }
            if (selected.count(9) || selected.count(23))
            {
                std::vector<std::string> peers = IO::sources(input("peers/Semaphore", ""));
                std::vector<std::vector<std::string>> commands;
                std::vector<std::string> link = {"link", "/nologo", "/OUT:integration.exe"};
                peers.insert(peers.begin(), input("current", "LockFreeQueueIntegration.cpp"));
                for (size_t i = 0; i < peers.size(); ++i)
                {
                    std::string file = IO::name(peers[i]);
                    if (file.size() < 4 || file.substr(file.size() - 4) != ".cpp")
                    {
                        continue;
                    }
                    std::vector<std::string> command = base;
                    command.insert(command.end(), {"/DSTATIC_LIB", "/DNOMINMAX", "/I" + input("current", ""), "/I" + input("peers", ""), "/c", peers[i]});
                    commands.push_back(command);
                    link.push_back(file.substr(0, file.size() - 4) + ".obj");
                }
                commands.push_back(link);
                build(arch, config, version, "integration", commands);
                for (int32_t id = 9; id <= 23; id += 14)
                {
                    if (selected.count(id))
                    {
                        Json::Value meta;
                        meta["kind"] = "functional";
                        add(arch + "/" + config + "/case-" + IO::integer(id), {IO::join(folder, "integration.exe"), IO::integer(id)}, folder, meta);
                    }
                }
            }
            if (selected.count(24))
            {
                std::vector<std::string> command = base;
                command.insert(command.end(), {"/I" + input("baseline-pause", ""), "/Febaseline-pause.exe", input("original", "LockFreeQueueBaseline.cpp")});
                // The source's quoted include must resolve to the instrumented baseline first.
                freeze(input("baseline-pause", "LockFreeQueueBaseline.cpp"), IO::read(input("original", "LockFreeQueueBaseline.cpp")));
                command.back() = input("baseline-pause", "LockFreeQueueBaseline.cpp");
                build(arch, config, version, "baseline-pause", std::vector<std::vector<std::string>>(1, command));
                Json::Value meta;
                meta["kind"] = "functional";
                add(arch + "/" + config + "/case-24", {IO::join(folder, "baseline-pause.exe")}, folder, meta, 15);
            }
            for (Json::Value::UInt i = 0; i < delayedPressure.size(); ++i)
            {
                m_plan.append(delayedPressure[i]);
            }
            for (int32_t original = 0; original < 2; ++original)
            {
                std::string implementation = original ? "original" : "current";
                for (int32_t pools = 0; pools < 2; ++pools)
                {
                    std::set<int32_t> cases = pools ? std::set<int32_t>({38, 40}) : std::set<int32_t>({5, 22, 33, 35});
                    std::vector<int32_t> chosen;
                    std::set_intersection(cases.begin(), cases.end(), selected.begin(), selected.end(), std::back_inserter(chosen));
                    if (chosen.empty())
                    {
                        continue;
                    }
                    std::string target = (pools ? "pool-" : "measure-") + implementation;
                    std::vector<std::string> command = base;
                    if (original)
                    {
                        command.push_back("/DLOCK_FREE_QUEUE_BASELINE");
                    }
                    if (pools)
                    {
                        command.push_back("/DLOCK_FREE_QUEUE_BENCH_VARIANT=" + IO::integer(original ? 0 : 1));
                    }
                    command.insert(command.end(), {"/Fe" + target + ".exe", input(implementation, pools ? "LockFreeQueuePerformance.cpp" : "LockFreeQueueMeasurements.cpp"), "/link", "Psapi.lib"});
                    build(arch, config, version, target, std::vector<std::vector<std::string>>(1, command));
                    for (int32_t r = 1; r <= (pools || config == "Debug" ? 1 : repeat); ++r)
                    {
                        for (size_t i = 0; i < chosen.size(); ++i)
                        {
                            Json::Value meta;
                            meta["kind"] = "functional";
                            std::vector<std::string> args = {IO::join(folder, target + ".exe"), IO::integer(chosen[i])};
                            if (pools)
                            {
                                args.push_back("--no-allocation-count");
                            }
                            add(arch + "/" + config + "/case-" + IO::integer(chosen[i]) + "-" + implementation + "-" + IO::integer(r), args, folder, meta);
                        }
                    }
                }
            }
            if (selected.count(46))
            {
                std::vector<std::string> command = base;
                command.insert(command.end(), {"/DLOCK_FREE_QUEUE_SHARED_EXTRA", "/Feshared.exe", input("current", "LockFreeQueueSharedPtrBenchmark.cpp"), "/link", "Psapi.lib"});
                build(arch, config, version, "shared", std::vector<std::vector<std::string>>(1, command));
                const int32_t producers[] = {1, 4, 8, 16};
                for (size_t i = 0; i < 4; ++i)
                {
                    for (int32_t r = 1; r <= repeat; ++r)
                    {
                        Json::Value meta;
                        meta["kind"] = "shared-ptr";
                        meta["items"] = 20000000.0;
                        meta["producers"] = double(producers[i]);
                        meta["arch"] = arch;
                        meta["version"] = "candidate";
                        meta["mode"] = "plain";
                        meta["round"] = r;
                        add(arch + "/" + config + "/case-46-p" + IO::integer(producers[i]) + "-" + IO::integer(r),
                        {IO::join(folder, "shared.exe"), "46", IO::integer(producers[i]), "20000000", "0", "0", "0"}, folder, meta, 300);
                    }
                }
            }
            if (m_tool->flag("consumers"))
            {
                for (int32_t staticLink = 0; staticLink < 2; ++staticLink)
                {
                    std::string target = staticLink ? "consumer-lib" : "consumer-dll", file = input("current", "LockFreeQueueConsumer.cpp");
                    std::vector<std::string> common = base;
                    if (staticLink)
                    {
                        common.push_back("/DLOCK_FREE_QUEUE_TEST_STATIC");
                    }
                    std::vector<std::string> provider = common, consumer = common;
                    provider.insert(provider.end(), {"/DLOCK_FREE_QUEUE_TEST_PROVIDER", "/c", "/Foprovider.obj", file});
                    consumer.insert(consumer.end(), {file, "/Fe" + target + ".exe", staticLink ? "provider-static.lib" : "provider.lib"});
                    std::vector<std::string> link = staticLink ? std::vector<std::string>({"lib", "/nologo", "provider.obj", "/OUT:provider-static.lib"}) :
                    std::vector<std::string>({"link", "/nologo", "/DLL", "provider.obj", "/OUT:provider.dll", "/IMPLIB:provider.lib"});
                    build(arch, config, version, target, {provider, link, consumer});
                    Json::Value meta;
                    meta["kind"] = "functional";
                    add(arch + "/" + config + "/" + target, {IO::join(folder, target + ".exe")}, folder, meta);
                }
            }
            if (m_tool->flag("projects"))
            {
                const char* kinds[] = {"dll", "lib", "test"};
                for (size_t i = 0; i < 3; ++i)
                {
                    std::string project = i == 2 ? IO::join(m_tool->tests(), "LockFreeQueueTest.vcxproj") : IO::join(m_tool->root(), std::string("src/LockFreeQueue/LockFreeQueue/vcxproj/LockFreeQueue") + (i == 1 ? "_lib" : "") + ".vcxproj");
                    std::string output = IO::join(folder, std::string("project-") + kinds[i]);
                    Json::Value meta;
                    meta["kind"] = "build";
                    add(arch + "/" + config + "/project-" + kinds[i], {"C:/Program Files (x86)/MSBuild/12.0/Bin/MSBuild.exe", project,
                    "/nologo", "/t:Rebuild", "/p:Configuration=" + config, "/p:Platform=" + arch, "/p:OutDir=" + output + "/", "/p:IntDir=" + output + "/obj/"}, folder, meta);
                    if (i == 2)
                    {
                        Json::Value check;
                        check["kind"] = "core";
                        const int32_t ids[] = {1, 11, 16, 17, 20, 21, 39};
                        for (size_t j = 0; j < 7; ++j)
                        {
                            check["ids"].append(ids[j]);
                        }
                        add(arch + "/" + config + "/project-test-run", {IO::join(output, "LockFreeQueueTest.exe"), "1,11,16,17,20,21,39"}, folder, check);
                    }
                }
            }
        }
    }
}

void LockFreeQueueTestRunner::performance(const std::string& mode)
{
    typedef LockFreeQueueTestTool IO;
    const std::vector<std::string> profiles = {"fixed", "dynamic", "default", "positive", "finite_positive", "grown_reuse"};
    const std::vector<std::string> selectedSuites = validatedList("suite", "35,extra,38,40", "35,extra,38,40");
    const std::vector<std::string> selectedProfiles = validatedList("profiles", "fixed,dynamic,default,positive,finite_positive,grown_reuse", "fixed,dynamic,default,positive,finite_positive,grown_reuse");
    std::vector<std::string> versions;
    std::map<std::string, int32_t> variants;
    std::map<std::string, std::string> inputs;
    std::set<std::string> references;
    bool revision = mode == "revision", three = mode == "three-way";
    int32_t rounds = m_tool->number("rounds", mode == "performance" ? 7 : 3, 1, 1000);
    int32_t supplement = m_tool->number("supplement-rounds", 3, 1, 1000), allocation = m_tool->number("allocation-rounds", 1, 1, 1000);
    if (revision)
    {
        if (m_tool->option("before-dir").empty())
        {
            throw std::runtime_error("revision requires --before-dir");
        }
        snapshot("before", m_tool->option("before-dir"));
        snapshot("after", m_tool->option("after-dir", m_tool->source()));
        std::vector<std::string> calls = validatedList("calls", "push,tryPush,producer_tryPush,producer_push", "push,tryPush,producer_tryPush,producer_push");
        std::map<std::string, int32_t> numbers = {{"push", 1}, {"tryPush", 2}, {"producer_tryPush", 3}, {"producer_push", 4}};
        for (size_t i = 0; i < calls.size(); ++i)
        {
            for (int32_t side = 0; side < 2; ++side)
            {
                std::string version = std::string(side ? "after-" : "before-") + calls[i];
                versions.push_back(version);
                variants[version] = numbers[calls[i]];
                inputs[version] = side ? "after" : "before";
            }
        }
    }
    else
    {
        baseline();
        snapshot("current", m_tool->source());
        // Pin the original benchmark reference to its recorded commit, not a caller's mutable replacement.
        const char* hashes[] = {"0b3b785cad9cd873d1ac3625ee0c6cf3369cf530760872190b3d84553d5ab215", "5d5166cdafe61f27f5b71f0e8740fbe46c617d0d41604ba4bef83ca722905ba3"};
        const char* names[] = {"LockFreeQueue.h", "LockFreeQueue.inl"};
        for (size_t i = 0; i < 2; ++i)
        {
            if (IO::digest(IO::read(input("original", names[i]))) != hashes[i])
            {
                throw std::runtime_error("Baseline differs from immutable original commit");
            }
        }
        references.insert("original");
        variants["original"] = 0;
        inputs["original"] = "original";
        if (three)
        {
            versions = {"original", "current", "std_list_mutex"};
            variants["current"] = 1;
            variants["std_list_mutex"] = 0;
            inputs["current"] = "current";
            inputs["std_list_mutex"] = "std_list_mutex";
            references.insert("std_list_mutex");
            std::vector<std::string> files = IO::sources(input("original", ""));
            for (size_t i = 0; i < files.size(); ++i)
            {
                freeze(input("std_list_mutex", IO::name(files[i])), IO::name(files[i]) == "LockFreeQueue.h" ? IO::read(input("original", "LockFreeQueueListMutex.h")) : IO::read(files[i]));
            }
            if (!m_tool->option("previous-dir").empty())
            {
                snapshot("previous", m_tool->option("previous-dir"));
                versions.push_back("previous");
                variants["previous"] = 1;
                inputs["previous"] = "previous";
            }
        }
        else
        {
            versions = {"original", "push", "tryPush", "producer_tryPush", "producer_push"};
            for (size_t i = 1; i < versions.size(); ++i)
            {
                variants[versions[i]] = static_cast<int32_t>(i);
                inputs[versions[i]] = "current";
            }
        }
    }
    std::vector<std::string> platforms = arches();
    for (size_t a = 0; a < platforms.size(); ++a)
    {
        for (size_t v = 0; v < versions.size(); ++v)
        {
            std::vector<std::string> command = flags("Release");
            command.push_back("/DLOCK_FREE_QUEUE_BENCH_VARIANT=" + IO::integer(variants[versions[v]]));
            if (references.count(versions[v]))
            {
                command.push_back("/DLOCK_FREE_QUEUE_BASELINE");
            }
            if (versions[v] == "std_list_mutex")
            {
                command.push_back("/DLOCK_FREE_QUEUE_LIST_MUTEX");
            }
            command.insert(command.end(), {"/Febench.exe", input(inputs[versions[v]], "LockFreeQueuePerformance.cpp"), "/link", "Psapi.lib"});
            build(platforms[a], "Release", versions[v], "bench", std::vector<std::vector<std::string>>(1, command));
            Json::Value meta;
            meta["kind"] = "cli";
            meta["expected_exit"] = 2;
            std::string folder = buildDirectory(platforms[a], versions[v]);
            add("cli/" + platforms[a] + "-" + versions[v], {IO::join(folder, "bench.exe"), "invalid"}, folder, meta);
        }
    }
    // One plan lists every timing/counter run; summary verifies this exact complete plan.
    std::vector<std::string> phases = revision ? std::vector<std::string>({"main", "allocation"}) : std::vector<std::string>({"preflight", "main", "supplement", "allocation"});
    for (size_t ph = 0; ph < phases.size(); ++ph)
    {
        std::string phase = phases[ph];
        int32_t count = phase == "preflight" ? 1 : (phase == "main" ? rounds : (phase == "supplement" ? supplement : allocation));
        for (size_t a = 0; a < platforms.size(); ++a)
        {
            for (int32_t turn = 1; turn <= count; ++turn)
            {
                std::vector<std::string> order = versions;
                if (revision)
                {
                    if (turn % 2 == 0)
                    {
                        for (size_t i = 0; i < order.size(); i += 2)
                        {
                            std::swap(order[i], order[i + 1]);
                        }
                    }
                }
                else
                {
                    std::rotate(order.begin(), order.begin() + (turn - 1) % order.size(), order.end());
                    if (!three && turn % 2 == 0)
                    {
                        std::reverse(order.begin(), order.end());
                    }
                }
                const char* suites[] = {"35", "extra", "38", "40"};
                for (size_t s = 0; s < 4; ++s)
                {
                    std::string suite = suites[s];
                    if (std::find(selectedSuites.begin(), selectedSuites.end(), suite) == selectedSuites.end())
                    {
                        continue;
                    }
                    if (s == 3 && !(revision || (three && m_tool->flag("wave"))))
                    {
                        continue;
                    }
                    if (!revision && phase == "main" && s != 0)
                    {
                        continue;
                    }
                    if (three && phase == "supplement" && s == 0)
                    {
                        continue;
                    }
                    for (size_t p = 0; p < profiles.size(); ++p)
                    {
                        if (std::find(selectedProfiles.begin(), selectedProfiles.end(), profiles[p]) == selectedProfiles.end())
                        {
                            continue;
                        }
                        if (s >= 2 && p != 2)
                        {
                            continue;
                        }
                        if (s == 1 && (p >= 3 || (phase == "preflight" && p != 2)))
                        {
                            continue;
                        }
                        if (!three && !revision && s == 0 && ((phase == "main" && p >= 3) || (phase == "supplement" && p < 3)))
                        {
                            continue;
                        }
                        for (size_t v = 0; v < order.size(); ++v)
                        {
                            if (revision && s == 3 && variants[order[v]] != 1)
                            {
                                continue;
                            }
                            std::string folder = buildDirectory(platforms[a], order[v]);
                            Json::Value meta;
                            meta["kind"] = "performance";
                            meta["phase_group"] = phase;
                            meta["arch"] = platforms[a];
                            meta["version"] = order[v];
                            meta["profile"] = profiles[p];
                            meta["suite"] = suite;
                            meta["round"] = turn;
                            meta["counted"] = phase == "allocation";
                            meta["reference"] = references.count(order[v]) != 0;
                            std::vector<std::string> args = {IO::join(folder, "bench.exe"), suite};
                            if (phase != "allocation")
                            {
                                args.push_back("--no-allocation-count");
                            }
                            add(phase + "/" + platforms[a] + "-" + profiles[p] + "-" + suite + "-" + order[v] + "-" + IO::integer(turn), args, folder, meta, 240);
                        }
                    }
                }
            }
        }
    }
}

void LockFreeQueueTestRunner::sharedPtr()
{
    typedef LockFreeQueueTestTool IO;
    if (m_tool->option("before-dir").empty())
    {
        throw std::runtime_error("shared-ptr requires --before-dir");
    }
    int32_t items = m_tool->number("items", 20000000, 16, 20000000), rounds = m_tool->number("rounds", 3, 1, 1000);
    if (items % 16)
    {
        throw std::runtime_error("Item count must be divisible by 16");
    }
    snapshot("online", m_tool->option("before-dir"));
    snapshot("candidate", m_tool->option("after-dir", m_tool->source()));
    std::vector<std::string> platforms = arches(), producers = validatedList("producers", "1,4,8,16", "1,4,8,16");
    std::vector<std::string> modes = validatedList("modes", "plain,bound", "plain,bound,prefix,distributed,reserved,distributed_pop");
    std::map<std::string, std::vector<std::string>> switches = {{"plain", {"0", "0", "0"}}, {"bound", {"1", "0", "0"}},
    {"prefix", {"1", "1", "0"}}, {"distributed", {"1", "1", "1"}}, {"reserved", {"1", "1", "1", "1"}}, {"distributed_pop", {"1", "0", "1"}}};
    for (size_t a = 0; a < platforms.size(); ++a)
    {
        for (int32_t candidate = 0; candidate < 2; ++candidate)
        {
            std::string version = candidate ? "candidate" : "online";
            std::vector<std::string> command = flags("Release");
            if (candidate)
            {
                command.push_back("/DLOCK_FREE_QUEUE_SHARED_EXTRA");
            }
            command.insert(command.end(), {"/Febench.exe", input(version, "LockFreeQueueSharedPtrBenchmark.cpp"), "/link", "Psapi.lib"});
            build(platforms[a], "Release", version, "bench", std::vector<std::vector<std::string>>(1, command));
        }
    }
    for (size_t a = 0; a < platforms.size(); ++a)
    {
        for (int32_t turn = 1; turn <= rounds; ++turn)
        {
            for (size_t p = 0; p < producers.size(); ++p)
            {
                for (size_t m = 0; m < modes.size(); ++m)
                {
                    for (int32_t side = 0; side < 2; ++side)
                    {
                        bool candidate = (turn % 2 ? side : 1 - side) != 0;
                        if (!candidate && (m_tool->flag("candidate-only") || (modes[m] != "plain" && modes[m] != "bound")))
                        {
                            continue;
                        }
                        std::string version = candidate ? "candidate" : "online", folder = buildDirectory(platforms[a], version);
                        std::vector<std::string> command = {IO::join(folder, "bench.exe"), "46", producers[p], IO::integer(items)};
                        command.insert(command.end(), switches[modes[m]].begin(), switches[modes[m]].end());
                        Json::Value meta;
                        meta["kind"] = "shared-ptr";
                        meta["arch"] = platforms[a];
                        meta["version"] = version;
                        meta["mode"] = modes[m];
                        meta["round"] = turn;
                        meta["items"] = double(items);
                        meta["producers"] = std::strtod(producers[p].c_str(), nullptr);
                        meta["acceptance_workload"] = items == 20000000;
                        add("runs/" + platforms[a] + "-" + version + "-" + modes[m] + "-p" + producers[p] + "-r" + IO::integer(turn), command, folder, meta, 300);
                    }
                }
            }
        }
    }
}

void LockFreeQueueTestRunner::scheduling()
{
    typedef LockFreeQueueTestTool IO;
    if (m_tool->option("before-dir").empty())
    {
        throw std::runtime_error("scheduling requires --before-dir");
    }
    snapshot("before", m_tool->option("before-dir"));
    snapshot("after", m_tool->option("after-dir", m_tool->source()));
    int32_t duration = m_tool->number("duration-ms", 2000, 100, 60000), rounds = m_tool->number("rounds", 2, 1, 1000);
    std::vector<std::string> platforms = arches(), masks = m_tool->list("masks", "1,3");
    std::vector<std::string> modes = validatedList("modes", "0,1,2,3,4,5", "0,1,2,3,4,5"), capacities = validatedList("capacities", "65536", "256,65536");
    std::vector<std::string> producers = validatedList("producers", "1,4,8,16", "1,4,8,16");
    for (size_t a = 0; a < platforms.size(); ++a)
    {
        for (int32_t side = 0; side < 2; ++side)
        {
            std::string version = side ? "after" : "before";
            std::vector<std::string> command = flags("Release");
            command.insert(command.end(), {"/Fescheduling.exe", input(version, "LockFreeQueueScheduling.cpp")});
            build(platforms[a], "Release", version, "scheduling", std::vector<std::vector<std::string>>(1, command));
        }
    }
    for (int32_t turn = 1; turn <= rounds; ++turn)
    {
        for (size_t a = 0; a < platforms.size(); ++a)
        {
            for (size_t k = 0; k < masks.size(); ++k)
            {
                for (size_t p = 0; p < producers.size(); ++p)
                {
                    for (size_t m = 0; m < modes.size(); ++m)
                    {
                        for (int32_t cancel = 0; cancel < 2; ++cancel)
                        {
                            for (size_t c = 0; c < capacities.size(); ++c)
                            {
                                for (int32_t side = 0; side < 2; ++side)
                                {
                                    uint64_t mask = cpuMask(masks[k]);
                                    if ((mask & m_inheritedAffinity) != mask)
                                    {
                                        throw std::runtime_error("Unavailable scheduling CPU mask");
                                    }
                                    bool after = (turn % 2 ? side : 1 - side) != 0;
                                    if (!after && m_tool->flag("after-only"))
                                    {
                                        continue;
                                    }
                                    std::string version = after ? "after" : "before", folder = buildDirectory(platforms[a], version);
                                    Json::Value meta;
                                    meta["kind"] = "scheduling";
                                    meta["arch"] = platforms[a];
                                    meta["version"] = version;
                                    meta["mask"] = masks[k];
                                    meta["producers"] = std::strtod(producers[p].c_str(), nullptr);
                                    meta["mode"] = std::atoi(modes[m].c_str());
                                    meta["cancelable"] = cancel;
                                    meta["capacity"] = std::atoi(capacities[c].c_str());
                                    meta["round"] = turn;
                                    add("scheduling/" + platforms[a] + "-cpu" + masks[k] + "-p" + producers[p] + "-m" + modes[m] + "-c" + IO::integer(cancel) + "-n" + capacities[c] + "-r" + IO::integer(turn) + "-" + version,
                                    {IO::join(folder, "scheduling.exe"), producers[p], IO::integer(duration), modes[m], IO::integer(cancel), capacities[c]}, folder, meta, duration / 1000 + 45);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void LockFreeQueueTestRunner::execute()
{
    typedef LockFreeQueueTestTool IO;
    bool failed = false;
    for (Json::Value::UInt i = 0; i < m_plan.size(); ++i)
    {
        const Json::Value& planned = m_plan[i];
        bool done = false;
        for (Json::Value::UInt j = 0; j < m_commands.size(); ++j)
        {
            if (m_commands[j]["label"] == planned["label"])
            {
                if (m_commands[j]["validated_exit"].asInt() != 0 || m_commands[j]["command"] != planned["command"] || m_commands[j]["meta"] != planned["meta"])
                {
                    throw std::runtime_error("Failed or changed evidence cannot be resumed");
                }
                std::string log = IO::join(m_batch, m_commands[j]["relative_log"].asString());
                if (IO::digest(IO::read(log)) != m_commands[j]["sha256"].asString())
                {
                    throw std::runtime_error("Completed log changed");
                }
                done = true;
            }
        }
        if (done)
        {
            continue;
        }
        std::string kind = planned["meta"]["kind"].asString();
        bool preparation = kind == "build" || kind == "cli";
        if (m_tool->flag("build-only") && kind != "build")
        {
            continue;
        }
        if (m_tool->flag("prepare-only") && !preparation)
        {
            continue;
        }
        if (m_tool->flag("measure-only") && preparation)
        {
            throw std::runtime_error("Preparation incomplete; run --prepare-only first");
        }
        std::vector<std::string> args;
        for (Json::Value::UInt j = 0; j < planned["command"].size(); ++j)
        {
            args.push_back(planned["command"][j].asString());
        }
        std::string relative = planned["label"].asString() + ".raw.log", log = IO::join(m_batch, relative);
        IO::inside(log, m_batch);
        IO::directory(planned["cwd"].asString());
        uint64_t previous = IO::affinity();
        if (kind == "scheduling")
        {
            IO::affinity(cpuMask(planned["meta"]["mask"].asString()));
        }
#ifdef _WIN32
        std::string profile = planned["meta"]["profile"].asString();
        if (kind == "performance")
        {
            SetEnvironmentVariableA("LFQ_BENCH_PROFILE", profile.c_str());
        }
#endif
        Json::Value record;
        try
        {
            record = IO::process(args, planned["cwd"].asString(), log, planned["timeout_seconds"].asInt());
        }
        catch (...)
        {
            if (kind == "scheduling")
            {
                IO::affinity(previous);
            }
            throw;
        }
        if (kind == "scheduling")
        {
            IO::affinity(previous);
        }
        record["label"] = planned["label"];
        record["relative_log"] = relative;
        record["meta"] = planned["meta"];
        record["sha256"] = IO::digest(IO::read(log));
        record["validated_exit"] = 0;
        try
        {
            record["parsed"] = Json::FastWriter().write(LockFreeQueueTestResults::parse(IO::read(log), planned["meta"], record["exit"].asInt()));
            if (kind == "build" && planned["meta"].isMember("binary"))
            {
                std::string file = planned["meta"]["binary"].asString();
                m_hashes[file.substr(m_batch.size() + 1)] = IO::digest(IO::read(file));
                IO::save(IO::join(m_batch, "frozen-hashes.json"), m_hashes);
            }
        }
        catch (const std::exception& error)
        {
            record["validated_exit"] = 1;
            record["validation_error"] = error.what();
            failed = true;
        }
        m_commands.append(record);
        IO::save(IO::join(m_batch, "commands.json"), m_commands);
        std::cout << (failed ? "FAIL " : "PASS ") << planned["label"].asString() << " " << record["seconds"].asDouble() << "s" << std::endl;
        if (failed)
        {
            throw std::runtime_error("Failure retained: " + log + " " + record["validation_error"].asString());
        }
    }
}

int32_t LockFreeQueueTestRunner::run(const std::string& command)
{
    typedef LockFreeQueueTestTool IO;
    if (command == "summarize")
    {
        LockFreeQueueTestResults::summarize(m_batch);
        return 0;
    }
    if (command != "tests" && command != "performance" && command != "three-way" && command != "revision" && command != "shared-ptr" && command != "scheduling")
    {
        throw std::runtime_error("Unknown host command: " + command);
    }
    bool resume = m_tool->flag("resume") || m_tool->flag("measure-only");
    if (int32_t(m_tool->flag("prepare-only")) + int32_t(m_tool->flag("build-only")) + int32_t(m_tool->flag("measure-only")) > 1)
    {
        throw std::runtime_error("Choose only one of --build-only, --prepare-only and --measure-only");
    }
    if (IO::exists(m_batch) && !resume)
    {
        throw std::runtime_error("Use a fresh --batch, or --resume for unchanged successful evidence");
    }
    IO::directory(m_batch);
    if (resume)
    {
        m_hashes = IO::load(IO::join(m_batch, "frozen-hashes.json"));
        verifyFrozen();
        if (IO::exists(IO::join(m_batch, "commands.json")))
        {
            m_commands = IO::load(IO::join(m_batch, "commands.json"));
        }
    }
    freeze(IO::join(m_batch, "host/LockFreeQueueTestTool.exe"), IO::read(IO::executable()));
    if (!m_tool->option("affinity").empty())
    {
        uint64_t mask = cpuMask(m_tool->option("affinity"));
        if ((mask & m_inheritedAffinity) != mask)
        {
            throw std::runtime_error("Unavailable CPU mask");
        }
        IO::affinity(mask);
    }
    try
    {
        if (command == "tests")
        {
            functional();
        }
        else if (command == "shared-ptr")
        {
            sharedPtr();
        }
        else if (command == "scheduling")
        {
            scheduling();
        }
        else
        {
            performance(command);
        }
        bool workloads = false;
        for (Json::Value::UInt i = 0; i < m_plan.size(); ++i)
        {
            workloads = workloads || (m_plan[i]["meta"]["kind"].asString() != "build" && m_plan[i]["meta"]["kind"].asString() != "cli");
        }
        if (!workloads)
        {
            throw std::runtime_error("Selected filters contain no executable workloads; wave requires --wave in three-way mode");
        }
        std::string planFile = IO::join(m_batch, "plan.json");
        if (resume && IO::load(planFile) != m_plan)
        {
            throw std::runtime_error("Run configuration differs from frozen matrix");
        }
        IO::save(planFile, m_plan);
        IO::save(IO::join(m_batch, "frozen-hashes.json"), m_hashes);
        Json::Value environment;
        std::ostringstream affinityText;
        affinityText << "0x" << std::hex << IO::affinity();
        environment["command"] = command;
        environment["affinity"] = affinityText.str();
        environment["independent_exclusive_host"] = false;
        environment["scope"] = "Queue tests only; peer smoke is not four-library or system acceptance";
        environment["baseline_commit"] = "17652486ea92ed9f6e10b47f25c2d5883f7e3ed8";
        environment["compiler"] = "VS2013; workload flags recorded in frozen build scripts";
        environment["format"] = 2;
        environment["planned_commands"] = static_cast<int32_t>(m_plan.size());
        environment["profiles_filter"] = m_tool->option("profiles", "all");
        environment["suite_filter"] = m_tool->option("suite", "all");
        const char* processor = std::getenv("PROCESSOR_IDENTIFIER");
        environment["cpu"] = processor ? processor : "not available";
        environment["before_dir"] = m_tool->option("before-dir");
        environment["after_dir"] = m_tool->option("after-dir", m_tool->source());
        environment["host_pointer_bits"] = int32_t(sizeof(void*) * 8);
        std::time_t timestamp = std::time(nullptr);
        char utc[32] = {};
        std::tm* time = std::gmtime(&timestamp);
        if (time)
        {
            std::strftime(utc, sizeof(utc), "%Y-%m-%dT%H:%M:%SZ", time);
        }
        environment["utc"] = utc;
        std::string environmentFile = IO::join(m_batch, "environment.json");
        if (resume)
        {
            Json::Value previous = IO::load(environmentFile);
            if (previous["affinity"] != environment["affinity"] || previous["command"] != environment["command"])
            {
                throw std::runtime_error("Execution affinity/command changed during resume");
            }
        }
        else
        {
            IO::save(environmentFile, environment);
        }
        if (m_tool->flag("plan"))
        {
            std::cout << "PLAN " << m_plan.size() << " commands; no compilation or measurements" << std::endl;
        }
        else
        {
            execute();
            verifyFrozen();
            if (m_tool->flag("build-only") || m_tool->flag("prepare-only"))
            {
                std::cout << "PREPARED; full workload remains pending" << std::endl;
            }
            else
            {
                LockFreeQueueTestResults::summarize(m_batch);
                std::cout << "COMPLETE " << m_commands.size() << " commands; see raw logs and reports under " << m_batch << std::endl;
            }
        }
    }
    catch (...)
    {
        if (m_inheritedAffinity)
        {
            IO::affinity(m_inheritedAffinity);
        }
        throw;
    }
    if (m_inheritedAffinity)
    {
        IO::affinity(m_inheritedAffinity);
    }
    return 0;
}