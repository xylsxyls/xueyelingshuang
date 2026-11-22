#include "CiniTest.h"
#include "Cini/CiniAPI.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdio>
#include <cstring>

#define CHECK(test) do { if (!(test)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; } } while (false)

/** 持有由系统排他创建的 INI 文件，析构删除本次文件 */
struct IniFile
{
    // 测试文件的唯一路径
    std::string path;

    /** 预创建空文件以同时支持两种后端 */
    IniFile()
    {
#ifdef _WIN32
        char dir[MAX_PATH] = {}, file[MAX_PATH] = {};
        DWORD length = GetTempPathA(MAX_PATH, dir);
        if (length && length < MAX_PATH && GetTempFileNameA(dir, "ini", 0, file))
        {
            path = file;
        }
#else
        const char* dir = std::getenv("TMPDIR");
        std::string pattern = std::string(dir && *dir ? dir : "/tmp") + "/cini-XXXXXX";
        int fd = mkstemp(&pattern[0]);
        if (fd >= 0)
        {
            close(fd);
            path.swap(pattern);
        }
#endif
    }

    /** 回收此测试文件 */
    ~IniFile()
    {
        if (!path.empty())
        {
            std::remove(path.c_str());
        }
    }
};

/** 使用内存数据逐项检查查询和替换
@return 成功返回 0
*/
static int ReadMemory()
{
    Cini ini("", true);
    const std::string name = "\xe4\xbd\xa0\xe5\xa5\xbd";
    CHECK(ini.loadData("; comment\r\n[Section]\r\nname = " + name + "\r\nempty=\r\n[B]\nname=other"));
    CHECK(ini.readIni("NAME") == name && ini.readIni("name", "B") == "other");
    CHECK(ini.readIni("empty").empty() && ini.readIni("missing", "B").empty());
    std::vector<std::string> sections = ini.getAllSection(), keys = ini.getAllKey();
    std::sort(sections.begin(), sections.end());
    std::sort(keys.begin(), keys.end());
    CHECK(sections.size() == 2 && sections[0] == "B" && sections[1] == "Section");
    CHECK(keys.size() == 2 && keys[0] == "empty" && keys[1] == "name");
    CHECK(ini.getFirstKey("B") == "name" && ini.getFirstKey("absent").empty());
    CHECK(ini.loadData("[New]\nv=fresh\n") && ini.readIni("name").empty() && ini.readIni("v","New") == "fresh");
    CHECK(ini.loadData("") && ini.getAllSection().empty());
    return 0;
}

/** 从新对象核对文件上的更改及删除
@return 成功返回 0
*/
static int FileChanges()
{
    IniFile file;
    CHECK(!file.path.empty());
    Cini writer(file.path, true);
    CHECK(writer.loadData("[Section]\nbase=first\n[B]\nkeep=yes\n"));
    writer.writeIni("added", "42");
    writer.writeIni("base", "overwritten");
    {
        Cini reader(file.path, true);
        CHECK(reader.readIni("added") == "42" && reader.readIni("base") == "overwritten");
    }
    writer.deleteKey("added");
    CHECK(writer.getAllKey().size() == 1 && writer.readIni("base") == "overwritten");
    writer.deleteSection("Section");
    Cini finalReader(file.path, true);
    CHECK(finalReader.getAllSection().size() == 1 && finalReader.readIni("keep", "B") == "yes");
    return 0;
}

#ifdef _WIN32
/** 通过 Profile API 读取截断文本，使用新对象检查持久化删除
@return 成功返回 0
*/
static int ProfileFile()
{
    IniFile file;
    CHECK(!file.path.empty());
    Cini ini(file.path);
    CHECK(!ini.loadData("[ignored]\nk=v"));
    ini.writeIni("key", "abcdef");
    ini.setMaxLength(4);
    CHECK(ini.readIni("key") == "abc");
    ini.setMaxLength(32);
    ini.setMaxSection(4);
    CHECK(ini.readIni("key") == "abcdef" && ini.getAllSection().size() == 1 && ini.getFirstKey() == "key");
    ini.writeIni("other", "2");
    ini.deleteKey("key");
    CHECK(ini.readIni("key").empty() && ini.readIni("other") == "2");
    ini.deleteSection("Section");
    CHECK(Cini(file.path).readIni("other").empty());
    return 0;
}
#endif

/** 取得配置解析及查找的分批统计，结果由调用次数及读取长度校验
@return 成功返回 0
*/
static int Benchmark()
{
    Cini ini("", true);
    std::string input("[Section]\n");
    for (unsigned i = 0; i < 64; ++i)
    {
        input += "k" + std::to_string(i) + "=payload\n";
    }
    CHECK(ini.loadData(input) && ini.readIni("k63") == "payload");
    LibraryBenchmark::run("ini.loadData.64", [&](uint32_t count) -> uint64_t
    {
        uint64_t successes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            successes += ini.loadData(input);
        }
        if (successes != count)
        {
            throw std::runtime_error("loadData");
        }
        return successes;
    });
    const std::string key("k63"), section("Section");
    LibraryBenchmark::run("ini.readIni.64", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            bytes += ini.readIni(key, section).size();
        }
        if (bytes != uint64_t(count) * 7)
        {
            throw std::runtime_error("readIni");
        }
        return bytes;
    });
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
    const LibraryTestCase cases[] = {{1, false, ReadMemory}, {2, false, FileChanges}
#ifdef _WIN32
        , {3, false, ProfileFile}
#endif
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}