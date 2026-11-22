#include "CiniTest.h"
#include "Cini/CiniAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#ifndef _WIN32
#include "Cini/ConvertUTF.h"
#endif
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <limits>
#include <type_traits>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define CHECK(test) do { if (!(test)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #test); return 1; } } while (false)

IniFile::IniFile(bool createFile)
{
#ifdef _WIN32
    char dir[MAX_PATH] = {};
    char marker[MAX_PATH] = {};
    DWORD length = GetTempPathA(MAX_PATH, dir);
    if (!length || length >= MAX_PATH || !GetTempFileNameA(dir, "ini", 0, marker))
    {
        return;
    }
    std::string directory = std::string(marker) + ".dir";
    bool created = CreateDirectoryA(directory.c_str(), nullptr) != 0;
    DeleteFileA(marker);
    if (!created)
    {
        return;
    }
    m_directory.swap(directory);
#else
    const char* dir = std::getenv("TMPDIR");
    std::string directory = std::string(dir && *dir ? dir : "/tmp") + "/cini-XXXXXX";
    if (!mkdtemp(&directory[0]))
    {
        return;
    }
    m_directory.swap(directory);
#endif
    std::string path = m_directory + "/config.ini";
    if (createFile)
    {
        std::ofstream file(path.c_str(), std::ios::binary);
        file.close();
        if (!file)
        {
            std::remove(path.c_str());
            return;
        }
    }
    m_path.swap(path);
}

IniFile::~IniFile()
{
    if (!m_path.empty())
    {
        std::remove(m_path.c_str());
    }
    if (!m_directory.empty())
    {
#ifdef _WIN32
        RemoveDirectoryA(m_directory.c_str());
#else
        rmdir(m_directory.c_str());
#endif
    }
}

/** 直接读取测试文件的原始字节，用于独立核对写入和删除的副作用
@param [in] path 由当前用例创建的文件路径
@param [out] bytes 文件内容，成功时有效
@return 打开并读取成功时返回 true
*/
static bool ReadFixture(const std::string& path, std::string& bytes)
{
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
    {
        return false;
    }
    bytes.clear();
    char buffer[4096];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
    {
        bytes.append(buffer, static_cast<size_t>(input.gcount()));
    }
    return input.eof() && !input.bad();
}

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
    std::vector<std::string> sections = ini.getAllSection();
    std::vector<std::string> keys = ini.getAllKey();
    std::sort(sections.begin(), sections.end());
    std::sort(keys.begin(), keys.end());
    CHECK(sections.size() == 2 && sections[0] == "B" && sections[1] == "Section");
    CHECK(keys.size() == 2 && keys[0] == "empty" && keys[1] == "name");
    CHECK(ini.getFirstKey("B") == "name" && ini.getFirstKey("absent").empty());
    CHECK(ini.loadData("[New]\nv=fresh\n") && ini.readIni("name").empty() && ini.readIni("v", "New") == "fresh");
    ini.setMaxLength(0);
    ini.setMaxSection(-1);
    CHECK(ini.readIni("v", "New") == "fresh" && ini.getAllSection().size() == 1);
    CHECK(ini.loadData("") && ini.getAllSection().empty());
    return 0;
}

/** 从新对象核对文件上的更改及删除
@return 成功返回 0
*/
static int FileChanges()
{
    IniFile file;
    CHECK(!file.m_path.empty());
    Cini writer(file.m_path, true);
    CHECK(writer.loadData("[Section]\nbase=first\n[B]\nkeep=yes\n"));
    writer.writeIni("added", "42");
    writer.writeIni("base", "overwritten");
    {
        Cini reader(file.m_path, true);
        CHECK(reader.readIni("added") == "42" && reader.readIni("base") == "overwritten");
    }
    const std::string longValue(2048, 'v');
    writer.writeIni("base", longValue);
    CHECK(Cini(file.m_path, true).readIni("base") == longValue);
    writer.writeIni("base", "overwritten");
    writer.deleteKey("added");
    CHECK(writer.getAllKey().size() == 1 && writer.readIni("base") == "overwritten");
    Cini afterDelete(file.m_path, true);
    CHECK(afterDelete.getAllKey().size() == 1 && afterDelete.getFirstKey() == "base");
    CHECK(afterDelete.readIni("base") == "overwritten" && afterDelete.readIni("added").empty());
    writer.deleteSection("Section");
    Cini finalReader(file.m_path, true);
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
    CHECK(!file.m_path.empty());
    Cini ini(file.m_path);
    CHECK(!ini.loadData("[ignored]\nk=v"));
    ini.writeIni("key", "abcdef");
    ini.setMaxLength(4);
    CHECK(ini.readIni("key") == "abc");
    ini.setMaxLength(32);
    ini.setMaxSection(4);
    CHECK(ini.readIni("key") == "abcdef" && ini.getAllSection().size() == 1 && ini.getFirstKey() == "key");
    CHECK(ini.getAllKey().empty());
    const std::string longValue(400, 'v');
    ini.writeIni("long", longValue);
    ini.setMaxLength(401);
    CHECK(ini.readIni("long") == longValue);
    ini.setMaxLength(400);
    CHECK(ini.readIni("long") == longValue.substr(0, 399));
    ini.writeIni("other", "2");
    ini.deleteKey("key");
    CHECK(ini.readIni("key").empty() && ini.readIni("other") == "2");
    ini.writeIni("other", "keep", "Witness");
    ini.deleteSection("Section");
    Cini reader(file.m_path);
    const std::vector<std::string> sections = reader.getAllSection();
    CHECK(sections.size() == 1 && sections.front() == "Witness");
    CHECK(reader.readIni("other").empty() && reader.readIni("other", "Witness") == "keep");
    return 0;
}
#endif

/** 解析带 BOM、重复键、无效节头及空分区的文本
@return 全部观察与固定输入一致时返回 0
*/
static int ParserBoundaries()
{
    Cini ini("", true);
    CHECK(ini.loadData("\xef\xbb\xbf[Section]\nK=old\nk=last\n[bad\nnext=ok\n[Empty]\n"));
    CHECK(ini.readIni("K") == "last" && ini.readIni("next") == "ok");
    std::vector<std::string> sections = ini.getAllSection();
    std::sort(sections.begin(), sections.end());
    CHECK(sections.size() == 2 && sections[0] == "Empty" && sections[1] == "Section");
    CHECK(ini.getAllKey("Empty").empty());
    ini.deleteSection("Empty");
    sections = ini.getAllSection();
    CHECK(sections.size() == 1 && sections.front() == "Section");
    CHECK(ini.getFirstKey() == "K" && ini.readIni("next") == "ok");
    return 0;
}

/** 仅持有一份解析状态，检查从缺失文件首次创建再逐步删除的落盘内容
@return 各生命周期边界均满足时返回 0
*/
static int FileLifecycle()
{
    CHECK(!std::is_copy_constructible<Cini>::value);
#if !defined(_MSC_VER) || _MSC_VER != 1800
    // VS2013 对已删除的赋值运算符误报为可赋值，见头文件的编译检查
    CHECK(!std::is_copy_assignable<Cini>::value);
#endif
    IniFile file(false);
    CHECK(!file.m_path.empty());
    Cini ini(file.m_path, true);
    ini.writeIni("solo", "saved");
    CHECK(ini.readIni("solo") == "saved");
    Cini snapshot(file.m_path, true);
#ifndef _WIN32
    CHECK(Cini(file.m_path).readIni("SOLO") == "saved");
#endif
    CHECK(snapshot.readIni("solo") == "saved");
    ini.deleteKey("solo");
    Cini emptySection(file.m_path, true);
    const std::vector<std::string> sections = emptySection.getAllSection();
    CHECK(emptySection.getAllKey().empty() && sections.size() == 1 && sections.front() == "Section");
    ini.deleteSection("Section");
    std::string remaining;
    CHECK(ReadFixture(file.m_path, remaining) && remaining.empty());
    CHECK(Cini(file.m_path, true).getAllSection().empty());
    CHECK(snapshot.readIni("solo") == "saved");
    return 0;
}

#ifdef _WIN32
/** 检查系统后端的容量边界、长分区名称及不同设置之间的隔离
@return 结果符合缓冲区和数量约定时返回 0
*/
static int ProfileLimits()
{
    IniFile file;
    CHECK(!file.m_path.empty());
    Cini ini(file.m_path);
    ini.writeIni("k", "value", "long-section");
    ini.setMaxLength(4);
    ini.setMaxSection(1);
    CHECK(ini.getAllSection().empty());
    ini.writeIni("k", "other", "B");
    ini.setMaxSection(8);
    std::vector<std::string> names = ini.getAllSection();
    std::sort(names.begin(), names.end());
    CHECK(names.size() == 2 && names[0] == "B" && names[1] == "long-section");
    ini.setMaxLength(-1);
    CHECK(ini.readIni("k", "B").empty() && ini.getFirstKey("B").empty() && ini.getAllSection().empty());
    ini.setMaxLength(1);
    CHECK(ini.readIni("k", "B").empty() && ini.getFirstKey("B").empty());
    ini.setMaxLength((std::numeric_limits<int32_t>::max)());
    ini.setMaxSection(0);
    CHECK(ini.getAllSection().empty() && ini.readIni("k", "B") == "other");
    ini.setMaxSection(1);
    CHECK(ini.getAllSection().size() == 1);
    ini.setMaxSection((std::numeric_limits<int32_t>::max)());
    CHECK(ini.getAllSection().size() == 2);
    return 0;
}
#endif

#ifndef _WIN32
/** 用通用 Unicode 转换器的实际符号检查输入与输出区间
@return 有效码点往返成功，非法码点、截断及空间不足时停在原位置
*/
static int ConversionBounds()
{
    const UTF8 smile[] = {0xf0, 0x9f, 0x98, 0x80};
    const UTF8 bad[] = {0xed, 0x20, 0x81};
    CHECK(isLegalUTF8Sequence(smile, smile + 4));
    CHECK(!isLegalUTF8Sequence(smile, smile) && !isLegalUTF8Sequence(smile, smile + 3));
    CHECK(!isLegalUTF8Sequence(bad, bad + 3));
    const UTF8* source = smile;
    UTF16 pair[2] = {};
    UTF16* out = pair;
    CHECK(ConvertUTF8toUTF16(&source, smile + 4, &out, pair + 1, strictConversion) == targetExhausted);
    CHECK(source == smile && out == pair);
    CHECK(ConvertUTF8toUTF16(&source, smile + 4, &out, pair + 2, strictConversion) == conversionOK);
    CHECK(source == smile + 4 && out == pair + 2 && pair[0] == 0xd83d && pair[1] == 0xde00);
    UTF8 bytes[4] = {};
    UTF8* target = bytes;
    const UTF16* wide = pair;
    CHECK(ConvertUTF16toUTF8(&wide, pair + 2, &target, bytes + 3, strictConversion) == targetExhausted);
    CHECK(wide == pair && target == bytes);
    CHECK(ConvertUTF16toUTF8(&wide, pair + 2, &target, bytes + 4, strictConversion) == conversionOK);
    CHECK(std::memcmp(bytes, smile, 4) == 0);
    UTF32 decoded = 0;
    UTF32* scalarOut = &decoded;
    source = smile;
    CHECK(ConvertUTF8toUTF32(&source, smile + 4, &scalarOut, &decoded + 1, strictConversion) == conversionOK);
    CHECK(source == smile + 4 && scalarOut == &decoded + 1 && decoded == 0x1f600);
    decoded = 0;
    scalarOut = &decoded;
    wide = pair;
    CHECK(ConvertUTF16toUTF32(&wide, pair + 2, &scalarOut, &decoded + 1, strictConversion) == conversionOK);
    CHECK(wide == pair + 2 && scalarOut == &decoded + 1 && decoded == 0x1f600);
    const UTF32 invalid[] = {0x110000, 'a'};
    const UTF32* scalar = invalid;
    target = bytes;
    out = pair;
    CHECK(ConvertUTF32toUTF8(&scalar, invalid + 2, &target, bytes + 4, strictConversion) == sourceIllegal);
    CHECK(scalar == invalid && target == bytes);
    CHECK(ConvertUTF32toUTF16(&scalar, invalid + 2, &out, pair + 2, strictConversion) == sourceIllegal);
    CHECK(scalar == invalid && out == pair);
    return 0;
}
#endif

/** 通过公共入口检查非法名称、值及路径不会命中已有配置
@return 文件、内存和枚举满足独立预期时返回 0
*/
static int FieldBoundaries()
{
    const std::string suffix = std::string(1, '\0') + "tail";
    const std::string section("Section");
    const std::string key("kept");
#ifdef _WIN32
    const int modes = 2;
#else
    const int modes = 1;
#endif
    for (int mode = 0; mode < modes; ++mode)
    {
        IniFile file;
        CHECK(!file.m_path.empty());
        const bool simple = mode == 0;
        Cini ini(file.m_path, simple);
        ini.writeIni(key, "original");
        std::string before;
        CHECK(ReadFixture(file.m_path, before) && !before.empty());
        ini.writeIni(key + suffix, "wrong");
        ini.writeIni(key, "wrong", section + suffix);
        ini.writeIni(key, "short" + suffix);
        ini.writeIni(key, "two\nlines");
        ini.writeIni("x=y", "bad");
        ini.writeIni("#comment", "bad");
        ini.writeIni("k", "bad", "in]valid");
        ini.writeIni("", "bad");
        ini.writeIni(" padded", "bad");
        ini.writeIni("trailing\t", "bad");
        ini.writeIni("line\nbreak", "bad");
        ini.writeIni("k", "bad", "line\rbreak");
        ini.writeIni("k", "bad", " padded");
        CHECK(ini.readIni(key) == "original");
        CHECK(ini.readIni(key + suffix).empty() && ini.readIni(key, section + suffix).empty());
        CHECK(ini.getFirstKey(section + suffix).empty() && ini.getAllKey(section + suffix).empty());
        ini.deleteKey(key + suffix);
        ini.deleteKey(key, section + suffix);
        ini.deleteSection(section + suffix);
        Cini alias(file.m_path + suffix, simple);
        CHECK(alias.readIni(key).empty() && alias.getAllSection().empty());
        CHECK(!alias.loadData("[Section]\nkept=wrong"));
        alias.writeIni(key, "wrong");
        alias.deleteSection(section);
        std::string after;
        CHECK(ReadFixture(file.m_path, after) && after == before);
        Cini snapshot(file.m_path, true);
        CHECK(snapshot.readIni(key) == "original" && snapshot.getAllSection().size() == 1);
        CHECK(snapshot.getAllKey().size() == 1 && snapshot.getFirstKey() == key);
    }
    return 0;
}

/** 检查含零字节的损坏配置无法产生可写的部分快照，显式重新载入可恢复
@return 损坏文件逐字节保留，恢复后的文件可重新读取时返回 0
*/
static int DamagedInput()
{
    const std::string bytes = std::string("[Section]\na=prefix") + '\0' + "\nb=tail\n";
    IniFile file;
    CHECK(!file.m_path.empty());
    {
        std::ofstream output(file.m_path.c_str(), std::ios::binary);
        output.write(bytes.data(), bytes.size());
        output.close();
        CHECK(output.good());
    }
    Cini ini(file.m_path, true);
    CHECK(ini.readIni("a").empty() && ini.getAllSection().empty());
    ini.writeIni("a", "wrong");
    ini.deleteSection();
    std::string actual;
    CHECK(ReadFixture(file.m_path, actual) && actual == bytes);
    CHECK(ini.loadData("[Section]\na=ready") && !ini.loadData(bytes));
    CHECK(ini.readIni("a").empty() && ini.getAllKey().empty());
    CHECK(ini.loadData("[Section]\na=ready") && ini.readIni("a") == "ready");
    ini.writeIni("a", "saved");
    CHECK(Cini(file.m_path, true).readIni("a") == "saved");
    return 0;
}

/** 检查全局键、UTF-8 名称与返回副本的生命周期
@return 重开后各键仍属原分区，销毁实例后返回副本仍有效时返回 0
*/
static int GlobalSectionAndCopies()
{
    IniFile file;
    CHECK(!file.m_path.empty());
    const std::string section("\xe8\x8a\x82=1");
    const std::string key("\xe9\x94\xae]");
    std::string saved;
    std::vector<std::string> keys;
    std::vector<std::string> sections;
    {
        Cini ini(file.m_path, true);
        ini.writeIni(key, "  padded  ", section);
        ini.writeIni(key, "global", "");
        CHECK(ini.readIni(key, section) == "  padded  " && ini.readIni(key).empty());
        Cini reopened(file.m_path, true);
        CHECK(reopened.readIni(key, "") == "global");
        saved = reopened.readIni(key, section);
        keys = reopened.getAllKey("");
        sections = reopened.getAllSection();
        reopened.deleteSection("");
        Cini afterDelete(file.m_path, true);
        CHECK(afterDelete.getAllKey("").empty() && afterDelete.readIni(key, section) == "padded");
    }
    std::sort(sections.begin(), sections.end());
    CHECK(sections.size() == 2 && sections[0].empty() && sections[1] == section);
    CHECK(saved == "padded" && keys.size() == 1 && keys.front() == key);
    return 0;
}

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
    bool valid = true;
    // 每批解析 count 次，以实际成功次数校验整批
    LibraryBenchmark::run("ini.loadData.64", [&](uint32_t count) -> uint64_t
    {
        uint64_t successes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            successes += ini.loadData(input);
        }
        if (successes != count)
        {
            valid = false;
        }
        return successes;
    });
    const std::string key("k63");
    const std::string section("Section");
    CHECK(valid && ini.getAllKey().size() == 64);
    for (uint32_t i = 0; i < 64; ++i)
    {
        CHECK(ini.readIni("k" + std::to_string(i)) == "payload");
    }
    // 每批查询 count 次，累计返回字节供计时器消费
    LibraryBenchmark::run("ini.readIni.64", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            bytes += ini.readIni(key, section).size();
        }
        if (bytes != uint64_t(count) * 7)
        {
            valid = false;
        }
        return bytes;
    });
    CHECK(valid);
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
    const LibraryTestCase cases[] = {
        {kReadMemory, false, ReadMemory},
        {kFileChanges, false, FileChanges},
#ifdef _WIN32
        {kProfileFile, false, ProfileFile},
#endif
        {kParserBoundaries, false, ParserBoundaries},
        {kFileLifecycle, false, FileLifecycle},
#ifdef _WIN32
        {kProfileLimits, false, ProfileLimits},
#else
        {kConversionBounds, false, ConversionBounds},
#endif
        {kFieldBoundaries, false, FieldBoundaries},
        {kDamagedInput, false, DamagedInput},
        {kGlobalSectionAndCopies, false, GlobalSectionAndCopies},
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}