#ifdef _WIN32
#include "LogManagerTest.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <windows.h>
#include <io.h>
#include <chrono>
#include <thread>
#include <functional>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <vector>

/** 补上目录末尾的分隔符
@param [in] path 私有测试目录
@return 可拼接文件名的目录
*/
static std::string NormalizeLogDir(const std::string& path)
{
    return path + "/";
}

/** 枚举本层文件和链接
@param [in] logDir 测试目录
@return 本层文件的路径
*/
static std::vector<std::string> ListTopFiles(const std::string& logDir)
{
    std::vector<std::string> result;
    std::string realLogDir = NormalizeLogDir(logDir);
    std::string findPath = realLogDir + "*";
    struct _finddata_t fileInfo;
    intptr_t handle = _findfirst(findPath.c_str(), &fileInfo);
    if (handle == -1)
    {
        return result;
    }
    do
    {
        std::string name = fileInfo.name;
        if (name == "." || name == "..")
        {
            continue;
        }
        if ((fileInfo.attrib & _A_SUBDIR) == 0)
        {
            result.push_back(realLogDir + name);
        }
    }
    while (_findnext(handle, &fileInfo) == 0);
    _findclose(handle);
    return result;
}

/** 截取文件名
@param [in] path 文件路径
@return 文件名
*/
static std::string TestFileName(const std::string& path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
    {
        return path;
    }
    return path.substr(pos + 1);
}

/** 检查非空十进制整数文本
@param [in] value 验证文本
@return 全为数字返回 true
*/
static bool IsNumber(const std::string& value)
{
    return !value.empty() && value.find_first_not_of("0123456789") == std::string::npos;
}

/** 独立解析会话名及卷号以核对文件归属
@param [in] filePath 文件路径
@param [in] processName 测试程序名
@param [out] oldFile 解析成功时填写的会话身份
@return 名称符合约定时返回 true
*/
static bool ParseLogName(const std::string& filePath, const std::string& processName, LogTestFileName& oldFile)
{
    std::string name = TestFileName(filePath);
    if (name.size() <= 4 || name.substr(name.size() - 4) != ".log")
    {
        return false;
    }

    std::string withoutSuffix = name.substr(0, name.size() - 4);
    int32_t fileIndex = -1;
    std::string baseName = withoutSuffix;
    size_t dotPos = withoutSuffix.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        std::string indexText = withoutSuffix.substr(dotPos + 1);
        if (IsNumber(indexText))
        {
            fileIndex = static_cast<int32_t>(std::atoi(indexText.c_str()));
            baseName = withoutSuffix.substr(0, dotPos);
        }
    }

    std::string prefix = processName + "_";
    if (baseName.find(prefix) != 0)
    {
        return false;
    }
    std::string remain = baseName.substr(prefix.size());
    std::vector<std::string> parts = CStringManager::split(remain, "_");
    if ((parts.size() != 3 && parts.size() != 4) || !IsNumber(parts[0]) || !IsNumber(parts[1]) || !IsNumber(parts[2]) ||
        (parts.size() == 4 && !IsNumber(parts[3])))
    {
        return false;
    }

    oldFile.m_fileName = name;
    oldFile.m_baseName = baseName;
    oldFile.m_pid = static_cast<int32_t>(std::atoi(parts[0].c_str()));
    oldFile.m_date = parts[1];
    oldFile.m_time = parts[2];
    oldFile.m_fileIndex = fileIndex;
    return true;
}

int RunLogChild(int argc, char** argv)
{
    // 同一测试程序的子进程分支，供父进程检查析构和多实例隔离。
    if (argc == 6 && std::string(argv[1]) == "--archive-child")
    {
        const std::string mode = argv[3];
        LogManagerConfig config;
        config.m_fileId = 17;
        config.m_path = argv[2];
        config.m_maxFileBytes = 700;
        config.m_maxFileCount = 3;
        config.m_checkFileSizeInterval = 1;
        config.m_archiveOldLog = mode != "noarchive";
        LogManager::instance().init(config);
        if (mode == "multi")
        {
            config.m_fileId = 18;
            LogManager::instance().init(config);
        }
        const std::string payload(240, 'x');
        for (int i = 0; i < 30; ++i)
        {
            LOGINFO_EX(17, "OWNER17 row=%d %s", i, payload.c_str());
            if (mode == "multi") { LOGINFO_EX(18, "OWNER18 row=%d %s", i, payload.c_str()); }
        }
        std::ofstream(argv[4]) << "ready";
        const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
        while (!std::ifstream(argv[5]).good())
        {
            if (std::chrono::steady_clock::now() >= deadline) { return 3; }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        LOGINFO_EX(17, "LAST17 before closing");
        if (mode == "destructor") { return 0; }
        if (mode == "delete")
        {
            LogManager::instance().deleteFile(17);
            return 0;
        }
        LogManager::instance().uninit(17);
        if (mode == "reinit")
        {
            LogManager::instance().init(config);
            LOGINFO_EX(17, "SECOND_SESSION");
            LogManager::instance().uninit(17);
        }
        if (mode == "multi") { LOGINFO_EX(18, "LAST18 still writable after closing 17"); }
        LogManager::instance().uninit(17);
        LogManager::instance().uninitAll();
        LogManager::instance().uninitAll();
        return 0;
    }
    if (argc >= 3 && std::string(argv[1]) == "--child")
    {
        LogManagerConfig childConfig;
        childConfig.m_fileId = 0;
        childConfig.m_path = argv[2];
        childConfig.m_maxFileBytes = 1024;
        childConfig.m_maxFileCount = 5;
        childConfig.m_checkFileSizeInterval = 1;
        childConfig.m_archiveOldLog = true;
        LogManager::instance().init(childConfig);
        for (int32_t i = 0; i < 300; ++i)
        {
            LOGINFO("child process macro log %d %s", i, "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
        }
        LogManager::instance().uninitAll();
        return 0;
    }

    return 2;
}

int CheckLogProcesses()
{
    int32_t totalCount = 0;
    int32_t failCount = 0;
    int32_t skipCount = 0;
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
        if (!ok)
        {
            ++failCount;
        }
    };
    std::function<void(const std::string&)> skip = [&skipCount](const std::string& name) -> void
    {
        ++skipCount;
        std::cout << "[SKIP] " << name << std::endl;
    };

    // 仅测试使用的局部辅助逻辑：读取实际目录与文件，不访问日志管理器私有状态。
    auto directories = [](const std::string& path) -> std::vector<std::string>
    {
        std::vector<std::string> result;
        std::string directory = NormalizeLogDir(path);
        WIN32_FIND_DATAA data;
        HANDLE find = FindFirstFileA((directory + "*").c_str(), &data);
        if (find == INVALID_HANDLE_VALUE) { return result; }
        do
        {
            std::string name = data.cFileName;
            if (name != "." && name != ".." && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                !(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) { result.push_back(directory + name); }
        } while (FindNextFileA(find, &data));
        FindClose(find);
        return result;
    };
    std::function<std::vector<std::string>(const std::string&)> allFiles;
    allFiles = [&allFiles, &directories](const std::string& path) -> std::vector<std::string>
    {
        std::vector<std::string> result = ListTopFiles(path);
        std::vector<std::string> dirs = directories(path);
        for (size_t i = 0; i < dirs.size(); ++i)
        {
            std::vector<std::string> nested = allFiles(dirs[i]);
            result.insert(result.end(), nested.begin(), nested.end());
        }
        return result;
    };
    auto readFile = [](const std::string& path) -> std::string
    {
        HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) { throw std::runtime_error("cannot read " + path); }
        std::string text;
        char bytes[4096];
        DWORD count = 0;
        BOOL ok = TRUE;
        while ((ok = ReadFile(file, bytes, sizeof(bytes), &count, nullptr)) && count != 0) { text.append(bytes, count); }
        CloseHandle(file);
        if (!ok) { throw std::runtime_error("read failed " + path); }
        return text;
    };

    std::function<int32_t(const std::string&)> countTopFiles = [](const std::string& path) -> int32_t
    {
        return static_cast<int32_t>(ListTopFiles(path).size());
    };
    std::function<std::string(const std::string&)> readLogText = [&allFiles](const std::string& path) -> std::string
    {
        std::ostringstream oss;
        std::vector<std::string> files = allFiles(path);
        for (size_t i = 0; i < files.size(); ++i)
        {
            LogTestFileName oldFile;
            if (!ParseLogName(files[i], CSystem::GetCurrentExeName(), oldFile) || oldFile.m_fileIndex < 0)
            {
                continue;
            }
            std::ifstream input(files[i].c_str(), std::ios::in | std::ios::binary);
            oss << input.rdbuf();
        }
        return oss.str();
    };
    std::function<int32_t(const std::string&, const std::string&)> countToken = [](const std::string& text, const std::string& token) -> int32_t
    {
        int32_t count = 0;
        size_t pos = 0;
        while ((pos = text.find(token, pos)) != std::string::npos)
        {
            ++count;
            pos += token.size();
        }
        return count;
    };
    LogSession session;
    if (session.config.m_path.empty()) { return 1; }
    std::string root = session.config.m_path + "/";
    std::string linkCheckTarget = root + "link_check_target.log";
    std::string linkCheckPath = root + "link_check.log";
    {
        std::ofstream targetFile(linkCheckTarget.c_str(), std::ios::out | std::ios::app);
        targetFile << "target" << std::endl;
    }
    bool canCreateSymbolicLink = (CreateSymbolicLinkA(linkCheckPath.c_str(), linkCheckTarget.c_str(), 0) != FALSE);
    CSystem::deleteFile(linkCheckPath.c_str());
    CSystem::deleteFile(linkCheckTarget.c_str());

    if (canCreateSymbolicLink)
    {
        check(CheckBusyLogLink() == 0, "LogManager preserves rotated files while a reader pins the link");
    }
    else
    {
        skip("LogManager busy link, current process has no symbolic link privilege");
    }

    LogManagerConfig config;
    config.m_fileId = 1004;
    config.m_path = root;
    config.m_maxFileBytes = 1024;
    config.m_maxFileCount = 5;
    config.m_checkFileSizeInterval = 1;
    config.m_outputConsole = false;
    config.m_archiveOldLog = true;
    LogManager::instance().init(config);
    check(LogManager::instance().findFileId(root) == 1004, "LogManager init and findFileId");
    for (int32_t i = 0; i < 500; ++i)
    {
        LOGINFO_EX(1004, "single process rolling macro log %d %s", i, "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    }
    std::vector<std::string> files = ListTopFiles(root);
    int32_t entityCount = 0;
    bool hasZero = false;
    bool hasLink = false;
    int32_t maxIndex = -1;
    std::string maxIndexPath;
    std::string zeroPath;
    std::string linkPath;
    for (size_t i = 0; i < files.size(); ++i)
    {
        LogTestFileName oldFile;
        if (ParseLogName(files[i], CSystem::GetCurrentExeName(), oldFile) && oldFile.m_fileIndex >= 0)
        {
            ++entityCount;
            if (oldFile.m_fileIndex == 0)
            {
                hasZero = true;
                zeroPath = files[i];
            }
            if (oldFile.m_fileIndex > maxIndex)
            {
                maxIndex = oldFile.m_fileIndex;
                maxIndexPath = files[i];
            }
        }
        else if (ParseLogName(files[i], CSystem::GetCurrentExeName(), oldFile) && oldFile.m_fileIndex < 0)
        {
            hasLink = true;
            linkPath = files[i];
        }
    }
    check(hasZero, "LogManager keep zero file");
    if (canCreateSymbolicLink)
    {
        check(hasLink, "LogManager create link");
    }
    else
    {
        skip("LogManager create link, current process has no symbolic link privilege");
    }
    check(entityCount <= 5, "LogManager rolling delete limit");
    if (!maxIndexPath.empty())
    {
        check(!CSystem::deleteFile(maxIndexPath.c_str()), "LogManager lock current entity file");
    }
    if (canCreateSymbolicLink && !linkPath.empty())
    {
        check(!CSystem::deleteFile(linkPath.c_str()), "LogManager lock link file");
    }
    else if (!canCreateSymbolicLink)
    {
        skip("LogManager lock link file, current process has no symbolic link privilege");
    }
    if (!zeroPath.empty() && maxIndex > 0)
    {
        check(CSystem::deleteFile(zeroPath.c_str()), "LogManager old entity can delete outside");
    }
    LogManager::instance().uninit(1004);

    std::string archiveRoot = root + "archive_old/";
    CSystem::createDirectories(archiveRoot);
    std::string archiveOldName = CSystem::GetCurrentExeName() + "_456_20260809_034122.0.log";
    {
        std::ofstream oldFile((archiveRoot + archiveOldName).c_str(), std::ios::out | std::ios::app);
        oldFile << "old" << std::endl;
    }
    LogManagerConfig archiveConfig;
    archiveConfig.m_fileId = 1005;
    archiveConfig.m_path = archiveRoot;
    archiveConfig.m_maxFileBytes = 2048;
    archiveConfig.m_checkFileSizeInterval = 1;
    archiveConfig.m_archiveOldLog = true;
    LogManager::instance().init(archiveConfig);
    LogManager::instance().uninit(1005);
    check(CSystem::DirOrFileExist(archiveRoot + archiveOldName), "LogManager leaves old log in original directory");

    std::string deleteRoot = root + "delete_old/";
    CSystem::createDirectories(deleteRoot);
    std::string oldName = CSystem::GetCurrentExeName() + "_123_20260809_034122.0.log";
    {
        std::ofstream oldFile((deleteRoot + oldName).c_str(), std::ios::out | std::ios::app);
        oldFile << "old" << std::endl;
    }
    LogManagerConfig deleteConfig;
    deleteConfig.m_fileId = 1006;
    deleteConfig.m_path = deleteRoot;
    deleteConfig.m_archiveOldLog = false;
    LogManager::instance().init(deleteConfig);
    LogManager::instance().uninit(1006);
    check(CSystem::DirOrFileExist(deleteRoot + oldName), "LogManager disabled archive also preserves old logs");

    check(LogManager::needPrintEvery(0, -1), "LogManager every first");
    check(!LogManager::needPrintEvery(1, -1), "LogManager once only");
    check(LogManager::needPrintEvery(1, 0), "LogManager every zero interval");
    check(LogManager::needPrintEvery(1000, 1000), "LogManager interval write");
    check(!LogManager::needPrintEvery(999, 1000), "LogManager interval skip");

    LogManagerConfig threadConfig;
    threadConfig.m_fileId = 1007;
    threadConfig.m_path = root + "thread/";
    threadConfig.m_maxFileBytes = 10LL * 1024LL * 1024LL;
    threadConfig.m_maxFileCount = 5;
    threadConfig.m_checkFileSizeInterval = 100;
    LogManager::instance().init(threadConfig);

    std::vector<std::thread> threads;
    for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
    {
        threads.push_back(std::thread([threadIndex]() -> void
        {
            for (int32_t loopIndex = 0; loopIndex < 500; ++loopIndex)
            {
                LOGINFO_EX(1007, "thread macro log thread=%d loop=%d", threadIndex, loopIndex);
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    LogManager::instance().uninit(1007);
    std::string threadText = readLogText(threadConfig.m_path);
    check(countToken(threadText, "thread macro log") == 8 * 500, "LogManager multithread macro line count");

    std::string deleteFileRoot = root + "delete_file/";
    LogManagerConfig deleteFileConfig;
    deleteFileConfig.m_fileId = 1008;
    deleteFileConfig.m_path = deleteFileRoot;
    deleteFileConfig.m_maxFileBytes = 512;
    deleteFileConfig.m_maxFileCount = 5;
    deleteFileConfig.m_checkFileSizeInterval = 1;
    LogManager::instance().init(deleteFileConfig);
    for (int32_t i = 0; i < 200; ++i)
    {
        LOGINFO_EX(1008, "delete file macro log %d %s", i, "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
    }
    check(countTopFiles(deleteFileRoot) > 0, "LogManager deleteFile has files before delete");
    LogManager::instance().deleteFile(1008);
    check(countTopFiles(deleteFileRoot) == 0, "LogManager deleteFile removes current files");

    std::string normalizeRoot = root + "normalize_config/";
    LogManagerConfig normalizeConfig;
    normalizeConfig.m_fileId = 1009;
    normalizeConfig.m_path = normalizeRoot;
    normalizeConfig.m_maxFileBytes = 0;
    normalizeConfig.m_maxFileCount = 1;
    normalizeConfig.m_checkFileSizeInterval = 0;
    LogManager::instance().init(normalizeConfig);
    LOGINFO_EX(1009, "normalize config macro log");
    LogManager::instance().uninit(1009);
    check(countToken(readLogText(normalizeRoot), "normalize config macro log") == 1, "LogManager normalize invalid config values");

    std::string relativeRoot = "LogManagerRelativeTest_" + std::to_string(GetCurrentProcessId()) + "/";
    if (CSystem::DirOrFileExist(relativeRoot)) { return 1; }
    LogManagerConfig relativeConfig;
    relativeConfig.m_fileId = 1011;
    relativeConfig.m_path = relativeRoot;
    relativeConfig.m_maxFileBytes = 1024 * 1024;
    relativeConfig.m_checkFileSizeInterval = 1;
    LogManager::instance().init(relativeConfig);
    LOGINFO_EX(1011, "relative link macro log");
    LogManager::instance().uninit(1011);
    if (canCreateSymbolicLink)
    {
        std::vector<std::string> relativeFiles = allFiles(relativeRoot);
        std::string relativeLinkPath;
        for (size_t i = 0; i < relativeFiles.size(); ++i)
        {
            LogTestFileName oldFile;
            if (ParseLogName(relativeFiles[i], CSystem::GetCurrentExeName(), oldFile) && oldFile.m_fileIndex < 0)
            {
                relativeLinkPath = relativeFiles[i];
                break;
            }
        }
        std::ifstream relativeInput(relativeLinkPath.c_str(), std::ios::in | std::ios::binary);
        std::ostringstream relativeText;
        relativeText << relativeInput.rdbuf();
        check(countToken(relativeText.str(), "relative link macro log") == 1, "LogManager relative path symbolic link target");
    }
    else
    {
        skip("LogManager relative path symbolic link target, current process has no symbolic link privilege");
    }

    RemoveLogTestTree(relativeRoot);
    std::string switchRoot = root + "write_switch/";
    LogManagerConfig switchConfig;
    switchConfig.m_fileId = 1010;
    switchConfig.m_path = switchRoot;
    switchConfig.m_maxFileBytes = 1024 * 1024;
    switchConfig.m_checkFileSizeInterval = 1;
    LogManager::instance().init(switchConfig);
    LogManager::instance().set(false, true);
    LOGINFO_EX(1010, "write switch disabled log");
    LogManager::instance().set(true, true);
    LOGINFO_EX(1010, "write switch enabled log");
    LogManager::instance().uninit(1010);
    std::string switchText = readLogText(switchRoot);
    check(countToken(switchText, "write switch disabled log") == 0, "LogManager write switch disabled");
    check(countToken(switchText, "write switch enabled log") == 1, "LogManager write switch enabled");

    std::string processRoot = root + "process/";
    CSystem::createDirectories(processRoot);
    std::string exePath = CSystem::GetCurrentExePath() + CSystem::GetCurrentExeFullName();
    std::vector<std::thread> processThreads;
    std::atomic<int32_t> processFailCount(0);
    for (int32_t processIndex = 0; processIndex < 4; ++processIndex)
    {
        processThreads.push_back(std::thread([exePath, processRoot, &processFailCount]() -> void
        {
            std::string command = "\"" + exePath + "\" --child \"" + processRoot + "\"";
            STARTUPINFOA startup = {};
            startup.cb = sizeof(startup);
            PROCESS_INFORMATION child = {};
            DWORD result = 1;
            if (CreateProcessA(exePath.c_str(), &command[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child))
            {
                if (WaitForSingleObject(child.hProcess, 30000) == WAIT_OBJECT_0) { GetExitCodeProcess(child.hProcess, &result); }
                CloseHandle(child.hThread);
                CloseHandle(child.hProcess);
            }
            if (result != 0)
            {
                processFailCount.fetch_add(1);
            }
        }));
    }
    for (size_t i = 0; i < processThreads.size(); ++i)
    {
        processThreads[i].join();
    }
    check(processFailCount.load() == 0, "LogManager multiprocess macro pressure");
    check(!allFiles(processRoot).empty() && countTopFiles(processRoot) == 0, "LogManager multiprocess output archived on close");

    // 归档集成回归：只创建本测试目录和本测试子进程，失败时也负责回收子进程。
    {
        const std::string processName = CSystem::GetCurrentExeName();
        const std::string executable = CSystem::GetCurrentExePath() + CSystem::GetCurrentExeFullName();
        const std::string suiteRoot = root + "archive_close/";
        CSystem::createDirectories(suiteRoot);
        std::vector<PROCESS_INFORMATION> children;
        std::map<DWORD, std::string> stops;
        std::map<DWORD, std::string> paths;
        auto require = [&check](bool ok, const std::string& name) -> void
        {
            check(ok, name);
            if (!ok) { throw std::runtime_error(name); }
        };
        auto start = [&](const std::string& path, const std::string& mode) -> DWORD
        {
            CSystem::createDirectories(path);
            std::ostringstream ticket;
            ticket << suiteRoot << "child_" << children.size();
            std::string ready = ticket.str() + ".ready";
            std::string stop = ticket.str() + ".stop";
            std::string command = "\"" + executable + "\" --archive-child \"" + path + "\" " + mode +
                " \"" + ready + "\" \"" + stop + "\"";
            std::vector<char> buffer(command.begin(), command.end());
            buffer.push_back('\0');
            STARTUPINFOA startup = {};
            startup.cb = sizeof(startup);
            PROCESS_INFORMATION child = {};
            require(CreateProcessA(executable.c_str(), &buffer[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                nullptr, nullptr, &startup, &child) != FALSE, "archive child starts");
            CloseHandle(child.hThread);
            child.hThread = nullptr;
            children.push_back(child);
            stops[child.dwProcessId] = stop;
            paths[child.dwProcessId] = path;
            auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            while (!CSystem::DirOrFileExist(ready))
            {
                if (WaitForSingleObject(child.hProcess, 0) != WAIT_TIMEOUT) { throw std::runtime_error("archive child exited before ready"); }
                if (std::chrono::steady_clock::now() >= deadline) { throw std::runtime_error("archive child ready timeout"); }
                std::this_thread::sleep_for(std::chrono::milliseconds(25));
            }
            require(true, "archive child ready within timeout");
            return child.dwProcessId;
        };
        auto finish = [&](DWORD pid) -> void
        {
            std::ofstream(stops.at(pid).c_str()) << "stop";
            for (size_t i = 0; i < children.size(); ++i)
            {
                if (children[i].dwProcessId != pid) { continue; }
                require(WaitForSingleObject(children[i].hProcess, 10000) == WAIT_OBJECT_0, "archive child exits within timeout");
                DWORD code = 0;
                require(GetExitCodeProcess(children[i].hProcess, &code) && code == 0, "archive child exit status");
                return;
            }
            throw std::runtime_error("unknown archive child");
        };
        auto ownFiles = [&](DWORD pid) -> std::vector<std::string>
        {
            std::vector<std::string> result;
            std::vector<std::string> files = ListTopFiles(paths.at(pid));
            for (size_t i = 0; i < files.size(); ++i)
            {
                LogTestFileName info;
                if (ParseLogName(files[i], processName, info) &&
                    info.m_pid == static_cast<int32_t>(pid)) { result.push_back(files[i]); }
            }
            return result;
        };
        auto entityCount = [&](const std::vector<std::string>& files) -> size_t
        {
            size_t count = 0;
            for (size_t i = 0; i < files.size(); ++i)
            {
                LogTestFileName info;
                if (ParseLogName(files[i], processName, info) && info.m_fileIndex >= 0) { ++count; }
            }
            return count;
        };
        auto snapshot = [&](DWORD pid) -> std::map<std::string, std::string>
        {
            std::map<std::string, std::string> result;
            std::vector<std::string> files = ownFiles(pid);
            for (size_t i = 0; i < files.size(); ++i) { result[files[i]] = readFile(files[i]); }
            return result;
        };
        auto unchanged = [&](const std::map<std::string, std::string>& saved, const std::string& name) -> void
        {
            for (auto it = saved.begin(); it != saved.end(); ++it)
            {
                require(CSystem::DirOrFileExist(it->first) && readFile(it->first) == it->second, name);
            }
        };
        auto verifyArchive = [&](DWORD pid, size_t expectedSessions) -> std::string
        {
            require(ownFiles(pid).empty(), "normal close removes only own top-level logs");
            std::vector<std::string> dirs = directories(paths.at(pid) + "/" + processName + "_log");
            size_t sessions = 0;
            std::string combined;
            for (size_t d = 0; d < dirs.size(); ++d)
            {
                std::vector<std::string> files = ListTopFiles(dirs[d]);
                std::string text, link, latest;
                int32_t maxIndex = -1;
                size_t count = 0, zero = 0, links = 0;
                for (size_t f = 0; f < files.size(); ++f)
                {
                    LogTestFileName info;
                    if (!ParseLogName(files[f], processName, info) || info.m_pid != static_cast<int32_t>(pid)) { continue; }
                    if (info.m_fileIndex < 0) { ++links; link = files[f]; continue; }
                    ++count;
                    if (info.m_fileIndex == 0) { ++zero; }
                    if (info.m_fileIndex > maxIndex) { maxIndex = info.m_fileIndex; latest = files[f]; }
                    text += readFile(files[f]);
                }
                if (count == 0) { continue; }
                ++sessions;
                require(count <= 3 && zero == 1, "archive retains zero and rolling file limit");
                require(text.find("BEGIN") != std::string::npos && text.find("END") != std::string::npos, "BEGIN and END archived");
                require(links == (canCreateSymbolicLink ? 1U : 0U), "owned symbolic link archived");
                if (canCreateSymbolicLink)
                {
                    require(readFile(link) == readFile(latest), "archived relative link resolves to latest file");
                }
                combined += text;
            }
            require(sessions == expectedSessions, "separate archive directory per session");
            return combined;
        };
        try
        {
            const char* modes[] = { "explicit", "destructor", "reinit", "multi", "noarchive", "delete" };
            for (size_t m = 0; m < sizeof(modes) / sizeof(modes[0]); ++m)
            {
                std::string mode = modes[m], path = suiteRoot + mode;
                CSystem::createDirectories(path);
                std::string old = path + "/" + processName + "_123_20000101_010101.0.log";
                std::ofstream(old.c_str()) << "untouched crash evidence";
                DWORD pid = start(path, mode);
                require(readFile(old) == "untouched crash evidence", mode + ": startup preserves residual log");
                size_t expected = mode == "multi" ? 2 : 1;
                require(entityCount(ownFiles(pid)) == 3 * expected, mode + ": live rolling files remain at top level");
                require(!CSystem::DirOrFileExist(path + "/" + processName + "_log"), mode + ": no archive before close");
                finish(pid);
                require(readFile(old) == "untouched crash evidence", mode + ": close preserves residual log");
                if (mode == "noarchive")
                {
                    require(entityCount(ownFiles(pid)) == 3, "disabled archive leaves files in place");
                    require(!CSystem::DirOrFileExist(path + "/" + processName + "_log"), "disabled archive creates no history");
                }
                else if (mode == "delete")
                {
                    require(ownFiles(pid).empty(), "deleteFile removes owned files");
                    require(!CSystem::DirOrFileExist(path + "/" + processName + "_log"), "deleteFile does not archive");
                }
                else
                {
                    std::string text = verifyArchive(pid, mode == "reinit" ? 2 : expected);
                    require(text.find("LAST17") != std::string::npos, "last message retained");
                    if (mode == "reinit") { require(text.find("SECOND_SESSION") != std::string::npos, "reopened session retained"); }
                    if (mode == "multi") { require(text.find("LAST18") != std::string::npos, "second ID writable after first closes"); }
                }
            }
            std::string shared = suiteRoot + "multiple_processes";
            DWORD first = start(shared, "explicit");
            auto firstSnapshot = snapshot(first);
            DWORD second = start(shared, "explicit");
            unchanged(firstSnapshot, "second startup preserves first process files");
            auto secondSnapshot = snapshot(second);
            finish(first);
            verifyArchive(first, 1);
            unchanged(secondSnapshot, "first close preserves second process files");
            finish(second);
            verifyArchive(second, 1);
            std::string collision = suiteRoot + "collision";
            DWORD pid = start(collision, "explicit");
            std::vector<std::string> files = ownFiles(pid);
            std::string zero;
            LogTestFileName info;
            for (size_t i = 0; i < files.size(); ++i)
            {
                if (ParseLogName(files[i], processName, info) && info.m_fileIndex == 0) { zero = files[i]; break; }
            }
            require(!zero.empty(), "collision setup has first entity");
            std::string original = readFile(zero);
            std::string foreign = collision + "/" + info.m_baseName + ".999999.log";
            std::ofstream(foreign.c_str()) << "not created by logger";
            std::string archive = collision + "/" + processName + "_log/" + info.m_date + "_" + info.m_time + "_" + CStringManager::toStringInt32(info.m_pid);
            CSystem::createDirectories(archive);
            std::string target = archive + "/" + info.m_fileName;
            std::ofstream(target.c_str()) << "existing archive";
            finish(pid);
            require(readFile(foreign) == "not created by logger", "unowned matching-name file not moved");
            require(readFile(target) == "existing archive", "target collision does not overwrite");
            require(readFile(zero) == original, "failed move retains source bytes");
            std::string blocked = suiteRoot + "blocked";
            pid = start(blocked, "explicit");
            std::ofstream((blocked + "/" + processName + "_log").c_str()) << "directory blocker";
            finish(pid);
            require(entityCount(ownFiles(pid)) == 3, "archive directory failure preserves sources");
            std::string abnormal = suiteRoot + "abnormal_exit";
            DWORD crashed = start(abnormal, "explicit");
            const auto beforeExit = snapshot(crashed);
            // 子进程已提交并在等待父进程信号。用本次创建的句柄模拟宿主被外部结束。
            const PROCESS_INFORMATION& child = children.back();
            require(child.dwProcessId == crashed && TerminateProcess(child.hProcess, 17) != FALSE, "stop owned child externally");
            require(WaitForSingleObject(child.hProcess, 10000) == WAIT_OBJECT_0, "externally stopped child exited");
            DWORD stoppedCode = 0;
            require(GetExitCodeProcess(child.hProcess, &stoppedCode) != FALSE && stoppedCode == 17, "external stop status");
            unchanged(beforeExit, "external stop preserves committed bytes");
            auto crashSnapshot = snapshot(crashed);
            require(crashSnapshot.size() == (canCreateSymbolicLink ? 4U : 3U), "abnormal exit leaves entities and link");
            DWORD next = start(abnormal, "explicit");
            finish(next);
            verifyArchive(next, 1);
            unchanged(crashSnapshot, "later startup and shutdown preserve crash evidence");
        }
        catch (const std::exception& exception)
        {
            check(false, std::string("archive suite exception: ") + exception.what());
        }
        for (size_t i = 0; i < children.size(); ++i)
        {
            if (WaitForSingleObject(children[i].hProcess, 0) == WAIT_TIMEOUT)
            {
                std::ofstream(stops.at(children[i].dwProcessId).c_str()) << "stop";
                WaitForSingleObject(children[i].hProcess, 30000);
            }
            CloseHandle(children[i].hProcess);
        }
    }
    LogManager::instance().uninitAll();
    std::cout << "LogManager test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
    return failCount == 0 ? 0 : 1;
}
#endif