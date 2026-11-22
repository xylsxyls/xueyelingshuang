#include "LogManagerTest.h"
#include "LogManager/LogManagerAPI.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif
#ifndef __EMSCRIPTEN__
#include <thread>
#endif
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>

#define CHECK(test) do { if (!(test)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; } } while (false)

/** 枚举独占测试目录下的文件名
@param [in] dir 已创建的目录
@return 普通文件和链接的文件名
*/
static std::vector<std::string> Files(const std::string& dir)
{
    std::vector<std::string> result;
#ifdef _WIN32
    WIN32_FIND_DATAA info = {};
    HANDLE search = FindFirstFileA((dir + "/*").c_str(), &info);
    if (search != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                result.push_back(info.cFileName);
            }
        } while (FindNextFileA(search, &info));
        FindClose(search);
    }
#else
    DIR* search = opendir(dir.c_str());
    if (search)
    {
        while (dirent* entry = readdir(search))
        {
            if (std::strcmp(entry->d_name, ".") && std::strcmp(entry->d_name, ".."))
            {
                result.push_back(entry->d_name);
            }
        }
        closedir(search);
    }
#endif
    return result;
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
    for (const std::string& name : Files(dir))
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
        text.append(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        ++volumes;
    }
    return text;
}

/** 确保即便断言失败也关闭本测试的文件并清理私有目录 */
struct LogSession
{
    // 本次测试配置和目录
    LogManagerConfig config;

    /** 创建唯一目录和独立的日志编号 */
    LogSession()
    {
#ifdef _WIN32
        char base[MAX_PATH] = {};
        DWORD length = GetTempPathA(MAX_PATH, base);
        if (!length || length >= MAX_PATH)
        {
            return;
        }
        std::string dir = std::string(base) + "LogManagerTest-" + std::to_string(GetCurrentProcessId()) + "-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        if (!CreateDirectoryA(dir.c_str(), nullptr))
        {
            return;
        }
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

    /** 回收创建的日志与链接 */
    ~LogSession()
    {
        if (config.m_path.empty())
        {
            return;
        }
        LogManager::instance().set(true, true);
        LogManager::instance().uninit(config.m_fileId);
        for (const std::string& name : Files(config.m_path))
        {
            std::remove((config.m_path + "/" + name).c_str());
        }
#ifdef _WIN32
        RemoveDirectoryA(config.m_path.c_str());
#else
        rmdir(config.m_path.c_str());
#endif
    }
};

/** 检查采样及配置默认值
@return 成功返回 0
*/
static int Sampling()
{
    const LogManagerConfig c;
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
    for (int n = 0; n < 5; ++n)
    {
        LOGWARNING_EX_EVERY(s.config.m_fileId, 2, "sample<%d>", n);
    }
    log.uninit(s.config.m_fileId);
    size_t volumes = 0;
    const std::string text = ReadVolumes(s.config.m_path, volumes);
    CHECK(volumes == 1 && log.findFileId(s.config.m_path) == -1 && text.find("drop-token") == std::string::npos);
    for (int n = 0; n < 5; ++n)
    {
        CHECK((text.find("sample<" + std::to_string(n) + ">") != std::string::npos) == (n % 2 == 0));
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
    CHECK(volumes >= 2 && volumes <= 3 && text.find("roll<79>") != std::string::npos);
    const std::vector<std::string> names = Files(s.config.m_path);
    bool first = false;
    for (const std::string& name : names)
    {
        first = first || (name.size() >= 6 && name.substr(name.size()-6) == ".0.log");
    }
    CHECK(first);
    log.deleteFile(s.config.m_fileId);
    CHECK(Files(s.config.m_path).empty() && log.findFileId(s.config.m_path) == -1);
    return 0;
}

#ifndef __EMSCRIPTEN__
/** 四个线程共用一个日志文件，检查每条消息唯一可读
@return 成功返回 0
*/
static int ConcurrentWrites()
{
    LogSession s;
    CHECK(!s.config.m_path.empty());
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
    CHECK(volumes == 1);
    for (unsigned t = 0; t < 4; ++t)
    {
        for (unsigned i = 0; i < 80; ++i)
        {
            const std::string token = "worker<" + std::to_string(t) + "," + std::to_string(i) + ">";
            const size_t at = text.find(token);
            CHECK(at != std::string::npos && text.find(token, at + token.size()) == std::string::npos);
        }
    }
    return 0;
}
#endif

/** 计量采样判断及同步日志写入，计时后按落盘记录数核验
@return 成功返回 0
*/
static int Benchmark()
{
    LibraryBenchmark::run("log.needPrintEvery.7", [](uint32_t count) -> uint64_t
    {
        uint64_t hits = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            hits += LogManager::needPrintEvery(i, 7);
        }
        if (hits != (uint64_t(count) + 6) / 7)
        {
            throw std::runtime_error("sampling count");
        }
        return hits;
    });
    LogSession session;
    CHECK(!session.config.m_path.empty());
    LogManager& log = LogManager::instance();
    log.set(true, false);
    log.init(session.config);
    CHECK(log.findFileId(session.config.m_path) == session.config.m_fileId);
    uint64_t written = 0;
    LibraryBenchmark::run("log.print.info.flush", [&](uint32_t count) -> uint64_t
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            LOGINFO_EX(session.config.m_fileId, "write-cost<%u>", i);
        }
        written += count;
        return written;
    });
    log.uninit(session.config.m_fileId);
    size_t volumes = 0;
    const std::string text = ReadVolumes(session.config.m_path, volumes);
    uint64_t found = 0;
    size_t pos = 0;
    while ((pos = text.find("write-cost<", pos)) != std::string::npos)
    {
        ++found;
        ++pos;
    }
    CHECK(volumes != 0 && written != 0 && found == written);
    return 0;
}

/** 根据稳定编号运行功能用例，--benchmark 单独计时
@param [in] argc 命令行参数个数
@param [in] argv 用例选项及对应参数
@return 全部选定用例通过时返回 0，失败返回非零
*/
int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return Benchmark();
    }
    const LibraryTestCase cases[] = {{1, false, Sampling}, {2, false, ReadBack}, {3, true, RollFiles}
#ifndef __EMSCRIPTEN__
        , {4, true, ConcurrentWrites}
#endif
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}