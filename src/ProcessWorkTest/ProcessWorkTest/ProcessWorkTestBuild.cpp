#include "ProcessWorkTestAutomation.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#ifdef _WIN32
#include <Windows.h>
#endif

/** 枚举一个源目录的直接文件，固定排序且不递归用户目录
@param [in] directory 已存在的源码目录
@return 文件名列表
*/
static std::vector<std::string> Files(const std::string& directory)
{
    std::vector<std::string> result;
#ifdef _WIN32
    WIN32_FIND_DATAA data;
    HANDLE search = FindFirstFileA((directory + "/*").c_str(), &data);
    bbCheck(search != INVALID_HANDLE_VALUE, "cannot enumerate " + directory);
    do
    {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            result.push_back(data.cFileName);
        }
    } while (FindNextFileA(search, &data));
    const DWORD error = GetLastError();
    FindClose(search);
    bbCheck(error == ERROR_NO_MORE_FILES, "source enumeration failed");
#else
    (void)directory;
    bbCheck(false, "private VS build is Windows only; use the existing CMake project on native POSIX");
#endif
    std::sort(result.begin(), result.end());
    return result;
}

/** 检查文件扩展名
@param [in] path 文件名
@param [in] suffix 点开头的扩展名
@return 是否匹配
*/
static bool Ends(const std::string& path, const std::string& suffix)
{
    return path.size() >= suffix.size() && path.compare(path.size() - suffix.size(), suffix.size(), suffix) == 0;
}

/** 给自有批处理构建参数加引号；拒绝cmd变量及元字符
@param [in] path 根目录、输出路径或其中派生的文件路径
@return 带引号的参数
*/
static std::string Quote(const std::string& path)
{
    bbCheck(path.find_first_of("\"%&|<>^!\r\n") == std::string::npos, "unsafe build path");
    return "\"" + path + "\"";
}

int32_t ProcessWorkTestAutomation::shell(const std::string& path)
{
#ifdef _WIN32
    bbCheck(Ends(path, "build.cmd"), "expected generated build.cmd");
    Quote(path);
    bbCheck(!bbRead(path).empty(), "missing build script");
    // cmd的/c尾部不是CRT参数数组；只在这个测试私有适配中使用其规定的双层引号。
    std::string command = "cmd.exe /d /s /c \"\"" + path + "\"\"";
    std::vector<char> writable(command.begin(), command.end());
    writable.push_back('\0');
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION child = {};
    bbCheck(CreateProcessA("C:\\Windows\\System32\\cmd.exe", &writable[0], nullptr, nullptr, TRUE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child) != FALSE, "cannot start build shell");
    CloseHandle(child.hThread);
    const DWORD wait = WaitForSingleObject(child.hProcess, 840000);
    DWORD code = 1;
    const bool completed = wait == WAIT_OBJECT_0 && GetExitCodeProcess(child.hProcess, &code) != FALSE;
    if (!completed)
    {
        TerminateProcess(child.hProcess, 124);
        WaitForSingleObject(child.hProcess, 5000);
    }
    CloseHandle(child.hProcess);
    bbCheck(completed, "private build timed out or status unavailable");
    return static_cast<int32_t>(code);
#else
    (void)path;
    bbCheck(false, "VS build shell requires Windows");
    return 1;
#endif
}

void ProcessWorkTestAutomation::build(const std::string& root, const std::string& output,
    const std::string& arch, const std::string& mode)
{
    bbCheck(arch == "x64" || arch == "x86", "architecture must be x64 or x86");
    bbCheck(mode == "tests" || mode == "dll" || mode == "benchmark" || mode == "memory" ||
        mode == "profile" || mode == "demo" || mode == "map", "unknown build mode");
    Quote(root); Quote(output);
#ifndef _WIN32
    bbCheck(false, "private VS build is Windows only; use CMake on native POSIX");
#endif
    directory(output);
    directory(output + "/source");
    directory(output + "/include");
    directory(output + "/test");
    const std::string support = root + "/src/CTaskThreadManagerTest/CTaskThreadManagerTest";
    const std::string suite = root + "/src/ProcessWorkTest/ProcessWorkTest";
    const char* modules[] = { "ProcessWork", "SharedMemory", "CTaskThreadManager", "CorrespondParam", "CStringManager",
        "CSystem", "LockFreeQueue", "AtomicMath", "Semaphore", "LockFreeMap", "LockFreeHashMap" };
    std::map<std::string, std::vector<std::string>> sources;
    std::ostringstream manifest;
    for (size_t i = 0; i < sizeof(modules) / sizeof(modules[0]); ++i)
    {
        const std::string module(modules[i]);
        const std::string directoryPath = root + "/src/" + module + "/" + module + "/src";
        directory(output + "/source/" + module);
        directory(output + "/include/" + module);
        const std::vector<std::string> files = Files(directoryPath);
        for (size_t j = 0; j < files.size(); ++j)
        {
            if (!Ends(files[j], ".cpp") && !Ends(files[j], ".h") && !Ends(files[j], ".inl"))
            {
                continue;
            }
            const std::string from = directoryPath + "/" + files[j];
            const std::string copied = output + "/source/" + module + "/" + files[j];
            bbWrite(copied, bbRead(from));
            manifest << from << '\t' << fingerprint(copied) << '\n';
            if (Ends(files[j], ".cpp"))
            {
                sources[module].push_back(copied);
            }
            else
            {
                bbWrite(output + "/include/" + module + "/" + files[j], bbRead(copied));
            }
        }
    }
    if (mode == "dll")
    {
        const std::string macro = bbRead(output + "/source/ProcessWork/ProcessWorkMacroDll.h");
        bbWrite(output + "/source/ProcessWork/ProcessWorkMacro.h", macro);
        bbWrite(output + "/include/ProcessWork/ProcessWorkMacro.h", macro);
    }
    std::vector<std::string> tests;
    const std::string testDirectories[] = { suite, support };
    for (int32_t i = 0; i < 2; ++i)
    {
        const std::vector<std::string> files = Files(testDirectories[i]);
        for (size_t j = 0; j < files.size(); ++j)
        {
            const std::string& name = files[j];
            if (i == 1 && name != "BlackBoxTestSupport.cpp" && name != "BlackBoxTestSupport.h" &&
                name != "CTaskPostCompatibility.h" && name != "CTaskPostCompatibility.inl")
            {
                continue;
            }
            if (!Ends(name, ".h") && !Ends(name, ".inl") && !Ends(name, ".cpp"))
            {
                continue;
            }
            const std::string copied = output + "/test/" + name;
            bbWrite(copied, bbRead(testDirectories[i] + "/" + name));
            manifest << testDirectories[i] + "/" + name << '\t' << fingerprint(copied) << '\n';
            if (Ends(name, ".cpp"))
            {
                tests.push_back(copied);
            }
        }
    }
    bbWrite(output + "/source-inputs.txt", manifest.str());
    if (mode == "memory" || mode == "profile")
    {
        bbWrite(output + "/include/ProcessWorkBenchmarkAudit.h", bbRead(suite + "/ProcessWorkBenchmarkAudit.h"));
        instrument(output, mode, output + "/test/ProcessWorkBenchmark.cpp");
    }
    if (mode == "demo")
    {
        const std::string source = plain(bbRead(output + "/source/ProcessWork/ProcessWork.cpp"));
        const std::string begin("/* PROCESS_WORK_USAGE_DEMO_BEGIN\n"), end("\nPROCESS_WORK_USAGE_DEMO_END */");
        const size_t first = source.find(begin), last = source.find(end);
        bbCheck(first != std::string::npos && last != std::string::npos && last > first &&
            source.find(begin, first + begin.size()) == std::string::npos, "usage demo markers missing or ambiguous");
        const std::string demo = output + "/test/ProcessWorkUsageDemo.cpp";
        writeCode(demo, source.substr(first + begin.size(), last - first - begin.size()));
        tests.assign(1, demo);
    }
    if (mode == "map")
    {
        tests.clear();
        const std::string mapSuite = root + "/src/LockFreeMapTest/LockFreeMapTest";
        const char* names[] = { "LockFreeMapTest", "LockFreeMapTestRunner", "LockFreeMapBasicTest", "LockFreeMapExamples",
            "LockFreeMapPlatformTest", "LockFreeMapTests", "LockFreeMapPoolTest", "LockFreeMapHistorical",
            "LockFreeMapHeadersTest", "LockFreeMapTestThread", "LockFreeMapTestCopyKey", "LockFreeMapContentionTest",
            "LockFreeMapStatusTest", "LockFreeMapArrivalTest", "LockFreeMapRetryTests", "LockFreeMapProgressTest" };
        const std::vector<std::string> headers = Files(mapSuite);
        for (size_t i = 0; i < headers.size(); ++i)
        {
            if (Ends(headers[i], ".h") || Ends(headers[i], ".inl"))
            {
                bbWrite(output + "/test/" + headers[i], bbRead(mapSuite + "/" + headers[i]));
            }
        }
        for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        {
            const std::string target = output + "/test/" + names[i] + ".cpp";
            bbWrite(target, bbRead(mapSuite + "/" + names[i] + ".cpp"));
            tests.push_back(target);
        }
    }
    std::ostringstream batch;
    batch << "@echo off\r\ncall \"C:\\Program Files (x86)\\Microsoft Visual Studio 12.0\\VC\\vcvarsall.bat\" " <<
        (arch == "x64" ? "amd64" : "x86") << "\r\nif errorlevel 1 exit /b 1\r\n";
    const std::string flags = "/nologo /W3 /c /EHsc /MT /O2 /Gy /DNDEBUG /DNOMINMAX /DSTATIC_LIB /D_CRT_SECURE_NO_WARNINGS /I" +
        Quote(output + "/include") + " /I" + Quote(output + "/test") + " /I" + Quote(output + "/include/LockFreeMap");
    const std::string systemLibraries = " /OPT:REF advapi32.lib user32.lib ole32.lib oleaut32.lib psapi.lib uuid.lib";
    std::string libraries, dllObjects;
    if (mode != "map")
    {
        for (std::map<std::string, std::vector<std::string>>::const_iterator module = sources.begin(); module != sources.end(); ++module)
        {
            std::string objects;
            for (size_t i = 0; i < module->second.size(); ++i)
            {
                const std::string object = output + "/" + module->first + "-" + bbNumber(i) + ".obj";
                batch << "cl " << flags << " /Fo" << Quote(object) << ' ' << Quote(module->second[i]) << "\r\nif errorlevel 1 exit /b 1\r\n";
                objects += " " + Quote(object);
            }
            if (objects.empty())
            {
                continue;
            }
            if (mode == "dll" && module->first == "ProcessWork")
            {
                dllObjects = objects;
                continue;
            }
            const std::string library = output + "/" + module->first + ".lib";
            batch << "lib /nologo /OUT:" << Quote(library) << objects << "\r\nif errorlevel 1 exit /b 1\r\n";
            libraries += " " + Quote(library);
        }
    }
    if (mode == "dll")
    {
        batch << "link /nologo /DLL /OUT:" << Quote(output + "/ProcessWork.dll") << " /IMPLIB:" << Quote(output + "/ProcessWork.lib") <<
            dllObjects << libraries << " /LIBPATH:" << Quote(output) << systemLibraries << "\r\nif errorlevel 1 exit /b 1\r\n";
        libraries += " " + Quote(output + "/ProcessWork.lib");
    }
    std::string objects;
    for (size_t i = 0; i < tests.size(); ++i)
    {
        const std::string object = output + "/Test-" + bbNumber(i) + ".obj";
        batch << "cl " << flags << " /DPROCESS_WORK_BENCH_IN_SUITE /DPROCESS_WORK_BENCH_RESULT_API /DPROCESS_WORK_BENCH_STORAGE_GROWTH";
        if (mode == "memory")
        {
            batch << " /DPROCESS_WORK_BENCH_MEMORY_AUDIT";
        }
        batch << " /Fo" << Quote(object) << ' ' << Quote(tests[i]) << "\r\nif errorlevel 1 exit /b 1\r\n";
        objects += " " + Quote(object);
    }
    const std::string executable = output + (mode == "demo" ? "/ProcessWorkUsageDemo.exe" : (mode == "map" ? "/LockFreeMapTest.exe" : "/ProcessWorkTest.exe"));
    batch << "link /nologo /OUT:" << Quote(executable) << objects << libraries << " /LIBPATH:" << Quote(output) << systemLibraries;
    if (mode == "map")
    {
        batch << " /NODEFAULTLIB:Semaphore.lib /NODEFAULTLIB:Semaphored.lib";
    }
    batch << "\r\nif errorlevel 1 exit /b 1\r\n";
    bbWrite(output + "/build.cmd", batch.str());
    std::ostringstream compiled;
    const std::string roots[] = { "source", "include", "test" };
    for (int32_t folder = 0; folder < 3; ++folder)
    {
        if (folder == 2)
        {
            const std::vector<std::string> files = Files(output + "/test");
            for (size_t i = 0; i < files.size(); ++i)
            {
                compiled << "test/" << files[i] << '\t' << fingerprint(output + "/test/" + files[i]) << '\n';
            }
            continue;
        }
        for (size_t m = 0; m < sizeof(modules) / sizeof(modules[0]); ++m)
        {
            const std::string dir = roots[folder] + "/" + modules[m];
            const std::vector<std::string> files = Files(output + "/" + dir);
            for (size_t i = 0; i < files.size(); ++i)
            {
                compiled << dir << '/' << files[i] << '\t' << fingerprint(output + "/" + dir + "/" + files[i]) << '\n';
            }
        }
    }
    bbWrite(output + "/compiled-inputs.txt", compiled.str());
    std::string batchPath = output + "/build.cmd";
    std::replace(batchPath.begin(), batchPath.end(), '/', '\\');
    execute(bbExecutable(), std::vector<std::string>{ "--tool", "shell", batchPath }, output + "/build.log", 900);
    bbWrite(output + "/binary-identity.txt", fingerprint(executable));
    if (mode == "demo")
    {
        execute(executable, std::vector<std::string>(), output + "/demo.log", 30);
        bbCheck(bbRead(output + "/demo.log").find("received=14") != std::string::npos, "demo completion count missing");
    }
    if (mode == "map")
    {
        execute(executable, std::vector<std::string>{ "--all", "--report", output + "/map.csv" }, output + "/map.log", 900);
    }
}