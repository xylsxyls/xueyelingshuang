#include "LogManagerTest.h"
#include "PlatformTest/PlatformTestAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <initializer_list>
#include <map>
#include <set>
#include <vector>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifndef __EMSCRIPTEN__
#include <thread>
#else
#include <emscripten.h>
#endif
#include <cstdio>
#include <cstring>

#define CHECK(test) do { if (!(test)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; } } while (false)

/** 枚举独占测试目录下的文件名
@param [in] dir 已创建的目录
@param [in] directories 为真时只返回子目录，始终不跟随链接
@param [out] complete 可选，完整遍历现有目录时为真
@return 匹配的本层名称
*/
static std::vector<std::string> Files(const std::string& dir, bool directories = false, bool* complete = nullptr)
{
    std::vector<std::string> result;
    bool ok = false;
#ifdef _WIN32
    WIN32_FIND_DATAA info = {};
    HANDLE search = FindFirstFileA((dir + "/*").c_str(), &info);
    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            const bool directory = (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT);
            if (directory == directories && std::strcmp(info.cFileName, ".") && std::strcmp(info.cFileName, ".."))
            {
                result.push_back(info.cFileName);
            }
        } while (FindNextFileA(search, &info));
        ok = GetLastError() == ERROR_NO_MORE_FILES;
        FindClose(search);
    }
    else if (GetLastError() == ERROR_FILE_NOT_FOUND)
    {
        const DWORD attributes = GetFileAttributesA(dir.c_str());
        ok = attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }
#else
    DIR* search = opendir(dir.c_str());
    if (search)
    {
        ok = true;
        for (;;)
        {
            errno = 0;
            dirent* entry = readdir(search);
            if (!entry)
            {
                ok = ok && errno == 0;
                break;
            }
            if (std::strcmp(entry->d_name, ".") && std::strcmp(entry->d_name, ".."))
            {
                struct stat info;
                if (lstat((dir + "/" + entry->d_name).c_str(), &info) != 0)
                {
                    ok = false;
                }
                else if ((S_ISDIR(info.st_mode) != 0) == directories)
                {
                    result.push_back(entry->d_name);
                }
            }
        }
        closedir(search);
    }
#endif
    if (complete)
    {
        *complete = ok;
    }
    return result;
}

/** 验证目录可遍历且没有指定类型的条目
@param [in] path 已创建的目录
@param [in] directories 为真时检查子目录，为假时检查文件及链接
@return 完成遍历且没有匹配项时返回 true
*/
static bool NoEntries(const std::string& path, bool directories = false)
{
    bool complete = false;
    return Files(path, directories, &complete).empty() && complete;
}

/** 读取命名为 名称.卷号.log 的实体日志
@param [in] dir 已关闭写流的目录
@param [out] volumes 读取到的实体卷数
@return 各卷的原始文本
*/
static std::string ReadVolumes(const std::string& dir, size_t& volumes)
{
    volumes = 0;
    std::string text;
    bool complete = false;
    const std::vector<std::string> files = Files(dir, false, &complete);
    if (!complete)
    {
        return "";
    }
    for (const std::string& name : files)
    {
        const size_t dot = name.rfind('.');
        if (dot == std::string::npos || name.substr(dot) != ".log")
        {
            continue;
        }
        const size_t prior = name.rfind('.', dot - 1);
        if (prior == std::string::npos || prior + 1 == dot || name.find_first_not_of("0123456789", prior + 1) != dot)
        {
            continue;
        }
        std::ifstream input((dir + "/" + name).c_str(), std::ios::binary);
        if (!input)
        {
            volumes = 0;
            return "";
        }
        text.append(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        ++volumes;
    }
    return text;
}

LogSession::LogSession()
{
#ifdef _WIN32
    char base[MAX_PATH] = {};
    DWORD length = GetTempPathA(MAX_PATH, base);
    if (!length || length >= MAX_PATH)
    {
        return;
    }
    static std::atomic<uint32_t> s_sequence(0);
    std::string dir;
    do
    {
        dir = std::string(base) + "LogManagerTest-" + std::to_string(GetCurrentProcessId()) + "-" +
            std::to_string(s_sequence.fetch_add(1));
        if (CreateDirectoryA(dir.c_str(), nullptr))
        {
            break;
        }
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            return;
        }
    } while (true);
#else
    const char* base = std::getenv("TMPDIR");
    std::string dir = std::string(base && *base ? base : "/tmp") + "/LogManagerTest-XXXXXX";
    if (!mkdtemp(&dir[0]))
    {
        return;
    }
#endif
    config.m_path = dir;
    config.m_fileId = 2741;
    config.m_archiveOldLog = false;
}

void RemoveLogTestTree(const std::string& path)
{
    for (const std::string& name : Files(path))
    {
        std::remove((path + "/" + name).c_str());
    }
    for (const std::string& name : Files(path, true))
    {
        RemoveLogTestTree(path + "/" + name);
    }
#ifdef _WIN32
    RemoveDirectoryA(path.c_str());
#else
    rmdir(path.c_str());
#endif
}

LogSession::~LogSession()
{
    if (!config.m_path.empty())
    {
        LogManager& log = LogManager::instance();
        log.set(true, true);
        log.uninit(config.m_fileId);
        log.setDefaultConfig(LogManagerConfig());
        RemoveLogTestTree(config.m_path);
    }
}

#ifdef _WIN32
/** 从实际文件或链接读回末尾的消息
@param [in] path 待核验的路径
@param [in] message 最后一条记录的正文
@return 文件末尾精确匹配该消息时返回 true
*/
static bool EndsWithMessage(const std::string& path, const std::string& message)
{
    std::ifstream file(path.c_str(), std::ios::binary);
    const std::string suffix = "] : " + message;
    file.seekg(-static_cast<std::streamoff>(suffix.size()), std::ios::end);
    std::string bytes(suffix.size(), '\0');
    return file.read(&bytes[0], static_cast<std::streamsize>(bytes.size())) && bytes == suffix;
}

int CheckBusyLogLink()
{
    for (LogTestLinkAction action : {LogTestLinkAction::RollAgain, LogTestLinkAction::Archive,
        LogTestLinkAction::Delete, LogTestLinkAction::ArchiveWhilePinned})
    {
        LogSession session;
        CHECK(!session.config.m_path.empty());
        session.config.m_archiveOldLog = true;
        session.config.m_maxFileBytes = 1;
        session.config.m_checkFileSizeInterval = 1;
        LogManager& log = LogManager::instance();
        const int32_t id = session.config.m_fileId;
        const std::string& path = session.config.m_path;
        log.set(true, false);
        log.init(session.config);
        LOGINFO_EX(id, "first");
        std::string base;
        for (const std::string& name : Files(path))
        {
            if (name.size() > 6 && name.compare(name.size() - 6, 6, ".0.log") == 0)
            {
                base = name.substr(0, name.size() - 6);
                break;
            }
        }
        const std::string alias = path + "/" + base + ".log";
        CHECK(!base.empty() && log.findFileId(alias) == id);
        HANDLE reader = CreateFileA(alias.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        CHECK(reader != INVALID_HANDLE_VALUE);
        LOGINFO_EX(id, "second");
        const bool stale = log.findFileId(alias) == -1 && EndsWithMessage(alias, "first");
        const bool committed = EndsWithMessage(path + "/" + base + ".1.log", "second");
        if (action == LogTestLinkAction::ArchiveWhilePinned)
        {
            log.uninit(id);
        }
        CloseHandle(reader);
        CHECK(stale && committed);
        if (action == LogTestLinkAction::RollAgain)
        {
            LOGINFO_EX(id, "third");
            CHECK(log.findFileId(alias) == id && EndsWithMessage(alias, "third"));
        }
        if (action == LogTestLinkAction::Delete || action == LogTestLinkAction::RollAgain)
        {
            log.deleteFile(id);
            CHECK(NoEntries(path) && NoEntries(path, true));
            continue;
        }
        if (action == LogTestLinkAction::Archive)
        {
            log.uninit(id);
        }
        const std::vector<std::string> roots = Files(path, true);
        CHECK(roots.size() == 1);
        const std::string root = path + "/" + roots.front();
        const std::vector<std::string> sessions = Files(root, true);
        CHECK(sessions.size() == 1);
        const std::string prefix = root + "/" + sessions.front() + "/" + base;
        CHECK(EndsWithMessage(prefix + ".0.log", "first") && EndsWithMessage(prefix + ".1.log", "second"));
        if (action == LogTestLinkAction::Archive)
        {
            CHECK(EndsWithMessage(prefix + ".log", "second") && NoEntries(path));
        }
        else
        {
            CHECK(GetFileAttributesA((prefix + ".log").c_str()) == INVALID_FILE_ATTRIBUTES);
        }
    }
    return 0;
}
#endif

/** 检查采样及配置默认值
@return 成功返回 0
*/
static int Sampling()
{
    const LogManagerConfig c;
    CHECK(c.m_path.empty());
    CHECK(c.m_maxFileBytes == 100LL * 1024 * 1024 && c.m_maxFileCount == 5 && c.m_checkFileSizeInterval == 100);
    CHECK(!c.m_outputConsole && c.m_archiveOldLog && c.m_fileId == 0);
    for (int n = 0; n < 40; ++n)
    {
        CHECK(LogManager::needPrintEvery(n, -3) == (n == 0));
        CHECK(LogManager::needPrintEvery(n, 0) && LogManager::needPrintEvery(n, 1));
        CHECK(LogManager::needPrintEvery(n, 7) == (n % 7 == 0));
    }
    return 0;
}

/** 写回、过滤和按 ID 查找
@return 成功返回 0
*/
static int ReadBack()
{
    LogSession s;
    CHECK(!s.config.m_path.empty());
    LogManager& log = LogManager::instance();
    log.init(s.config);
    CHECK(log.findFileId(s.config.m_path) == s.config.m_fileId);
    log.set(false, true);
    LOGINFO_EX(s.config.m_fileId, "%s", "drop-token");
    log.set(true, true);
    for (int n = 0; n < 6; ++n)
    {
        LOGWARNING_EX_EVERY(s.config.m_fileId, 2, "sample<%d>", n);
    }
    log.uninit(s.config.m_fileId);
    size_t volumes = 0;
    const std::string text = ReadVolumes(s.config.m_path, volumes);
    CHECK(volumes == 1 && log.findFileId(s.config.m_path) == -1 && text.find("drop-token") == std::string::npos);
    for (int n = 0; n < 6; ++n)
    {
        const std::string token = "sample<" + std::to_string(n) + ">";
        const size_t first = text.find(token);
        CHECK((first != std::string::npos) == (n % 2 == 0));
        CHECK(first == text.rfind(token));
    }
    for (bool begin : {false, true})
    {
        LogSession boundary;
        CHECK(!boundary.config.m_path.empty());
        log.set(true, begin);
        log.init(boundary.config);
        LOGINFO_EX(boundary.config.m_fileId, "boundary-switch");
        log.set(true, !begin);
        log.uninit(boundary.config.m_fileId);
        const std::string records = ReadVolumes(boundary.config.m_path, volumes);
        const size_t second = records.find('\n', 1);
        CHECK(volumes == 1 && records.compare(0, 4, "\n[1]") == 0 && second != std::string::npos &&
            records.compare(second, 4, "\n[1]") == 0 && records.find('\n', second + 1) == std::string::npos);
        const size_t business = records.find("] : boundary-switch");
        CHECK(business != std::string::npos && business == records.rfind("] : boundary-switch") &&
            (business > second) == begin);
        CHECK((records.find("[  BEGIN]") != std::string::npos) == begin &&
            (records.find("[    END]") != std::string::npos) == !begin);
    }
    return 0;
}

/** 验证滚动后的保留数量与按会话删除
@return 成功返回 0
*/
static int RollFiles()
{
    LogSession s;
    CHECK(!s.config.m_path.empty());
    s.config.m_maxFileBytes = 256;
    s.config.m_checkFileSizeInterval = 1;
    s.config.m_maxFileCount = 3;
    LogManager& log = LogManager::instance();
    log.init(s.config);
    for (int n = 0; n < 80; ++n)
    {
        LOGINFO_EX(s.config.m_fileId, "roll<%d> abcdefghijklmnopqrstuvwxyz", n);
    }
    size_t volumes = 0;
    const std::string text = ReadVolumes(s.config.m_path, volumes);
    CHECK(volumes == 3 && text.find("roll<79>") != std::string::npos);
    const std::vector<std::string> names = Files(s.config.m_path);
    bool first = false;
    for (const std::string& name : names)
    {
        first = first || (name.size() >= 6 && name.substr(name.size()-6) == ".0.log");
    }
    CHECK(first);
    log.deleteFile(s.config.m_fileId);
    CHECK(NoEntries(s.config.m_path) && log.findFileId(s.config.m_path) == -1);
    return 0;
}

/** 检查提交边界、转义、带显式来源的日志及长记录
@return 全部符合时返回 0
*/
static int RecordFormat()
{
    LogSession session;
    CHECK(!session.config.m_path.empty());
    LogManager& log = LogManager::instance();
    log.set(true, false);
    log.init(session.config);
    LOGINFO_EX(session.config.m_fileId, "%s", "one\r\n[1]inside");
    size_t volumes = 0;
    std::string first = ReadVolumes(session.config.m_path, volumes);
    CHECK(volumes == 1 && first.compare(0, 4, "\n[1]") == 0);
    CHECK(first.find('\n', 1) == std::string::npos && first.find("one\\r\\n[1]inside") != std::string::npos);
    LOGINFO_EX(session.config.m_fileId, "%s", "two");
    std::string second = ReadVolumes(session.config.m_path, volumes);
    CHECK(second.compare(0, first.size(), first) == 0 && second.compare(first.size(), 4, "\n[1]") == 0);
    CHECK(second.size() >= 3 && second.substr(second.size() - 3) == "two");
    const std::string longLine(128 * 1024 + 3, 'x');
    LOGINFO_EX(session.config.m_fileId, "%s", longLine.c_str());
    const std::string third = ReadVolumes(session.config.m_path, volumes);
    CHECK(third.compare(0, second.size(), second) == 0);
    CHECK(third.size() > longLine.size() && third.compare(third.size() - longLine.size(), longLine.size(), longLine) == 0);
    log.print(session.config.m_fileId, LogManager::LOG_FATAL, "a/b.cpp", "job", "custom", "12:34:56", 27, "value=%d", 31);
    CHECK(ReadVolumes(session.config.m_path, volumes).find("[12:34:56][  FATAL][ThreadId:27][custom][b.cpp][job] : value=31") != std::string::npos);
    const std::string source("job\0tail", 8);
    log.print(session.config.m_fileId, LogManager::LOG_INFO, "c.cpp", source, "demo", "01:02:03", 5, "%s", "last");
    CHECK(ReadVolumes(session.config.m_path, volumes).find("[c.cpp][" + source + "] : last") != std::string::npos);
    return 0;
}

#ifdef __EMSCRIPTEN__
/** 只让提交标志的刷新失败，检查当前可读文件、控制台和后续写入
@return 当前可见字节、失败时无控制台副本及后续独立提交均符合预期时返回 0
*/
static int FailedMarkerFlush()
{
    LogSession session;
    CHECK(!session.config.m_path.empty());
    session.config.m_outputConsole = true;
    LogManager& log = LogManager::instance();
    log.set(true, false);
    log.init(session.config);
    const int32_t id = session.config.m_fileId;
    CHECK(log.findFileId(session.config.m_path) == id);
    std::ostringstream console;
    std::streambuf* previous = std::cout.rdbuf(console.rdbuf());
    EM_ASM({
        var disk = require('fs');
        var sync = disk.fsyncSync;
        var calls = 0;
        Module.restoreMarkerSync = function() { disk.fsyncSync = sync; return calls; };
        disk.fsyncSync = function(fd) {
            if (++calls === 2) {
                var failure = new Error('injected marker flush failure');
                failure.code = 'EIO';
                throw failure;
            }
            return sync.call(disk, fd);
        };
    });
    log.print(id, LogManager::LOG_INFO, "f.cpp", "task", "app", "01:02:03", 7, "%s", "uncertain");
    const int flushes = EM_ASM_INT({ return Module.restoreMarkerSync(); });
    log.print(id, LogManager::LOG_INFO, "f.cpp", "task", "app", "01:02:03", 7, "%s", "next");
    std::cout.rdbuf(previous);
    log.uninit(id);
    const std::string header = "[01:02:03][   INFO][ThreadId:7][app][f.cpp][task] : ";
    size_t volumes = 0;
    const std::string bytes = ReadVolumes(session.config.m_path, volumes);
    CHECK(flushes == 2 && volumes == 1);
    CHECK(bytes == "\n[1]" + header + "uncertain\n[1]" + header + "next");
    CHECK(console.str() == header + "next\n");
    return 0;
}

/** 模拟正文刷新失败，并检查后续记录仍能独立提交
@return 两条记录的标志及内容均匹配时返回 0
*/
static int FailedBodyFlush()
{
    LogSession session;
    CHECK(!session.config.m_path.empty());
    LogManager& log = LogManager::instance();
    log.set(true, false);
    log.init(session.config);
    CHECK(log.findFileId(session.config.m_path) == session.config.m_fileId);
    // Fail just the next host flush. The following record must still commit.
    EM_ASM({
        var disk = require('fs');
        var original = disk.fsyncSync;
        disk.fsyncSync = function() {
            disk.fsyncSync = original;
            throw new Error("injected file flush failure");
        };
    });
    LOGINFO_EX(session.config.m_fileId, "unconfirmed");
    // 从真实宿主句柄读取每次 fsync 前的最后一条记录，并始终还原原函数。
    EM_ASM({
        var disk = require('fs');
        var original = disk.fsyncSync;
        Module.logFlushPhases = [];
        disk.fsyncSync = function(fd) {
            var size = disk.fstatSync(fd).size;
            var bytes = Buffer.alloc(size);
            disk.readSync(fd, bytes, 0, size, 0);
            Module.logFlushPhases.push(bytes.toString().split('\n').pop());
            return original.call(disk, fd);
        };
        Module.restoreLogSync = function() { disk.fsyncSync = original; };
    });
    LOGINFO_EX(session.config.m_fileId, "confirmed");
    CHECK(EM_ASM_INT({
        Module.restoreLogSync();
        var phases = Module.logFlushPhases;
        return phases.length === 2 && phases[0].startsWith('[0]') && phases[0].endsWith('confirmed') &&
            phases[1] === '[1]' + phases[0].slice(3);
    }));
    size_t volumes = 0;
    const std::string text = ReadVolumes(session.config.m_path, volumes);
    CHECK(volumes == 1 && text.compare(0, 4, "\n[0]") == 0);
    CHECK(text.find("unconfirmed\n[1]") != std::string::npos);
    CHECK(text.size() >= 9 && text.substr(text.size() - 9) == "confirmed");
    // 把故障固定在记录的一个字节偏移，短写长度由距该偏移的剩余空间决定。
    const std::string header = "[04:05:06][   INFO][ThreadId:7][app][f.cpp][task] : ";
    const std::string body = header + "partial-body-missing-tail";
    const size_t half = body.size() / 2;
    EM_ASM({
        var disk = require('fs');
        var write = disk.writeSync;
        var cutoff = $0;
        Module.restoreLogWrite = function() { disk.writeSync = write; };
        disk.writeSync = function(fd, buffer, offset, length, position) {
            if (position == null) {
                return write.apply(disk, arguments);
            }
            var remaining = cutoff - Number(position);
            if (remaining <= 0) {
                disk.writeSync = write;
                var failure = new Error('partial record');
                failure.code = 'EIO';
                throw failure;
            }
            return write.call(disk, fd, buffer, offset, Math.min(length, remaining), position);
        };
    }, static_cast<int>(text.size() + 4 + half));
    log.print(session.config.m_fileId, LogManager::LOG_INFO, "f.cpp", "task", "app", "04:05:06", 7, "%s", "partial-body-missing-tail");
    EM_ASM({ Module.restoreLogWrite(); });
    log.print(session.config.m_fileId, LogManager::LOG_INFO, "f.cpp", "task", "app", "04:05:06", 7, "%s", "later-committed");
    const std::string expected = text + "\n[0]" + body.substr(0, half) + std::string(body.size() - half, '\0') +
        "\n[1]" + header + "later-committed";
    const std::string partial = ReadVolumes(session.config.m_path, volumes);
    CHECK(volumes == 1 && partial == expected);
    log.uninit(session.config.m_fileId);
    return FailedMarkerFlush();
}
#endif

#ifndef __EMSCRIPTEN__
/** 四个线程共用一个日志文件，检查每条消息唯一可读
@param [in] maxFileBytes 是否跨卷由此字节上限决定
@return 成功返回 0
*/
static int ConcurrentWritesWithLimit(int64_t maxFileBytes)
{
    LogSession s;
    CHECK(!s.config.m_path.empty());
    s.config.m_maxFileBytes = maxFileBytes;
    s.config.m_maxFileCount = 400;
    s.config.m_checkFileSizeInterval = 1;
    LogManager::instance().init(s.config);
    std::vector<std::thread> writers;
    for (unsigned t = 0; t < 4; ++t)
    {
        writers.emplace_back([&, t]()
        {
            for (unsigned i = 0; i < 80; ++i)
            {
                LOGINFO_EX(s.config.m_fileId, "worker<%u,%u>", t, i);
            }
        });
    }
    for (std::thread& thread : writers)
    {
        thread.join();
    }
    LogManager::instance().uninitAll();
    size_t volumes = 0;
    const std::string text = ReadVolumes(s.config.m_path, volumes);
    CHECK(maxFileBytes == 1024 ? volumes > 1 : volumes == 1);
    std::set<std::string> expected;
    for (unsigned t = 0; t < 4; ++t)
    {
        for (unsigned i = 0; i < 80; ++i)
        {
            expected.insert("worker<" + std::to_string(t) + "," + std::to_string(i) + ">");
        }
    }
    size_t begins = 0, ends = 0;
    CHECK(text.find('\0') == std::string::npos);
    for (size_t pos = 0; pos < text.size(); )
    {
        CHECK(text.compare(pos, 5, "\n[1][") == 0);
        const size_t next = text.find('\n', pos + 1);
        const size_t limit = next == std::string::npos ? text.size() : next;
        const std::string row = text.substr(pos + 4, limit - pos - 4);
        const size_t timeEnd = row.find(']');
        CHECK(timeEnd != std::string::npos);
        if (row.compare(timeEnd + 1, 9, "[   INFO]") == 0)
        {
            const size_t body = row.find("] : ");
            CHECK(body != std::string::npos && expected.erase(row.substr(body + 4)) == 1);
        }
        else
        {
            CHECK(row.find("] : ") == std::string::npos && row.back() == ']');
            if (row.compare(timeEnd + 1, 9, "[  BEGIN]") == 0) { ++begins; }
            else { CHECK(row.compare(timeEnd + 1, 9, "[    END]") == 0); ++ends; }
        }
        pos = limit;
    }
    CHECK(expected.empty() && begins == 1 && ends == 1);
    return 0;
}

/** 同卷多写者验证
@return 所有记录完整且唯一时返回 0
*/
static int ConcurrentWrites()
{
    return ConcurrentWritesWithLimit(1024 * 1024);
}

/** 并发跨卷验证
@return 分卷和记录完整时返回 0
*/
static int ConcurrentRolling()
{
    return ConcurrentWritesWithLimit(1024);
}

#endif

/** 验证可选控制台输出失败时文件依然完成提交和关闭
@return 业务记录和 END 可读、ID 已释放且调用正常返回时为 0
*/
static int ConsoleFailure()
{
    LogSession session;
    CHECK(!session.config.m_path.empty());
    session.config.m_outputConsole = true;
    session.config.m_archiveOldLog = true;
    LogManager& log = LogManager::instance();
    log.set(true, true);
    log.init(session.config);
    CHECK(log.findFileId(session.config.m_path) == session.config.m_fileId);
    std::stringbuf readOnly(std::ios::in);
    const std::ios::iostate initialState = std::cout.rdstate();
    const std::ios::iostate initialMask = std::cout.exceptions();
    std::streambuf* previous = std::cout.rdbuf(&readOnly);
    std::cout.exceptions(std::ios::badbit | std::ios::failbit);
    bool escaped = false;
    try
    {
        LOGINFO_EX(session.config.m_fileId, "console-copy-probe");
    }
    catch (...)
    {
        escaped = true;
    }
    try
    {
        log.uninit(session.config.m_fileId);
    }
    catch (...)
    {
        escaped = true;
    }
    const bool refused = std::cout.fail();
    std::cout.exceptions(std::ios::goodbit);
    std::cout.rdbuf(previous);
    std::cout.clear(initialState);
    std::cout.exceptions(initialMask);
#ifdef _WIN32
    CHECK(refused || GetModuleHandleA("LogManager.dll") || GetModuleHandleA("LogManagerd.dll"));
#else
    CHECK(refused);
#endif
    CHECK(!escaped && log.findFileId(session.config.m_path) == -1);
    CHECK(NoEntries(session.config.m_path));
    const std::vector<std::string> roots = Files(session.config.m_path, true);
    CHECK(roots.size() == 1);
    const std::string root = session.config.m_path + "/" + roots.front();
    const std::vector<std::string> sessions = Files(root, true);
    CHECK(sessions.size() == 1);
    size_t count = 0;
    const std::string text = ReadVolumes(root + "/" + sessions.front(), count);
    CHECK(count == 1 && text.find("[  BEGIN]") != std::string::npos &&
        text.find("console-copy-probe\n[1]") != std::string::npos && text.find("[    END]") != std::string::npos);
    return 0;
}

/** 验证预设复制语义、显式配置覆盖、控制台副本和全局开关
@return 所有可观察效果匹配返回 0
*/
static int WriteWithDefaults()
{
    LogSession first;
    LogSession other;
    CHECK(!first.config.m_path.empty() && !other.config.m_path.empty());
    first.config.m_fileId = 0;
    LogManagerConfig preset = first.config;
    preset.m_fileId = other.config.m_fileId;
    preset.m_outputConsole = true;
    LogManager& log = LogManager::instance();
    log.setDefaultConfig(preset);
    preset.m_outputConsole = false;
    log.set(false, false);
    log.init();
    log.init(other.config);
    CHECK(log.findFileId(first.config.m_path) == -1 && NoEntries(first.config.m_path));
    CHECK(log.findFileId(other.config.m_path) == -1 && NoEntries(other.config.m_path));
    log.set(true, false);
#ifndef _WIN32
    std::ostringstream output;
    std::streambuf* previous = std::cout.rdbuf(output.rdbuf());
#endif
    LOGINFO("default-console");
    log.init(other.config);
    LOGINFO_EX(other.config.m_fileId, "private-file");
#ifndef _WIN32
    std::cout.rdbuf(previous);
    CHECK(output.str().find("default-console") != std::string::npos && output.str().find("private-file") == std::string::npos);
#endif
    CHECK(log.findFileId(first.config.m_path) == 0 && log.findFileId(other.config.m_path) == other.config.m_fileId);
    size_t count = 0;
    const std::string text = ReadVolumes(first.config.m_path, count);
    CHECK(count == 1 && text.find("default-console") != std::string::npos && text.find("private-file") == std::string::npos);
    CHECK(text.find("BEGIN") == std::string::npos);
    CHECK(ReadVolumes(other.config.m_path, count).find("private-file") != std::string::npos);
    log.deleteFile(0);
    log.uninit(other.config.m_fileId);
    log.setDefaultConfig(other.config);
    log.init();
    CHECK(log.findFileId(other.config.m_path) == other.config.m_fileId);
    log.uninit(other.config.m_fileId);
    return ConsoleFailure();
}

/** 核验预设参数及实际控制台输出，Windows 从子进程继承的标准输出读取 DLL 或静态库的字节
@return 子进程、文件内容和输出选择均符合预期时返回 0
*/
static int Defaults()
{
#ifdef _WIN32
    LogSession session;
    CHECK(!session.config.m_path.empty());
    const std::string capture = session.config.m_path + "/console.txt";
    uint32_t pid = 0;
    uint64_t monitor = 0;
    CHECK(CSystem::startCapturedProcess(CSystem::currentExecutablePathUtf8(), {"--console-child"},
        CStringManager::AnsiToUtf8(capture), pid, monitor) == 0);
    const DWORD wait = WaitForSingleObject(reinterpret_cast<HANDLE>(static_cast<uintptr_t>(monitor)), 30000);
    int32_t exitCode = -1;
    const int32_t state = CSystem::pollProcessMonitor(monitor, exitCode);
    CSystem::closeProcessMonitor(monitor);
    CHECK(wait == WAIT_OBJECT_0 && state == 1 && exitCode == 0);
    std::ifstream input(capture.c_str(), std::ios::binary);
    CHECK(input.good());
    const std::string output((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    CHECK(output.find("default-console") != std::string::npos && output.find("private-file") == std::string::npos);
    CHECK(output.find("LogManager: console output failed") != std::string::npos || output.find("console-copy-probe") != std::string::npos);
    return 0;
#else
    return WriteWithDefaults();
#endif
}

/** 验证按实际写入条数触发检查，以及有界/无界的保留数
@return 独立统计的卷数与首末条符合预期返回 0
*/
static int LimitSettings()
{
    LogManager& log = LogManager::instance();
    const int32_t intervals[] = {3, 0, -4, 1, 1};
    const int32_t limits[] = {-1, 1, 2, 0, -2};
    const std::string header = "\n[1][04:05:06][   INFO][ThreadId:7][app][f.cpp][task] : ";
    for (size_t index = 0; index < sizeof(limits) / sizeof(limits[0]); ++index)
    {
        LogSession test;
        CHECK(!test.config.m_path.empty());
        log.set(true, false);
        test.config.m_maxFileBytes = 1;
        test.config.m_maxFileCount = limits[index];
        test.config.m_checkFileSizeInterval = intervals[index];
        log.init(test.config);
        std::string base;
        for (const std::string& name : Files(test.config.m_path))
        {
            if (name.size() > 6 && name.compare(name.size() - 6, 6, ".0.log") == 0)
            {
                base = name.substr(0, name.size() - 6);
            }
        }
        CHECK(!base.empty());
        std::map<std::string, std::string> expected;
        for (int n = 0; n < 10; ++n)
        {
            log.print(test.config.m_fileId, LogManager::LOG_INFO, "f.cpp", "task", "app", "04:05:06", 7, "limit<%d>", n);
            // 不限卷数时每三条一卷；其余四组逐条换卷，只应留下首卷和末卷。
            if (index == 0 || n == 0 || n == 9)
            {
                const int volume = index == 0 ? n / 3 : n;
                expected[base + "." + std::to_string(volume) + ".log"] += header + "limit<" + std::to_string(n) + ">";
            }
        }
        log.uninit(test.config.m_fileId);
        bool complete = false;
        for (const std::string& name : Files(test.config.m_path, false, &complete))
        {
            if (name == base + ".log")
            {
                continue;
            }
            const auto entry = expected.find(name);
            CHECK(entry != expected.end());
            std::ifstream input((test.config.m_path + "/" + name).c_str(), std::ios::binary);
            CHECK(input.good());
            const std::string bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
            CHECK(bytes == entry->second);
            expected.erase(entry);
        }
        CHECK(complete && expected.empty());
    }
    return 0;
}

/** 让间隔表达式重入同一采样点，检验外层调用先取得自己的序号
@param [in] fileId 已打开的私有会话
@param [in] nested 内层调用直接给出间隔，外层调用先触发一次内层调用
*/
static void SampleNestedInterval(int32_t fileId, bool nested)
{
    LOGINFO_EX_EVERY(fileId, nested ? 2 : (SampleNestedInterval(fileId, true), 2), "%s", nested ? "nested-call" : "outer-call");
}

/** 所有默认 ID 和指定 ID 的宏都从实体卷逐条核对
@return 各级别及采样方式的内容和位置符合预期返回 0
*/
static int Macros()
{
    LogSession first;
    LogSession other;
    CHECK(!first.config.m_path.empty() && !other.config.m_path.empty());
    first.config.m_fileId = 0;
    LogManager::instance().set(true, false);
    LogManager::instance().init(first.config);
    LogManager::instance().init(other.config);
    for (int i = 0; i < 6; ++i)
    {
        LOGDEBUG("token:LOGDEBUG:%d;", i);
        LOGDEBUG_EX(other.config.m_fileId, "token:LOGDEBUG_EX:%d;", i);
        LOGDEBUG_EVERY(3, "token:LOGDEBUG_EVERY:%d;", i);
        LOGDEBUG_EX_EVERY(other.config.m_fileId, 3, "token:LOGDEBUG_EX_EVERY:%d;", i);
        LOGDEBUG_ONCE("token:LOGDEBUG_ONCE:%d;", i);
        LOGDEBUG_EX_ONCE(other.config.m_fileId, "token:LOGDEBUG_EX_ONCE:%d;", i);
        LOGINFO("token:LOGINFO:%d;", i);
        LOGINFO_EX(other.config.m_fileId, "token:LOGINFO_EX:%d;", i);
        LOGINFO_EVERY(3, "token:LOGINFO_EVERY:%d;", i);
        LOGINFO_EX_EVERY(other.config.m_fileId, 3, "token:LOGINFO_EX_EVERY:%d;", i);
        LOGINFO_ONCE("token:LOGINFO_ONCE:%d;", i);
        LOGINFO_EX_ONCE(other.config.m_fileId, "token:LOGINFO_EX_ONCE:%d;", i);
        LOGWARNING("token:LOGWARNING:%d;", i);
        LOGWARNING_EX(other.config.m_fileId, "token:LOGWARNING_EX:%d;", i);
        LOGWARNING_EVERY(3, "token:LOGWARNING_EVERY:%d;", i);
        LOGWARNING_EX_EVERY(other.config.m_fileId, 3, "token:LOGWARNING_EX_EVERY:%d;", i);
        LOGWARNING_ONCE("token:LOGWARNING_ONCE:%d;", i);
        LOGWARNING_EX_ONCE(other.config.m_fileId, "token:LOGWARNING_EX_ONCE:%d;", i);
        LOGERROR("token:LOGERROR:%d;", i);
        LOGERROR_EX(other.config.m_fileId, "token:LOGERROR_EX:%d;", i);
        LOGERROR_EVERY(3, "token:LOGERROR_EVERY:%d;", i);
        LOGERROR_EX_EVERY(other.config.m_fileId, 3, "token:LOGERROR_EX_EVERY:%d;", i);
        LOGERROR_ONCE("token:LOGERROR_ONCE:%d;", i);
        LOGERROR_EX_ONCE(other.config.m_fileId, "token:LOGERROR_EX_ONCE:%d;", i);
        LOGFATAL("token:LOGFATAL:%d;", i);
        LOGFATAL_EX(other.config.m_fileId, "token:LOGFATAL_EX:%d;", i);
        LOGFATAL_EVERY(3, "token:LOGFATAL_EVERY:%d;", i);
        LOGFATAL_EX_EVERY(other.config.m_fileId, 3, "token:LOGFATAL_EX_EVERY:%d;", i);
        LOGFATAL_ONCE("token:LOGFATAL_ONCE:%d;", i);
        LOGFATAL_EX_ONCE(other.config.m_fileId, "token:LOGFATAL_EX_ONCE:%d;", i);
    }
    const int logManagerCallIndex = 3;
    int evaluations = 0;
    for (int row = 0; row < 8; ++row)
    {
        const int32_t id = row % 2 == 0 ? first.config.m_fileId : other.config.m_fileId;
        LOGWARNING_EX_EVERY(id, logManagerCallIndex, "caller<%d,%d,%d>", row, static_cast<int>(logManagerCallIndex), ++evaluations);
    }
    CHECK(evaluations == 3);
    SampleNestedInterval(other.config.m_fileId, false);
#ifndef __EMSCRIPTEN__
    std::atomic<unsigned> ready(0);
    std::atomic<unsigned> hits(0);
    std::atomic<unsigned> once(0);
    std::atomic<bool> start(false);
    std::vector<std::thread> threads;
    for (int worker = 0; worker < 4; ++worker)
    {
        // 同时开始，所有线程命中同一对宏调用点；序号实参只应在选中时递增。
        threads.emplace_back([&]()
        {
            ready.fetch_add(1);
            while (!start.load())
            {
                std::this_thread::yield();
            }
            for (int row = 0; row < 16; ++row)
            {
                LOGINFO_EX_EVERY(other.config.m_fileId, 7, "thread-sample<%u>", hits.fetch_add(1));
                LOGERROR_EX_ONCE(other.config.m_fileId, "thread-once<%u>", once.fetch_add(1));
            }
        });
    }
    while (ready.load() != 4)
    {
        std::this_thread::yield();
    }
    start.store(true);
    for (std::thread& thread : threads)
    {
        thread.join();
    }
    CHECK(hits.load() == 10 && once.load() == 1);
#endif
    LogManager::instance().uninitAll();
    const char* labels[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};
    const char* fields[] = {"[  DEBUG]", "[   INFO]", "[WARNING]", "[  ERROR]", "[  FATAL]"};
    const char* modes[] = {"", "_EVERY", "_ONCE"};
    for (int id = 0; id < 2; ++id)
    {
        std::map<std::string, std::string> expected;
        for (size_t level = 0; level < 5; ++level)
        {
#ifndef _DEBUG
            if (level == 0)
            {
                continue;
            }
#endif
            for (int mode = 0; mode < 3; ++mode)
            {
                for (int i = 0; i < 6; ++i)
                {
                    if (mode == 0 || (mode == 1 && i % 3 == 0) || (mode == 2 && i == 0))
                    {
                        const std::string token = std::string("token:LOG") + labels[level] + (id ? "_EX" : "") +
                            modes[mode] + ":" + std::to_string(i) + ";";
                        expected[token] = fields[level];
                    }
                }
            }
        }
        if (id == 0)
        {
            expected["caller<0,3,1>"] = "[WARNING]";
            expected["caller<6,3,3>"] = "[WARNING]";
        }
        else
        {
            expected["caller<3,3,2>"] = "[WARNING]";
            expected["outer-call"] = "[   INFO]";
#ifndef __EMSCRIPTEN__
            expected["thread-once<0>"] = "[  ERROR]";
            for (unsigned hit = 0; hit < 10; ++hit)
            {
                expected["thread-sample<" + std::to_string(hit) + ">"] = "[   INFO]";
            }
#endif
        }
        size_t volumes = 0;
        const std::string data = ReadVolumes(id == 0 ? first.config.m_path : other.config.m_path, volumes);
        CHECK(volumes == 1);
        for (size_t start = 0; start < data.size(); )
        {
            const size_t next = data.find('\n', start + 1);
            const size_t limit = next == std::string::npos ? data.size() : next;
            const std::string row = data.substr(start, limit - start);
            CHECK(row.compare(0, 5, "\n[1][") == 0);
            const size_t timeEnd = row.find(']', 5);
            const size_t body = row.find("] : ");
            CHECK(timeEnd != std::string::npos && body != std::string::npos && body > timeEnd + 9);
            const auto item = expected.find(row.substr(body + 4));
            if (item == expected.end() || row.compare(timeEnd + 1, 9, item->second) != 0)
            {
                std::cerr << "unexpected record in ID " << (id == 0 ? first.config.m_fileId : other.config.m_fileId)
                    << ": " << row.substr(1) << std::endl;
                return 1;
            }
            expected.erase(item);
            start = limit;
        }
        CHECK(expected.empty());
    }
    return 0;
}

/** 正常关闭仅归档自身分卷并保留其他日志
@return 当前目录和归档目录的实际内容符合预期返回 0
*/
static int Archive()
{
    LogSession test;
    CHECK(!test.config.m_path.empty());
    test.config.m_archiveOldLog = true;
    const std::string foreign = test.config.m_path + "/foreign.0.log";
    std::ofstream(foreign.c_str()) << "preserve";
    LogManager& log = LogManager::instance();
    log.init(test.config);
    LOGINFO_EX(test.config.m_fileId, "owned-token");
    log.uninit(test.config.m_fileId);
    size_t count = 0;
    CHECK(ReadVolumes(test.config.m_path, count) == "preserve" && count == 1);
    auto dirs = Files(test.config.m_path, true);
    CHECK(dirs.size() == 1);
    std::string parent = test.config.m_path + "/" + dirs[0];
    dirs = Files(parent, true);
    CHECK(dirs.size() == 1);
    const std::string archived = ReadVolumes(parent + "/" + dirs[0], count);
    CHECK(count == 1 && archived.find("owned-token") != std::string::npos && archived.find("BEGIN") != std::string::npos && archived.find("END") != std::string::npos);
    return 0;
}

/** 跨过内置 100 MiB 阈值，确认 -1 真正关闭按大小滚动
@return 写后只有包含尾标记的首卷时返回 0
*/
static int UnlimitedBytes()
{
    LogSession test;
    CHECK(!test.config.m_path.empty());
    test.config.m_maxFileBytes = -1;
    test.config.m_checkFileSizeInterval = 1;
    LogManager& log = LogManager::instance();
    log.set(true, false);
    log.init(test.config);
    const std::string block(1024 * 1024, 'x');
    for (int i = 0; i < 101; ++i)
    {
        LOGINFO_EX(test.config.m_fileId, "%s", block.c_str());
    }
    LOGINFO_EX(test.config.m_fileId, "limit-end");
    size_t entities = 0;
    for (const auto& name : Files(test.config.m_path))
    {
        if (name.size() < 6 || name.substr(name.size() - 6) != ".0.log")
        {
            CHECK(name.size() < 6 || name.substr(name.size() - 6) != ".1.log");
            continue;
        }
        ++entities;
        std::ifstream input((test.config.m_path + "/" + name).c_str(), std::ios::binary | std::ios::ate);
        CHECK(input.tellg() > std::streamoff(100LL * 1024 * 1024));
        input.seekg(-9, std::ios::end);
        char tail[9];
        CHECK(input.read(tail, 9) && std::string(tail, 9) == "limit-end");
    }
    CHECK(entities == 1);
    return 0;
}

/** 用真实卷边界验证无效字节上限经过预设配置后恢复到 100 MiB
@return 前 100 个 1 MiB 载荷在首卷，超限后的下一条独占第 1 卷时返回 0
*/
static int DefaultBytes()
{
    const int64_t limits[] = {0, -2};
    const std::string block(1024 * 1024, 'x');
    for (const int64_t limit : limits)
    {
        LogSession session;
        CHECK(!session.config.m_path.empty());
        session.config.m_maxFileBytes = limit;
        session.config.m_checkFileSizeInterval = 1;
        LogManager& log = LogManager::instance();
        log.set(true, false);
        log.setDefaultConfig(session.config);
        log.init();
        for (int index = 0; index < 100; ++index)
        {
            LOGINFO_EX(session.config.m_fileId, "%s", block.c_str());
        }
        LOGINFO_EX(session.config.m_fileId, "rolled-after-default");
        log.uninit(session.config.m_fileId);
        size_t first = 0, next = 0;
        for (const auto& name : Files(session.config.m_path))
        {
            if (name.size() < 6)
            {
                continue;
            }
            const std::string suffix = name.substr(name.size() - 6);
            if (suffix == ".0.log")
            {
                std::ifstream volume((session.config.m_path + "/" + name).c_str(), std::ios::binary | std::ios::ate);
                CHECK(volume.tellg() > std::streamoff(100LL * 1024 * 1024));
                ++first;
            }
            if (suffix == ".1.log")
            {
                std::ifstream volume((session.config.m_path + "/" + name).c_str(), std::ios::binary | std::ios::ate);
                CHECK(volume.tellg() > 4 && volume.tellg() < 1024);
                volume.seekg(0);
                const std::string row((std::istreambuf_iterator<char>(volume)), std::istreambuf_iterator<char>());
                CHECK(row.compare(0, 4, "\n[1]") == 0 && row.find('\n', 1) == std::string::npos &&
                    row.find("rolled-after-default") != std::string::npos);
                ++next;
            }
        }
        CHECK(first == 1 && next == 1);
    }
    return 0;
}

/** 非法初始化不注册 ID、不截短路径；失败后可重试，同 ID 不覆盖已有配置
@return 对路径、会话和启停状态的核对全部通过返回 0
*/
static int InvalidSettings()
{
    LogSession test;
    LogSession other;
    CHECK(!test.config.m_path.empty() && !other.config.m_path.empty());
    LogManager& log = LogManager::instance();
    LogManagerConfig bad = test.config;
    bad.m_path += std::string("\0tail", 5);
    log.init(bad);
    CHECK(log.findFileId(test.config.m_path) == -1 && NoEntries(test.config.m_path));
    bad = test.config;
    bad.m_fileId = -1;
    log.init(bad);
    CHECK(NoEntries(test.config.m_path));
    const std::string blocker = test.config.m_path + "/block";
    std::ofstream(blocker.c_str()) << "saved";
    bad = test.config;
    bad.m_path = blocker + "/child";
    log.init(bad);
    CHECK(log.findFileId(bad.m_path) == -1);
    log.init(test.config);
    log.init(other.config);
    LOGINFO_EX(test.config.m_fileId, "first binding");
    CHECK(log.findFileId(test.config.m_path) == test.config.m_fileId && log.findFileId(other.config.m_path) == -1);
    log.set(false, true);
    log.uninit(test.config.m_fileId);
    size_t count = 0;
    const std::string text = ReadVolumes(test.config.m_path, count);
    CHECK(count == 1 && text.find("first binding") != std::string::npos && text.find("[    END]") == std::string::npos);
    return 0;
}

/** 验证已打开分卷无法被排他重建；目标已存在时归档保留两份原内容
@return 字节、源文件及预建目标均符合预期返回 0
*/
static int ExclusivePaths()
{
    LogSession test;
    CHECK(!test.config.m_path.empty());
    LogManager& log = LogManager::instance();
    log.set(true, false);
    log.init(test.config);
    LOGINFO_EX(test.config.m_fileId, "original");
    std::string path;
    for (const auto& name : Files(test.config.m_path))
    {
        if (name.size() > 6 && name.substr(name.size() - 6) == ".0.log")
        {
            path = test.config.m_path + "/" + name;
        }
    }
    CHECK(!path.empty() && log.findFileId(path) == test.config.m_fileId);
    CHECK(CSystem::createDurableFile(path) == -1);
    LOGINFO_EX(test.config.m_fileId, "second");
    log.uninit(test.config.m_fileId);
    const std::string target = test.config.m_path + "/kept";
    std::ofstream(target.c_str()) << "existing";
    CHECK(!CSystem::movePathIfAbsent(path, target));
    size_t count = 0;
    const std::string text = ReadVolumes(test.config.m_path, count);
    std::string kept;
    std::ifstream(target.c_str()) >> kept;
    CHECK(kept == "existing" && count == 1 && text.find("original\n[1]") != std::string::npos && text.find("second") != std::string::npos);
    return 0;
}

/** 占用两个未来卷名，核验跳号、保留数量和外部文件内容
@return 新卷实际接收消息且会话删除后仅剩外部文件时返回 0
*/
static int RollCollision()
{
    LogSession session;
    CHECK(!session.config.m_path.empty());
    session.config.m_maxFileBytes = 1;
    session.config.m_maxFileCount = 2;
    session.config.m_checkFileSizeInterval = 1;
    const int32_t id = session.config.m_fileId;
    LogManager& log = LogManager::instance();
    log.set(true, false);
    log.init(session.config);
    LOGINFO_EX(id, "first-volume");
    std::string prefix;
    for (const auto& name : Files(session.config.m_path))
    {
        if (name.size() > 6 && name.substr(name.size() - 6) == ".0.log")
        {
            prefix = session.config.m_path + "/" + name.substr(0, name.size() - 5);
        }
    }
    CHECK(!prefix.empty());
#ifdef __EMSCRIPTEN__
    // 拒绝一次排他新建，下一次打开恢复正常；已打开的首卷仍须接收本次消息。
    EM_ASM({
        var fs = require('fs');
        var open = fs.openSync;
        Module.rejectedLogRoll = false;
        Module.restoreLogOpen = function() { fs.openSync = open; };
        fs.openSync = function(path, flags) {
            if (flags === 'wx+' && String(path).endsWith('.1.log')) {
                fs.openSync = open;
                Module.rejectedLogRoll = true;
                throw new Error('injected create failure');
            }
            return open.apply(fs, arguments);
        };
    });
    LOGINFO_EX(id, "kept-on-open-failure");
    CHECK(EM_ASM_INT({ Module.restoreLogOpen(); return Module.rejectedLogRoll; }));
    size_t initial = 0;
    CHECK(ReadVolumes(session.config.m_path, initial).find("kept-on-open-failure") != std::string::npos && initial == 1);
#endif
    for (int index = 1; index <= 2; ++index)
    {
        std::ofstream file((prefix + std::to_string(index) + ".log").c_str(), std::ios::binary);
        file << "foreign-" << index;
        file.close();
        CHECK(file.good());
    }
    LOGINFO_EX(id, "third-volume");
    CHECK(log.findFileId(prefix + "3.log") == id);
    {
        std::ifstream file((prefix + "3.log").c_str(), std::ios::binary);
        const std::string row((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        CHECK(row.compare(0, 4, "\n[1]") == 0 && row.find("third-volume") != std::string::npos);
    }
    LOGINFO_EX(id, "fourth-volume");
    CHECK(log.findFileId(prefix + "4.log") == id && !std::ifstream((prefix + "3.log").c_str()).good());
    size_t count = 0;
    const std::string text = ReadVolumes(session.config.m_path, count);
    CHECK(count == 4 && text.find("first-volume") != std::string::npos && text.find("fourth-volume") != std::string::npos);
#ifdef __EMSCRIPTEN__
    // 只让当前会话的第 4 卷暂时拒绝删除和属性查询，下一次滚动应重试清理。
    EM_ASM({
        var disk = require('fs');
        var stat = disk.lstatSync;
        var remove = disk.unlinkSync;
        var target = UTF8ToString(Number($0));
        var host = NODEFS.realPath(FS.lookupPath(target).node);
        Module.logDeniedRetention = false;
        function check(path) {
            if (path === host) {
                Module.logDeniedRetention = true;
                var e = new Error('volume temporarily inaccessible');
                e.code = 'EACCES';
                throw e;
            }
        }
        disk.lstatSync = function(path) { check(path); return stat.apply(disk, arguments); };
        disk.unlinkSync = function(path) { check(path); return remove.apply(disk, arguments); };
        Module.restoreLogRetention = function() { disk.lstatSync = stat; disk.unlinkSync = remove; };
    }, (prefix + "4.log").c_str());
    LOGINFO_EX(id, "fifth-volume");
    CHECK(EM_ASM_INT({ Module.restoreLogRetention(); return Module.logDeniedRetention; }));
    CHECK(std::ifstream((prefix + "4.log").c_str()).good() && log.findFileId(prefix + "5.log") == id);
    LOGINFO_EX(id, "sixth-volume");
    const std::string recovered = ReadVolumes(session.config.m_path, count);
    CHECK(count == 4 && log.findFileId(prefix + "6.log") == id && recovered.find("first-volume") != std::string::npos &&
        recovered.find("sixth-volume") != std::string::npos && recovered.find("fourth-volume") == std::string::npos &&
        recovered.find("fifth-volume") == std::string::npos);
#endif
    log.deleteFile(id);
    CHECK(Files(session.config.m_path).size() == 2);
    for (int index = 1; index <= 2; ++index)
    {
        std::ifstream file((prefix + std::to_string(index) + ".log").c_str(), std::ios::binary);
        const std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        CHECK(data == "foreign-" + std::to_string(index));
    }
    return 0;
}

/** 从两套独立的预设打开在途会话，验证更新后的参数只影响随后打开的 ID
@return 独立读回的卷数和逐条消息归属均正确时返回 0
*/
static int PresetIsolation()
{
    LogSession first, second;
    CHECK(!first.config.m_path.empty() && !second.config.m_path.empty());
    first.config.m_maxFileBytes = -1;
    second.config.m_fileId = first.config.m_fileId + 1;
    second.config.m_maxFileBytes = 1;
    second.config.m_maxFileCount = 2;
    second.config.m_checkFileSizeInterval = 1;
    LogManager& log = LogManager::instance();
    log.set(true, false);
    log.setDefaultConfig(first.config);
    log.init();
    log.setDefaultConfig(second.config);
    log.init();
    for (int row = 0; row < 5; ++row)
    {
        LOGINFO_EX(first.config.m_fileId, "unchanged<%d>", row);
        LOGINFO_EX(second.config.m_fileId, "bounded<%d>", row);
    }
    log.uninitAll();
    size_t count = 0;
    const std::string all = ReadVolumes(first.config.m_path, count);
    CHECK(count == 1 && all.find("bounded<") == std::string::npos);
    const std::string retained = ReadVolumes(second.config.m_path, count);
    CHECK(count == 2 && retained.find("unchanged<") == std::string::npos);
    for (int row = 0; row < 5; ++row)
    {
        const std::string id = std::to_string(row) + ">";
        CHECK(all.find("unchanged<" + id) != std::string::npos);
        CHECK((retained.find("bounded<" + id) != std::string::npos) == (row == 0 || row == 4));
    }
    return 0;
}

/** 切换测试进程的工作目录
@param [in] path 已创建的目录
@return 成功时返回 0
*/
static int ChangeDirectory(const std::string& path)
{
#ifdef _WIN32
    return _chdir(path.c_str());
#else
    return chdir(path.c_str());
#endif
}

/** 在两个独占目录间切换，验证相对配置的卷、归档和删除始终作用于原目录
@return 两种关闭方式都保留另一个目录的同名文件时返回 0
*/
static int RelativeDirectory()
{
#ifdef _WIN32
    char* original = _getcwd(nullptr, 0);
#else
    char* original = getcwd(nullptr, 0);
#endif
    CHECK(original != nullptr);
    const std::string saved(original);
    std::free(original);
    LogManager& log = LogManager::instance();
    for (int archive = 0; archive < 2; ++archive)
    {
        LogSession owner, other;
        CHECK(!owner.config.m_path.empty() && !other.config.m_path.empty());
        const std::string source = owner.config.m_path + "/logs";
        const std::string foreign = other.config.m_path + "/logs";
#ifdef _WIN32
        CHECK(_mkdir(foreign.c_str()) == 0);
#else
        CHECK(mkdir(foreign.c_str(), 0700) == 0);
#endif
        LogManagerConfig cfg = owner.config;
        cfg.m_path = "logs";
        cfg.m_maxFileBytes = 1;
        cfg.m_checkFileSizeInterval = 1;
        cfg.m_archiveOldLog = archive != 0;
        log.set(true, false);
        CHECK(ChangeDirectory(owner.config.m_path) == 0);
        log.init(cfg);
        LOGINFO_EX(cfg.m_fileId, "before-change");
        std::string volume;
        for (const std::string& name : Files(source))
        {
            if (name.size() > 6 && name.substr(name.size() - 6) == ".0.log")
            {
                volume = name;
            }
        }
        const bool found = !volume.empty() && log.findFileId("logs/" + volume) == cfg.m_fileId;
        const bool changed = ChangeDirectory(other.config.m_path) == 0;
        const std::string guard = foreign + "/" + volume;
        if (changed && !volume.empty())
        {
            std::ofstream(guard.c_str()) << "untouched";
        }
        const bool lookup = log.findFileId(source) == cfg.m_fileId && log.findFileId("logs") == -1;
        LOGINFO_EX(cfg.m_fileId, "after-change");
        size_t count = 0;
        const std::string text = ReadVolumes(source, count);
        if (archive)
        {
            log.uninit(cfg.m_fileId);
        }
        else
        {
            log.deleteFile(cfg.m_fileId);
        }
        CHECK(ChangeDirectory(saved) == 0);
        std::string content;
        std::ifstream(guard.c_str()) >> content;
        CHECK(changed && found && lookup && content == "untouched");
        CHECK(count == 2 && text.find("before-change") != std::string::npos && text.find("after-change") != std::string::npos);
        CHECK(NoEntries(source) && Files(foreign).size() == 1 && NoEntries(foreign, true));
        const std::vector<std::string> roots = Files(source, true);
        CHECK(roots.size() == static_cast<size_t>(archive));
        if (archive)
        {
            const std::string root = source + "/" + roots.front();
            const std::vector<std::string> sessions = Files(root, true);
            CHECK(sessions.size() == 1);
            const std::string moved = ReadVolumes(root + "/" + sessions.front(), count);
            CHECK(count == 2 && moved.find("before-change") != std::string::npos && moved.find("after-change") != std::string::npos);
        }
    }
    return 0;
}

char LogTestGroupedNumbers::do_thousands_sep() const
{
    return ',';
}

std::string LogTestGroupedNumbers::do_grouping() const
{
    return "\3";
}

/** 精确命中字节阈值，使用独立的预期字节校验各卷而不依赖目录枚举顺序
@return 首卷两条、下一卷一条，且线程号以无分组十进制编码时返回 0
*/
static int ExactBytes()
{
    LogSession session;
    CHECK(!session.config.m_path.empty());
    const std::string prefix = "\n[1][08:09:10][   INFO][ThreadId:12345][demo][main.cpp][task] : row";
    session.config.m_maxFileBytes = static_cast<int64_t>(prefix.size() + 1);
    session.config.m_checkFileSizeInterval = 1;
    LogManager& log = LogManager::instance();
    log.set(true, false);
    log.init(session.config);
    const std::locale original = std::locale::global(std::locale(std::locale::classic(), new LogTestGroupedNumbers));
    std::ostringstream display;
    bool completed = true;
    try
    {
        display << 12345;
        for (int row = 1; row <= 3; ++row)
        {
            log.print(session.config.m_fileId, LogManager::LOG_INFO, "main.cpp", "task", "demo", "08:09:10", 12345, "row%d", row);
        }
    }
    catch (...)
    {
        completed = false;
    }
    std::locale::global(original);
    log.uninit(session.config.m_fileId);
    CHECK(completed && display.str() == "12,345");
    size_t volumes = 0;
    for (const auto& name : Files(session.config.m_path))
    {
        if (name.size() < 6) { continue; }
        const std::string suffix = name.substr(name.size() - 6);
        if (suffix != ".0.log" && suffix != ".1.log") { continue; }
        std::ifstream volume((session.config.m_path + "/" + name).c_str(), std::ios::binary);
        const std::string data((std::istreambuf_iterator<char>(volume)), std::istreambuf_iterator<char>());
        CHECK(data == (suffix == ".0.log" ? prefix + "1" + prefix + "2" : prefix + "3"));
        ++volumes;
    }
    size_t total = 0;
    ReadVolumes(session.config.m_path, total);
    CHECK(volumes == 2 && total == 2);
    return 0;
}

/** 计量采样判断及同步日志写入，计时后按落盘记录数核验
@return 成功返回 0
*/
static int Benchmark()
{
    bool validSampling = true;
    LibraryBenchmark::run("log.needPrintEvery.7", [&](uint32_t count) -> uint64_t
    {
        uint64_t hits = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            hits += LogManager::needPrintEvery(i, 7);
        }
        if (hits != (uint64_t(count) + 6) / 7)
        {
            validSampling = false;
        }
        return hits;
    });
    CHECK(validSampling);
    LogSession session;
    CHECK(!session.config.m_path.empty());
    LogManager& log = LogManager::instance();
    session.config.m_maxFileBytes = -1;
    log.set(true, false);
    log.init(session.config);
    CHECK(log.findFileId(session.config.m_path) == session.config.m_fileId);
    uint64_t written = 0;
    LibraryBenchmark::run("log.print.info.flush", [&](uint32_t count) -> uint64_t
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            LOGINFO_EX(session.config.m_fileId, "write-cost<%010u>", static_cast<unsigned>(written + i));
        }
        written += count;
        return written;
    });
    log.uninit(session.config.m_fileId);
    size_t volumes = 0;
    const std::string text = ReadVolumes(session.config.m_path, volumes);
    uint64_t found = 0;
    for (size_t pos = 0; pos < text.size(); )
    {
        CHECK(text.compare(pos, 4, "\n[1]") == 0);
        const size_t end = text.find('\n', pos + 1);
        const std::string digits = std::to_string(static_cast<uint32_t>(found));
        const std::string suffix = "write-cost<" + std::string(10 - digits.size(), '0') + digits + ">";
        const size_t limit = end == std::string::npos ? text.size() : end;
        CHECK(limit >= pos + 4 + suffix.size() && text.compare(limit - suffix.size(), suffix.size(), suffix) == 0);
        ++found;
        pos = end == std::string::npos ? text.size() : end;
    }
    CHECK(volumes == 1 && written != 0 && found == written);
    return 0;
}

#ifdef __EMSCRIPTEN__
/** 在内存挂载上检查初始化失败没有残留文件
@return 无注册 ID 和实体时返回 0
*/
static int MemoryMount()
{
    LogManagerConfig memory;
    char directory[] = "/log-manager-memory-XXXXXX";
    CHECK(mkdtemp(directory) != nullptr);
    memory.m_path = directory;
    memory.m_fileId = 19;
    LogManager::instance().init(memory);
    LOGINFO_EX(19, "cannot persist");
    CHECK(LogManager::instance().findFileId(memory.m_path) == -1 && NoEntries(memory.m_path));
    CHECK(rmdir(memory.m_path.c_str()) == 0);
    return 0;
}
#endif

/** 解析指定编号，保留编号 8 只报告一次跳过
@param [in] input 命令行中的编号或闭区间
@param [in] maximum 登记表的最大编号
@param [out] normalized 去重、升序且剔除保留编号的选择，供 PlatformTest 校验宿主适用性
@return 语法正确且含有可执行候选项时返回 true
*/
static bool SelectLogIds(const char* input, unsigned maximum, std::string& normalized)
{
    std::set<unsigned> ids;
    const char* cursor = input;
    for (;;)
    {
        unsigned first = 0;
        if (!PlatformTest::caseNumber(cursor, maximum, first)) { return false; }
        unsigned last = first;
        if (*cursor == '-')
        {
            ++cursor;
            if (!PlatformTest::caseNumber(cursor, maximum, last) || last < first) { return false; }
        }
        for (unsigned id = first; id <= last; ++id) { ids.insert(id); }
        if (*cursor == '\0') { break; }
        if (*cursor++ != ',') { return false; }
    }
    if (ids.erase(8) != 0) { std::puts("skip reserved case 8"); }
    normalized.clear();
    for (unsigned id : ids)
    {
        if (!normalized.empty()) { normalized += ','; }
        normalized += std::to_string(id);
    }
    return !normalized.empty();
}

/** 核对登记后按本宿主的能力执行，交给 PlatformTest 统一选择和报告返回码
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数
@param [in] memoryOnly 纯内存 Wasm 宿主只验证采样和拒绝持久化
@return 全部通过为 0，用例失败为 1，选择或登记错误为 2
*/
static int RunLogCases(int argc, char** argv, bool memoryOnly)
{
    typedef std::map<unsigned, LogCase> Registry;
    const std::initializer_list<Registry::value_type> definitions = {
        {LogTestSampling, {false, L"默认参数与采样间隔符合独立算式", Sampling}},
        {LogTestReadBack, {false, L"禁写、采样及边界开关按调用时的设置生效", ReadBack}},
        {LogTestRolling, {true, L"滚动保留三个实体卷，删除后目录为空", RollFiles}},
#ifndef __EMSCRIPTEN__
        {LogTestConcurrent, {true, L"四线程同卷的每条正文完整且唯一", ConcurrentWrites}},
#endif
        {LogTestFormat, {false, L"来源、转义、长消息和提交标志按字节匹配", RecordFormat}},
#ifdef __EMSCRIPTEN__
        {LogTestFailedFlush, {false, L"短写、正文刷新或标志刷新失败后，文件和控制台反映各自提交结果", FailedBodyFlush}},
#else
        {LogTestConcurrentRoll, {true, L"跨卷并发写入时每条正文完整且唯一", ConcurrentRolling}},
#endif
        {LogTestDefaults, {false, L"预设独立生效，控制台失败仍能关闭和归档", Defaults}},
        {LogTestLimits, {false, L"有界和无限设置产生预期数量的卷", LimitSettings}},
        {LogTestMacros, {false, L"实体卷中的实际等级、正文、采样次数和调用方实参正确", Macros}},
        {LogTestArchive, {false, L"本次会话归档，外部文件保留原字节", Archive}},
#ifdef _WIN32
        {LogTestChildren, {true, L"子进程关闭或异常退出后卷归属、链接和提交数据可核验", CheckLogProcesses}},
#endif
#ifdef __EMSCRIPTEN__
        {LogTestMemory, {false, L"内存挂载不能注册持久日志或创建实体卷", MemoryMount}},
#endif
        {LogTestUnlimited, {true, L"单卷无限制时写过默认容量仍只有首卷", UnlimitedBytes}},
        {LogTestInvalid, {false, L"非法配置无副作用，有效配置可以重试", InvalidSettings}},
        {LogTestExclusive, {false, L"排他创建和移动遇到重名保留原内容", ExclusivePaths}},
        {LogTestRelativeDir, {false, L"切换工作目录后仍在原目录操作会话", RelativeDirectory}},
        {LogTestDefaultBytes, {true, L"两种非法字节上限均在默认容量处换卷", DefaultBytes}},
        {LogTestRollCollision, {false, L"换卷跳过外部同名文件并重试暂时失败的清理", RollCollision}},
        {LogTestPresetIsolation, {false, L"更换预设后两个会话的路径和保留策略彼此独立", PresetIsolation}},
        {LogTestExactBytes, {false, L"宿主的数字分组不改变格式或精确换卷边界", ExactBytes}}
    };
    const Registry registry(definitions);
    if (registry.size() != definitions.size()) { return 2; }
    typedef std::vector<const Registry::value_type*> Entries;
    Entries entries;
    for (const auto& entry : registry)
    {
        if (!entry.first || entry.first == 8 || !entry.second.m_expectation || !*entry.second.m_expectation || !entry.second.m_run)
        {
            return 2;
        }
        if (!memoryOnly || entry.first == LogTestSampling || entry.first == LogTestMemory)
        {
            entries.push_back(&entry);
        }
    }
    // context 为本次的有序登记表，index 为宿主内下标，返回编号并填入压力标记。
    const auto info = [](void* context, size_t index, bool& stress) -> unsigned
    {
        const Entries& hostEntries = *static_cast<const Entries*>(context);
        const auto& entry = *hostEntries[index];
        stress = entry.second.m_stress;
        return entry.first;
    };
    // 使用同一条目的中文预期和真实检查，返回用例实际退出码。
    const auto execute = [](void* context, size_t index) -> int
    {
        const Entries& hostEntries = *static_cast<const Entries*>(context);
        const auto& entry = *hostEntries[index];
        const std::string expectation = CStringManager::UnicodeToUtf8(entry.second.m_expectation);
        std::printf("case %u - %s\n", entry.first, expectation.c_str());
        return entry.second.m_run();
    };
    if (!memoryOnly && (argc == 1 || (argc == 2 && std::strcmp(argv[1], "--all") == 0)))
    {
        char instant[] = "--instant";
        char stress[] = "--stress";
        char* selection[] = {argv[0], instant, nullptr};
        const int first = PlatformTest::runCases(2, selection, entries.size(), &entries, info, execute);
        selection[1] = stress;
        const int second = PlatformTest::runCases(2, selection, entries.size(), &entries, info, execute);
        return first != 0 ? first : second;
    }
    if (argc == 3 && std::strcmp(argv[1], "--cases") == 0)
    {
        std::string normalized;
        if (!SelectLogIds(argv[2], registry.rbegin()->first, normalized)) { return 2; }
        char* selection[] = {argv[0], argv[1], &normalized[0], nullptr};
        return PlatformTest::runCases(3, selection, entries.size(), &entries, info, execute);
    }
    return PlatformTest::runCases(argc, argv, entries.size(), &entries, info, execute);
}

/** 根据稳定编号运行功能用例，--benchmark 单独计时
@param [in] argc 命令行参数个数
@param [in] argv 用例选项及对应参数
@return 全部选定用例通过时返回 0，失败返回非零
*/
int main(int argc, char** argv)
{
#ifdef _WIN32
    if (argc == 2 && std::strcmp(argv[1], "--console-child") == 0)
    {
        return WriteWithDefaults();
    }
    if (argc > 1 && (std::strcmp(argv[1], "--child") == 0 || std::strcmp(argv[1], "--archive-child") == 0))
    {
        return RunLogChild(argc, argv);
    }
#endif
#ifdef __EMSCRIPTEN__
    if (!EM_ASM_INT({ return typeof process === 'object' && !!process.versions && !!process.versions.node && typeof require === 'function'; }))
    {
        return RunLogCases(argc, argv, true);
    }
    EM_ASM({ if (!FS.lookupPath('/tmp').node.mount.type || FS.lookupPath('/tmp').node.mount.type !== NODEFS) FS.mount(NODEFS, {root: require('os').tmpdir()}, '/tmp'); });
#endif
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return Benchmark();
    }
    return RunLogCases(argc, argv, false);
}