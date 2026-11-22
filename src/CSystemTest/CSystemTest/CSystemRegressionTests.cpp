#include "CSystemTest.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#if CSYSTEM_HAS_NATIVE_API
#include <algorithm>
#include <atomic>
#include <thread>
#ifndef _WIN32
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#endif
#endif

#if CSYSTEM_HAS_NATIVE_API
/** 在本进程的独占目录验证环境变更、仓库根路径和版本选择，退出时恢复原环境
@param [in] root 本轮已创建的临时目录
@return 可正确看到环境变更并找到预期版本时返回 true
*/
static bool CheckCommonProgram(const std::string& root)
{
    const char* key = "XUEYELINGSHUANG";
#ifdef _WIN32
    std::vector<wchar_t> original(32768);
    ::SetLastError(ERROR_SUCCESS);
    const DWORD count = ::GetEnvironmentVariableW(L"XUEYELINGSHUANG", original.data(), static_cast<DWORD>(original.size()));
    const DWORD readError = ::GetLastError();
    if (count >= original.size() || (count == 0 && readError != ERROR_SUCCESS && readError != ERROR_ENVVAR_NOT_FOUND))
    {
        return false;
    }
    const bool present = count != 0 || readError == ERROR_SUCCESS;
    const auto restore = [&](const char*) {
        ::SetEnvironmentVariableW(L"XUEYELINGSHUANG", present ? original.data() : nullptr);
    };
    const auto setRoot = [&](const std::string& value) { return ::SetEnvironmentVariableA(key, value.c_str()) != FALSE; };
    const char* suffix = ".exe";
#else
    const char* previous = std::getenv(key);
    const bool present = previous != nullptr;
    const std::string original = present ? previous : "";
    const auto restore = [&](const char*) {
        if (present)
        {
            ::setenv(key, original.c_str(), 1);
        }
        else
        {
            ::unsetenv(key);
        }
    };
    const auto setRoot = [&](const std::string& value) { return ::setenv(key, value.c_str(), 1) == 0; };
    const char* suffix = "";
#endif
    std::unique_ptr<const char, decltype(restore)> environment(key, restore);
    const std::string directory = root + "/common";
    if (!CSystem::CreateDir(directory))
    {
        return false;
    }
    const std::string older = directory + "/CSystemLookup1.0" + suffix;
#ifdef _WIN32
    const std::string newer = directory + "/CSystemLookup2.5.EXE";
#else
    const std::string newer = directory + "/CSystemLookup2.5";
    const std::string literal = directory + "/part\\CSystemLookup9.9";
    CSystem::saveFile("different basename", literal);
#endif
    const std::string numberOnly = directory + "/42.0" + suffix;
    CSystem::saveFile("no prefix", numberOnly);
    CSystem::saveFile("older", older);
    CSystem::saveFile("newer", newer);
    const bool filesReady = CSystem::readFile(older) == "older" && CSystem::readFile(newer) == "newer";
    const bool withoutSlash = setRoot(root) && CSystem::GetEnvironment(key) == root &&
        CSystem::readFile(CSystem::commonFile("CSystemLookup")) == "newer";
    const bool withSlash = setRoot(root + "/") && CSystem::GetEnvironment(key) == root + "/" &&
        CSystem::readFile(CSystem::commonFile("CSystemLookup")) == "newer";
    const bool rejectsEmpty = CSystem::commonFile("").empty();
    const bool removedOlder = CSystem::deleteFile(older.c_str());
    const bool removedNewer = CSystem::deleteFile(newer.c_str());
    const bool removedNumber = CSystem::deleteFile(numberOnly.c_str());
#ifndef _WIN32
    const bool removedLiteral = CSystem::deleteFile(literal.c_str());
    if (!removedLiteral)
    {
        return false;
    }
#endif
    const bool removedDirectory = CSystem::DestroyDir(directory);
    return filesReady && withoutSlash && withSlash && rejectsEmpty && removedOlder && removedNewer && removedNumber && removedDirectory;
}
#endif

/** 验证本进程的系统接口，文件操作限制在本轮新建的独占目录
@return 所有已执行检查均通过时返回 0
*/
int RunSystemInterfaceTests()
{
    int32_t totalCount = 0;
    int32_t failCount = 0;
    // 累计每个断言并输出名称，允许后续清理继续执行。
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
        if (!ok)
        {
            ++failCount;
        }
    };

#if CSYSTEM_HAS_NATIVE_API
    check(CSystem::currentProcessPid() > 0, "CSystem currentProcessPid");
    check(CSystem::SystemThreadId() > 0, "CSystem SystemThreadId");
    check(!CSystem::GetCurrentExePath().empty(), "CSystem GetCurrentExePath");
    check(!CSystem::GetCurrentExeName().empty(), "CSystem GetCurrentExeName");
    const std::string executable = CSystem::GetCurrentExeFullName();
    // executable 已是操作系统文件名，POSIX 允许其中出现反斜杠。
    check(!executable.empty() && executable.substr(0, executable.rfind('.')) == CSystem::GetCurrentExeName() &&
        CSystem::DirOrFileExist(CSystem::joinPath(CSystem::GetCurrentExePath(), executable)), "executable directory and names");
    check(!CSystem::GetCurrentDllPath().empty() && CSystem::DirOrFileExist(CSystem::GetCurrentDllPath()),
        "module directory exists");
    check(CSystem::GetSystemBits() == 32 || CSystem::GetSystemBits() == 64, "CSystem GetSystemBits");
    check(!CSystem::uuid(1).empty() && !CSystem::uuid(0).empty(), "CSystem uuid");
#endif
    check(!CSystem::timetToStr(0).empty(), "CSystem timetToStr local");
    check(!CSystem::timetToStr(0, false).empty(), "CSystem timetToStr utc");
#if CSYSTEM_HAS_NATIVE_API
    std::string emptyEnvironmentName = "CSYSTEM_EMPTY_ENV_" + CSystem::uuid(0);
    check(CSystem::GetEnvironment(nullptr).empty(), "CSystem GetEnvironment null");
    check(CSystem::GetEnvironment(emptyEnvironmentName.c_str()).empty(), "CSystem GetEnvironment missing");
#endif
    int32_t** dyadicArray = CSystem::CreateDyadicArray<int32_t>(2, 3);
    check(dyadicArray != nullptr, "CSystem CreateDyadicArray normal");
    if (dyadicArray != nullptr)
    {
        dyadicArray[1][2] = 7;
        check(dyadicArray[1][2] == 7, "CSystem CreateDyadicArray write");
        CSystem::DestroyDyadicArray(dyadicArray, 2);
    }
    check(CSystem::CreateDyadicArray<int32_t>(0, 3) == nullptr, "CSystem CreateDyadicArray invalid");

#if CSYSTEM_HAS_NATIVE_API
    std::chrono::high_resolution_clock::time_point beginTime = CSystem::GetHighTickCount();
    CSystem::Sleep(1);
    check(CSystem::GetHighTickCountMilliRunTime(beginTime) >= 0, "CSystem Sleep and high tick");

#ifdef _WIN32
    std::string separator = "\\";
#else
    std::string separator = "/";
#endif
    const char* scratch = std::getenv("CSYSTEM_TEST_DIR");
    if (scratch == nullptr)
    {
        scratch = std::getenv("TMPDIR");
    }
    const std::string parent = CSystem::canonicalPath(scratch == nullptr ? "." : scratch);
    const std::string id = CSystem::uuid(0);
    if (parent.empty() || id.empty())
    {
        return 1;
    }
    const std::string root = CSystem::joinPath(parent, "CSystemTest_" + id);
    std::string filePath = root + separator + "data.txt";
    std::string emptyFilePath = root + separator + "empty.txt";
    std::string renamePath = root + separator + "rename.txt";
    const bool created = CSystem::CreateDir(root);
    check(created, "CSystem CreateDir");
    if (!created)
    {
        return 1;
    }
    check(CSystem::DirOrFileExist(root), "CSystem DirOrFileExist dir");
    const std::string utf8Parent = CSystem::joinPath(root, "nested-dirs");
    const std::string utf8Leaf = CSystem::joinPath(utf8Parent, "leaf");
    check(CSystem::createDirectoriesUtf8(TestPathUtf8(utf8Leaf)) && CSystem::DirOrFileExist(utf8Leaf) &&
        CSystem::createDirectoriesUtf8(TestPathUtf8(utf8Leaf)), "UTF-8 nested directories exist and can be requested twice");
    check(CSystem::DestroyDir(utf8Leaf) && CSystem::DestroyDir(utf8Parent), "remove UTF-8 directory fixture");
    check(CheckCommonProgram(root), "current environment and versioned program with both root spellings");
    const std::string invalidRoot = root + std::string("\0tail", 5);
    check(!CSystem::DirOrFileExist(invalidRoot) && !CSystem::DestroyDir(invalidRoot) &&
        CSystem::DirOrFileExist(root), "embedded NUL never observes or removes a different directory");
    const std::string absent = CSystem::joinPath(root, "unexpected");
    check(!CSystem::CreateDir(absent + std::string("\0tail", 5)) && !CSystem::DirOrFileExist(absent),
        "embedded NUL never creates a prefix directory");
    check(CSystem::canonicalPath("").empty(), "CSystem canonicalPath empty");
    check(CSystem::canonicalPath(std::string("a\0b", 3), false).empty(), "CSystem canonicalPath embedded null");
    check(!CSystem::canonicalPath(root).empty(), "CSystem canonicalPath existing");
    check(CSystem::canonicalPath(root, false) == CSystem::canonicalPath(root), "CSystem canonicalPath lexical and existing");
    check(CSystem::canonicalPath(CSystem::joinPath(root, "missing")).empty(), "CSystem canonicalPath missing");
    check(CSystem::isPathWithin(CSystem::joinPath(root, "child"), root, false), "CSystem isPathWithin child");
    check(!CSystem::isPathWithin(root, root, false), "CSystem isPathWithin excludes root");
    check(!CSystem::isPathWithin(root + "_sibling", root, false), "CSystem isPathWithin excludes shared prefix");
    const std::string aliasSource = CSystem::joinPath(root, "alias-source");
    const std::string aliasTarget = CSystem::joinPath(root, "alias-target");
    check(CSystem::CreateDir(aliasSource), "create migration alias fixture");
    const std::string aliasFile = CSystem::joinPath(aliasSource, "file");
    const std::string movedFile = CSystem::joinPath(aliasTarget, "file");
    CSystem::saveFile("alias bytes", aliasFile);
    std::string pathAndMessage = aliasSource;
    check(CSystem::migrateDirectory(pathAndMessage, aliasTarget, pathAndMessage) && pathAndMessage.empty() &&
        CSystem::readFile(movedFile) == "alias bytes" && !CSystem::DirOrFileExist(aliasSource),
        "migration error output may share the source string");
    pathAndMessage = aliasSource;
    check(CSystem::migrateDirectory(aliasTarget, pathAndMessage, pathAndMessage) && pathAndMessage.empty() &&
        CSystem::readFile(aliasFile) == "alias bytes" && !CSystem::DirOrFileExist(aliasTarget),
        "migration error output may share the destination string");
    CSystem::deleteFile(aliasFile.c_str());
    CSystem::deleteFile(movedFile.c_str());
    CSystem::DestroyDir(aliasTarget);
    CSystem::DestroyDir(aliasSource);
    std::string migrationParent = CSystem::joinPath(root, "migration source");
    std::string migrationSource = CSystem::joinPath(migrationParent, "nested");
    std::string migrationTarget = CSystem::joinPath(root, "migration target");
    std::string migrationError;
    check(CSystem::createDirectories(migrationSource), "CSystem createDirectories recursive");
    check(CSystem::createDirectories(migrationSource), "CSystem createDirectories existing");
    check(CSystem::createDirectories(migrationTarget), "CSystem createDirectories target");
    CSystem::saveFile("migration content", CSystem::joinPath(migrationSource, "data.bin"));
    CSystem::saveFile("", CSystem::joinPath(migrationSource, "empty.bin"));
    CSystem::saveFile("migration content", CSystem::joinPath(migrationTarget, "data.bin"));
    const std::string sourceData = CSystem::joinPath(migrationSource, "data.bin");
    const std::string sourceEmpty = CSystem::joinPath(migrationSource, "empty.bin");
#ifndef _WIN32
    const struct timeval times[] = {{1500000000, 123456}, {1500000020, 765432}};
    check(::utimes(sourceData.c_str(), times) == 0 && ::utimes(sourceEmpty.c_str(), times) == 0,
        "set independent subsecond modification times");
#endif
    uint64_t length = 0, dataStamp = 0, emptyStamp = 0, observed = 0;
    check(CSystem::fileStampUtf8(TestPathUtf8(sourceData), &length, &dataStamp) && length == 17 &&
        CSystem::fileStampUtf8(TestPathUtf8(sourceEmpty), &length, &emptyStamp) && length == 0, "record source modification times");
    check(!CSystem::migrateDirectory(migrationSource, CSystem::joinPath(migrationSource, "child"), migrationError), "CSystem migrateDirectory rejects overlap");
    check(!CSystem::DirOrFileExist(CSystem::joinPath(migrationSource, "child")) && CSystem::readFile(CSystem::joinPath(migrationSource, "data.bin")) == "migration content", "CSystem migrateDirectory overlap has no side effect");
    check(CSystem::migrateDirectory(migrationSource, migrationTarget, migrationError), "CSystem migrateDirectory merges matching content");
    check(!CSystem::DirOrFileExist(migrationSource) && CSystem::readFile(CSystem::joinPath(migrationTarget, "data.bin")) == "migration content", "CSystem migrateDirectory verifies bytes before source removal");
    check(CSystem::DirOrFileExist(CSystem::joinPath(migrationTarget, "empty.bin")), "CSystem migrateDirectory preserves empty file");
    check(CSystem::fileStampUtf8(TestPathUtf8(CSystem::joinPath(migrationTarget, "data.bin")), &length, &observed) &&
        observed == dataStamp && CSystem::fileStampUtf8(TestPathUtf8(CSystem::joinPath(migrationTarget, "empty.bin")), &length, &observed) &&
        observed == emptyStamp, "migration preserves modification time on merge and copy");
    check(!CSystem::createDirectories(CSystem::joinPath(migrationTarget, "data.bin")), "CSystem createDirectories rejects file");
    check(CSystem::createDirectories(migrationSource), "CSystem migration conflict source");
    CSystem::saveFile("different content", CSystem::joinPath(migrationSource, "data.bin"));
    check(!CSystem::migrateDirectory(migrationSource, migrationTarget, migrationError) && !migrationError.empty(), "CSystem migrateDirectory rejects conflict");
    check(CSystem::readFile(CSystem::joinPath(migrationSource, "data.bin")) == "different content" && CSystem::readFile(CSystem::joinPath(migrationTarget, "data.bin")) == "migration content", "CSystem migrateDirectory conflict preserves both files");
    check(CSystem::deleteFile(CSystem::joinPath(migrationSource, "data.bin").c_str()) && CSystem::DestroyDir(migrationSource), "CSystem migration source cleanup");
    check(CSystem::migrateDirectory(migrationSource, migrationTarget, migrationError), "CSystem migrateDirectory missing source is complete");
    check(CSystem::deleteFile(CSystem::joinPath(migrationTarget, "data.bin").c_str()) && CSystem::deleteFile(CSystem::joinPath(migrationTarget, "empty.bin").c_str()), "CSystem migration target file cleanup");
    check(CSystem::DestroyDir(migrationTarget) && CSystem::DestroyDir(migrationParent), "CSystem migration directory cleanup");
#ifndef _WIN32
    const std::string real = root + "/original";
    const std::string alias = root + "/alias";
    check(CSystem::CreateDir(real) && ::symlink(real.c_str(), alias.c_str()) == 0, "create migration alias");
    check(!CSystem::migrateDirectory(real, alias + "/nested/target", migrationError) &&
        !CSystem::DirOrFileExist(real + "/nested"), "physical overlap rejected before creating children");
    check(CSystem::DestroyDir(real) && !CSystem::migrateDirectory(alias, real, migrationError) &&
        ::unlink(alias.c_str()) == 0, "dangling source is rejected");
    // POSIX 允许目录名以反斜杠结尾，以显式斜杠建立独立的预期子路径。
    const std::string slashSource = root + "/source\\";
    const std::string slashTarget = root + "/target\\";
    check(CSystem::createDirectories(slashSource), "create POSIX source ending in backslash");
    CSystem::saveFile("nested bytes", slashSource + "/item");
    check(CSystem::migrateDirectory(slashSource, slashTarget, migrationError) &&
        CSystem::readFile(slashTarget + "/item") == "nested bytes" && !CSystem::DirOrFileExist(slashSource),
        "migration preserves POSIX directory names ending in backslash");
    CSystem::deleteFile((slashSource + "/item").c_str());
    CSystem::deleteFile((slashTarget + "/item").c_str());
    CSystem::DestroyDir(slashSource);
    CSystem::DestroyDir(slashTarget);
#endif
#ifdef _WIN32
    check(!CSystem::ShellCopy("", filePath.c_str()) && !CSystem::ShellCopy(filePath.c_str(), ""),
        "ShellCopy rejects empty source and destination");
#endif
    CSystem::saveFile("hello system", filePath);
    check(CSystem::readFile(filePath) == "hello system", "CSystem saveFile readFile");
#ifdef _WIN32
    const std::string shellTarget = CSystem::joinPath(root, "shell-copy.txt");
    check(CSystem::ShellCopy(filePath.c_str(), shellTarget.c_str()) && CSystem::readFile(shellTarget) == "hello system",
        "ShellCopy copies the original bytes");
    check(CSystem::deleteFile(shellTarget.c_str()), "remove ShellCopy target");
#endif
    const std::string rejectedPath = emptyFilePath + std::string("\0suffix", 7);
    CSystem::CopyFileOver(rejectedPath, filePath, true);
    CSystem::OutputVector(std::vector<std::string>(1, "entry"), rejectedPath);
    CSystem::OutputMap(std::map<std::string, std::string>(), rejectedPath);
    check(!CSystem::DirOrFileExist(emptyFilePath), "invalid target must not create the prefix");
    CSystem::CopyFileOver(emptyFilePath, filePath + std::string("\0suffix", 7), true);
    check(!CSystem::DirOrFileExist(emptyFilePath), "invalid source must not copy the prefix");
    CSystem::CopyFileOver(filePath, filePath, true);
    check(CSystem::readFile(filePath) == "hello system", "copy onto the source preserves its contents");
    CSystem::saveFile("preserved", emptyFilePath);
    CSystem::CopyFileOver(emptyFilePath, filePath, false);
    check(CSystem::readFile(emptyFilePath) == "preserved", "copy without overwrite preserves existing bytes");
    CSystem::CopyFileOver(emptyFilePath, filePath, true);
    check(CSystem::readFile(emptyFilePath) == "hello system", "copy with overwrite replaces existing bytes");
    CSystem::saveFile("", emptyFilePath);
    check(CSystem::DirOrFileExist(emptyFilePath) && CSystem::readFile(emptyFilePath).empty(), "CSystem empty readFile");
    CSystem::saveFile("previous destination", renamePath);
#ifdef _WIN32
    check(!CSystem::rename(filePath, renamePath) && CSystem::readFile(filePath) == "hello system" &&
        CSystem::readFile(renamePath) == "previous destination", "Windows rename preserves both existing files");
    check(CSystem::deleteFile(renamePath.c_str()), "remove existing rename destination");
#else
    check(CSystem::rename(filePath, renamePath) && !CSystem::DirOrFileExist(filePath) &&
        CSystem::readFile(renamePath) == "hello system", "POSIX rename replaces the existing file");
    check(CSystem::rename(renamePath, filePath), "restore source before ordinary rename");
#endif
    check(CSystem::rename(filePath, renamePath), "CSystem rename file");
    check(CSystem::readFile(renamePath) == "hello system", "CSystem read renamed file");
    std::vector<std::string> findFiles = CSystem::findFilePath(root, 1, "rename.txt");
    check(findFiles.size() == 1 && CSystem::canonicalPath(findFiles.front()) == CSystem::canonicalPath(renamePath),
        "CSystem findFilePath exact result");
    // 从输出列表中借用两个输入，与调用前已知的实际文件比较。
    std::vector<std::string> retryPaths;
    retryPaths.push_back(root);
    retryPaths.push_back("rename.txt");
    const auto retried = CSystem::findFilePath(retryPaths[0], 1, retryPaths[1], nullptr, &retryPaths);
    check(retryPaths.empty() && retried.size() == 1 &&
        CSystem::canonicalPath(retried[0]) == CSystem::canonicalPath(renamePath),
        "reuse prior unvisited entries as root and exact filename");
    const std::string missingDirectory = CSystem::joinPath(root, "missing-retry");
    CSystem::findFilePath(missingDirectory, 3, "", nullptr, &retryPaths);
    const bool recorded = retryPaths.size() == 1;
    check(recorded, "record unavailable enumeration root");
    if (recorded)
    {
        check(CSystem::CreateDir(missingDirectory), "prepare previously unavailable directory");
        const std::string entry = CSystem::joinPath(missingDirectory, "retry.txt");
        CSystem::saveFile("retry", entry);
        const auto found = CSystem::findFilePath(retryPaths.front(), 2, "txt", nullptr, &retryPaths);
        check(retryPaths.empty() && found.size() == 1 &&
            CSystem::canonicalPath(found[0]) == CSystem::canonicalPath(entry),
            "retry a previously unavailable root and replace the diagnostics");
        CSystem::deleteFile(entry.c_str());
        CSystem::DestroyDir(missingDirectory);
    }
    const std::string nested = CSystem::joinPath(root, "nested");
    const std::string concurrentDir = CSystem::joinPath(root, "mkdir-race");
    const bool createdTogether = RunSystemWorkers(4, [&concurrentDir](size_t) -> bool {
        return CSystem::createDirectories(concurrentDir);
    });
    check(createdTogether && CSystem::DestroyDir(concurrentDir), "all concurrent directory creators succeed");
#ifndef _WIN32
#if CSYSTEM_HAS_DESKTOP_API
    const std::string fifo = root + "/ordinary-file-check.fifo";
    check(::mkfifo(fifo.c_str(), 0600) == 0, "create private FIFO fixture");
    uint32_t openError = 0;
    intptr_t fifoHandle = CSystem::openFileHandle(fifo.c_str(), 1, &openError);
    const bool rejectedFifo = fifoHandle == -1 && openError != 0;
    CSystem::closeFileHandle(&fifoHandle);
    check(rejectedFifo && ::unlink(fifo.c_str()) == 0, "regular-file API rejects a FIFO without a writer");
#endif
    const std::string pending = root + "/draft";
    const std::string published = root + "/final\\name";
    CSystem::saveFile("published", pending);
    check(CSystem::publishFileUtf8(pending.c_str(), published.c_str()) && CSystem::readFile(published) == "published" &&
        !CSystem::DirOrFileExist(pending), "publish fsyncs the POSIX parent of a literal backslash");
    CSystem::deleteFile(pending.c_str());
    CSystem::deleteFile(published.c_str());
    const std::string literal = CSystem::joinPath(root, "part\\leaf");
    check(CSystem::createDirectories(literal) && CSystem::DestroyDir(literal),
        "POSIX directory component preserves literal backslash");
#endif
    const std::string firstChild = CSystem::joinPath(nested, "first");
    const std::string secondChild = CSystem::joinPath(nested, "second");
    const std::string child = CSystem::joinPath(firstChild, "first.TXT");
    const std::string sibling = CSystem::joinPath(secondChild, "second.txt");
    check(CSystem::createDirectories(firstChild) && CSystem::createDirectories(secondChild), "create enumeration children");
    CSystem::saveFile("child", child);
    CSystem::saveFile("sibling", sibling);
    size_t visits = 0;
    const auto selected = CSystem::findFilePath(nested, 2, "txt", [&visits](const std::string&) -> bool {
        ++visits;
        return true;
    });
    check(selected.size() == 1 && visits == 1, "callback stops entire search with children queued");
    check(CSystem::findFilePath(root, 2, "TxT").size() == 4 && CSystem::findFilePath(invalidRoot, 3).empty(),
        "extension filter is case insensitive and rejects invalid roots");
    const std::string noSuffix = CSystem::joinPath(firstChild, "README");
    CSystem::saveFile("plain", noSuffix);
    const auto plain = CSystem::findFilePath(nested, 2, "");
    const auto combined = CSystem::findFilePath(nested, 2, "txt.");
    check(plain.size() == 1 && CSystem::canonicalPath(plain.front()) == CSystem::canonicalPath(noSuffix) &&
        combined.size() == 3, "extension list preserves the empty field");
#ifndef _WIN32
    const std::string literalSuffix = firstChild + "/note.ab\\cd";
    CSystem::saveFile("literal", literalSuffix);
    const auto literalMatches = CSystem::findFilePath(nested, 2, "AB\\CD");
    check(literalMatches.size() == 1 && literalMatches.front() == literalSuffix &&
        CSystem::findFilePath(nested, 2, "").size() == 1, "POSIX backslash is part of the extension");
    check(CSystem::deleteFile(literalSuffix.c_str()), "remove literal-extension fixture");
#endif
    check(CSystem::deleteFile(noSuffix.c_str()), "remove extensionless fixture");
    bool propagated = false;
    try
    {
        CSystem::findFilePath(nested, 3, "", [](const std::string&) -> bool { throw 17; });
    }
    catch (int error) { propagated = error == 17; }
    check(propagated && CSystem::deleteFile(child.c_str()) && CSystem::deleteFile(sibling.c_str()) &&
        CSystem::DestroyDir(firstChild) && CSystem::DestroyDir(secondChild) && CSystem::DestroyDir(nested),
        "callback exception leaves no open search handle");
    check(CSystem::deleteFile(renamePath.c_str()), "CSystem deleteFile");
    check(CSystem::deleteFile(emptyFilePath.c_str()), "CSystem delete empty file");
    check(!CSystem::DirOrFileExist(renamePath), "CSystem deleted file missing");
    check(CSystem::DestroyDir(root), "CSystem DestroyDir");

#endif
    std::cout << "CSystem interface regressions " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << std::endl;
    return failCount == 0 ? 0 : 1;
}

#if CSYSTEM_HAS_NATIVE_API
bool RunSystemWorkers(size_t count, const std::function<bool(size_t)>& work)
{
    std::atomic<bool> start(false);
    std::atomic<bool> success(count != 0);
    std::vector<std::thread> threads;
    try
    {
        threads.reserve(count);
        for (size_t index = 0; index < count; ++index)
        {
            threads.emplace_back([&, index]() {
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                try
                {
                    if (!work(index))
                    {
                        success.store(false);
                    }
                }
                catch (...)
                {
                    std::fprintf(stderr, "CSystemTest worker %u failed with an exception\n", static_cast<unsigned>(index));
                    success.store(false);
                }
            });
        }
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "CSystemTest could not start workers: %s\n", error.what());
        success.store(false);
    }
    start.store(true);
    for (auto& thread : threads)
    {
        thread.join();
    }
    return success.load();
}

/** 同时保留八个工作线程，验证线程号稳定且各不相同，并检查每次生成的 UUID
@return 线程号和全部 UUID 均满足预期时返回 0
*/
int RunSystemConcurrentTests()
{
    const size_t workerCount = 8;
    const size_t iterations = 2000;
    std::vector<uint32_t> ids(workerCount);
    std::vector<std::string> values(workerCount * iterations);
    std::atomic<size_t> finished(0);
    const bool acceptedFailure = RunSystemWorkers(2, [&finished](size_t index) -> bool {
        ++finished;
        return index != 0;
    });
    if (acceptedFailure || finished.load() != 2)
    {
        return 1;
    }
    const bool stable = RunSystemWorkers(workerCount, [&](size_t index) -> bool {
        ids[index] = CSystem::SystemThreadId();
        for (size_t i = 0; i < iterations; ++i)
        {
            if (CSystem::SystemThreadId() != ids[index])
            {
                return false;
            }
            values[index * iterations + i] = CSystem::uuid(0);
        }
        return true;
    });
    std::sort(ids.begin(), ids.end());
    std::sort(values.begin(), values.end());
    return stable && ids.front() != 0 && std::adjacent_find(ids.begin(), ids.end()) == ids.end() &&
        !values.front().empty() && std::adjacent_find(values.begin(), values.end()) == values.end() ? 0 : 1;
}

/** 核对随机源、回调返回值和调用者缓冲两端的保护字节
@return 每个边界断言通过时返回 0
*/
int RunSystemEntropyTests()
{
    uint32_t assertions = 0;
#define ENTROPY_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    ENTROPY_CHECK(CSystem::secureRandomBytes(nullptr, 0));
    ENTROPY_CHECK(!CSystem::secureRandomBytes(nullptr, 1));
    ENTROPY_CHECK(CSystem::secureRandomBytesCallback(nullptr, nullptr, 0) == 0);
    ENTROPY_CHECK(CSystem::secureRandomBytesCallback(nullptr, nullptr, 1) == -1);
    uint8_t bytes[8194];
    std::memset(bytes, 0xa5, sizeof(bytes));
    ENTROPY_CHECK(!CSystem::secureRandomBytes(bytes + 1, 8193));
    for (size_t index = 0; index < sizeof(bytes); ++index)
    {
        ENTROPY_CHECK(bytes[index] == 0xa5);
    }
    const uint32_t sizes[] = {0, 1, 32, 256, 8192};
    for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
    {
        std::memset(bytes, 0xa5, sizeof(bytes));
        ENTROPY_CHECK(CSystem::secureRandomBytes(bytes + 1, sizes[index]));
        ENTROPY_CHECK(bytes[0] == 0xa5 && bytes[sizes[index] + 1] == 0xa5);
    }
    uint8_t other[8192];
    ENTROPY_CHECK(CSystem::secureRandomBytesCallback(nullptr, other, 32) == 0);
    ENTROPY_CHECK(CSystem::secureRandomBytes(other, sizeof(other)));
    ENTROPY_CHECK(std::memcmp(other, bytes + 1, sizeof(other)) != 0);
    std::printf("CSystem entropy PASS %u assertions\n", assertions);
    return 0;
#undef ENTROPY_CHECK
}
#endif

#if CSYSTEM_HAS_DESKTOP_API
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
#include <crtdbg.h>
// 串行租约故障用例中，距首次拒绝还余多少次普通堆分配。
static uint32_t s_leaseRemaining = 0;
// 当前故障位置拒绝的分配次数。
static uint32_t s_leaseRejected = 0;
/** 从指定普通堆分配位置开始持续拒绝，其他参数沿用CRT原型
@param [in] operation CRT 分配操作
@param [in] block CRT 内存块类型，其余未命名参数按回调原型忽略
@return 是否允许
*/
static int __cdecl LeaseMemoryHook(int operation, void*, size_t, int block, long, const unsigned char*, int)
{
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        if (s_leaseRemaining == 0)
        {
            ++s_leaseRejected;
            return 0;
        }
        --s_leaseRemaining;
    }
    return 1;
}

/** 逐个拒绝结果构造时的普通堆分配，并检查每次调用前后的快照句柄数
@param [in] query 已预热的同步查询，返回结果在调用期间销毁
@param [in] positions 从首次普通分配起覆盖的位置数
@return 观察到 bad_alloc 的位置数，句柄泄漏或异常类型不符时返回 -1
*/
static int32_t CheckSnapshotAllocations(const std::function<void()>& query, uint32_t positions)
{
    int32_t failures = 0;
    query();
    for (uint32_t position = 0; position < positions; ++position)
    {
        DWORD before = 0, after = 0;
        if (!::GetProcessHandleCount(::GetCurrentProcess(), &before))
        {
            return -1;
        }
        s_leaseRemaining = position;
        s_leaseRejected = 0;
        const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(LeaseMemoryHook);
        bool rejected = false;
        bool unexpected = false;
        try
        {
            query();
        }
        catch (const std::bad_alloc&)
        {
            rejected = true;
        }
        catch (...)
        {
            unexpected = true;
        }
        _CrtSetAllocHook(previous);
        if (unexpected || rejected != (s_leaseRejected != 0) ||
            !::GetProcessHandleCount(::GetCurrentProcess(), &after) || before != after)
        {
            std::fprintf(stderr, "snapshot allocation %u: rejected=%u, handles %lu -> %lu\n",
                position, s_leaseRejected, before, after);
            return -1;
        }
        if (rejected)
        {
            ++failures;
        }
    }
    return failures;
}
#endif

/** 使用测试专属名称验证租约取得失败回滚、就绪及释放，不操作正式网关租约
@return 所有检查通过为true
*/
bool SystemLeaseAudit()
{
    uint32_t checks = 0;
    uint32_t failures = 0;
    uint32_t successes = 0;
#define LEASE_CHECK(value) { ++checks; if (!(value)) { std::printf("System lease line=%d FAIL\n", __LINE__); return false; } }
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
    // CRT首次处理分配异常会建立自身常驻资源；先单独触发，不把它计为租约泄漏。
    std::string warm;
    const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(LeaseMemoryHook);
    try { warm.resize(4096); } catch (...) {}
    _CrtSetAllocHook(previous);
    const DWORD self = ::GetCurrentProcessId();
    const std::string name = CSystem::processName(self);
    const std::wstring wideName = CSystem::processNameW(self);
    LEASE_CHECK(!name.empty() && !wideName.empty());
    LEASE_CHECK(!CSystem::processMountDll(self).empty());
    LEASE_CHECK(CheckSnapshotAllocations([&]() { CSystem::processMountDll(self); }, 6) == 6);
    LEASE_CHECK(CheckSnapshotAllocations([&]() { CSystem::processPid(name); }, 6) > 0);
    LEASE_CHECK(CheckSnapshotAllocations([&]() { CSystem::processPid(wideName); }, 6) > 0);
    LEASE_CHECK(CheckSnapshotAllocations([&]() { CSystem::processFirstPid(name); }, 2) >= 0);
    LEASE_CHECK(CheckSnapshotAllocations([&]() { CSystem::processFirstPid(wideName); }, 2) >= 0);
    LEASE_CHECK(CheckSnapshotAllocations([&]() { CSystem::processName(self); }, 2) >= 0);
    LEASE_CHECK(CheckSnapshotAllocations([&]() { CSystem::processNameW(self); }, 2) >= 0);
#endif
#ifdef _WIN32
    // 本进程的编号已独占命名空间，快照前避免初始化 RPC 的 UUID 后台工作。
    const std::string prefix = "CSystemTestLease." + std::to_string(CSystem::currentProcessPid());
#else
    const std::string prefix = "CSystemTestLease." + CSystem::uuid(0);
    const std::string role = prefix + ".fifo";
    const uint32_t pid = CSystem::currentProcessPid();
    const std::string marker = "/tmp/xyls." + role + ".ready." + std::to_string(pid);
    LEASE_CHECK(::mkfifo(marker.c_str(), 0600) == 0);
    const bool ready = CSystem::isProcessLeaseReady(role, pid);
    const bool removed = ::unlink(marker.c_str()) == 0;
    LEASE_CHECK(!ready && removed);
#endif
    for (uint32_t position = 0; position < 32; ++position)
    {
        const std::string name = prefix + "." + std::to_string(position);
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
        _CrtMemState before = {};
        _CrtMemCheckpoint(&before);
        DWORD handlesBefore = 0;
        LEASE_CHECK(GetProcessHandleCount(GetCurrentProcess(), &handlesBefore) != FALSE);
#endif
        bool escaped = false;
        bool valid = true;
        {
            std::vector<uint64_t> state;
            int32_t acquired = -99;
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
            s_leaseRemaining = position;
            s_leaseRejected = 0;
            const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(LeaseMemoryHook);
#endif
            try
            {
                acquired = CSystem::acquireProcessLease(name, false, 0, state);
            }
            catch (...)
            {
                escaped = true;
            }
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
            _CrtSetAllocHook(old);
#endif
            if (acquired == 1)
            {
                ++successes;
                valid = state.size() == 3 && !CSystem::processLeaseStopRequested(state) &&
                    CSystem::setProcessLeaseReady(state, true) && CSystem::isProcessLeaseReady(name, CSystem::currentProcessPid());
                // 重复使用同一个输出容器是非法输入，必须保留已取得租约，不改变ready。
                valid = valid && CSystem::acquireProcessLease(name, false, 0, state) == -1 &&
                    state.size() == 3 && CSystem::isProcessLeaseReady(name, CSystem::currentProcessPid());
                valid = valid && CSystem::setProcessLeaseReady(state, false) &&
                    !CSystem::isProcessLeaseReady(name, CSystem::currentProcessPid()) &&
                    CSystem::setProcessLeaseReady(state, true);
            }
            else
            {
                ++failures;
                valid = acquired == -1 && state.empty() && !CSystem::isProcessLeaseReady(name, CSystem::currentProcessPid());
            }
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
            s_leaseRemaining = 0;
            s_leaseRejected = 0;
            const _CRT_ALLOC_HOOK cleanupHook = _CrtSetAllocHook(LeaseMemoryHook);
#endif
            CSystem::releaseProcessLease(state);
            CSystem::releaseProcessLease(state);
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
            _CrtSetAllocHook(cleanupHook);
            valid = valid && s_leaseRejected == 0;
#endif
            valid = valid && state.empty() && !CSystem::isProcessLeaseReady(name, CSystem::currentProcessPid());
            valid = valid && CSystem::acquireProcessLease(name, false, 0, state) == 1;
            CSystem::releaseProcessLease(state);
        }
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
        DWORD handlesAfter = 0;
        LEASE_CHECK(GetProcessHandleCount(GetCurrentProcess(), &handlesAfter) != FALSE);
        if (handlesBefore != handlesAfter)
        {
            std::fprintf(stderr, "lease position=%u process handles before=%lu after=%lu\n", position, handlesBefore, handlesAfter);
        }
        LEASE_CHECK(handlesBefore == handlesAfter);
        _CrtMemState after = {};
        _CrtMemCheckpoint(&after);
        LEASE_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
#endif
#ifndef _WIN32
        // POSIX 保持锁文件 inode 稳定；测试名独占且句柄已释放，此时清理这两个已知路径。
        valid = CSystem::deleteFile(("/tmp/xyls." + name + ".lease").c_str()) && valid;
        valid = CSystem::deleteFile(("/tmp/xyls." + name + ".ready." + std::to_string(CSystem::currentProcessPid())).c_str()) && valid;
#endif
        LEASE_CHECK(!escaped && valid);
    }
#if defined(_WIN32) && defined(_DEBUG) && !defined(SYSTEM_LEASE_NO_HEAP)
    LEASE_CHECK(failures != 0 && successes != 0);
#else
    LEASE_CHECK(failures == 0 && successes == 32);
#endif
    // 独立验证竞争，线程启动和 CRT 线程状态的延迟清理不进入故障注入的资源差值。
    const std::string contest = prefix + ".contest";
    std::vector<uint64_t> owner;
    LEASE_CHECK(CSystem::acquireProcessLease(contest, false, 0, owner) == 1);
    const bool competing = RunSystemWorkers(1, [&](size_t) -> bool {
        std::vector<uint64_t> another;
        const int32_t result = CSystem::acquireProcessLease(contest, false, 0, another);
        CSystem::releaseProcessLease(another);
        return result == 0;
    });
    CSystem::releaseProcessLease(owner);
#ifndef _WIN32
    LEASE_CHECK(CSystem::deleteFile(("/tmp/xyls." + contest + ".lease").c_str()));
    LEASE_CHECK(CSystem::deleteFile(("/tmp/xyls." + contest + ".ready." + std::to_string(CSystem::currentProcessPid())).c_str()));
#endif
    LEASE_CHECK(competing && owner.empty());
    std::printf("System lease positions=32 failed=%u acquired=%u checks=%u PASS\n", failures, successes, checks);
    return true;
#undef LEASE_CHECK

}

#endif