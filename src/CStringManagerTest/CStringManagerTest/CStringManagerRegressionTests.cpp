#include "CStringManagerTest.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstdio>
#include <new>
#include <clocale>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#endif
#if defined(_WIN32) && defined(_DEBUG)
// 当前串行回归允许的普通堆分配次数，负值时不拒绝。
static int32_t s_conversionRejectAfter = -1;
// 当前调用观察到的普通堆分配总次数。
static uint32_t s_conversionAllocations = 0;
// 当前调用中申请至少 256 字节的数据缓冲区次数。
static uint32_t s_conversionPayloadAllocations = 0;
/** 对编码分配逐点持续拒绝，验证异常收敛和结果缓冲的唯一所有权
@param [in] operation CRT 分配操作
@param [in] size 本次申请的字节数
@param [in] block CRT 内存块类型，其余未命名参数按回调原型忽略
@return 非零允许分配，零拒绝
*/
static int __cdecl ConversionMemoryHook(int operation, void*, size_t size, int block, long, const unsigned char*, int)
{
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        ++s_conversionAllocations;
        if (size >= 256)
        {
            ++s_conversionPayloadAllocations;
        }
        if (s_conversionRejectAfter == 0)
        {
            return 0;
        }
        if (s_conversionRejectAfter > 0)
        {
            --s_conversionRejectAfter;
        }
    }
    return 1;
}
#endif

int TestReplaceAll()
{
    std::string value(4096, 'a');
    value[8] = '\0';
    const std::string expected(value);
    const std::string pattern("a\0a", 3);
    const std::string samePattern(pattern);
#if defined(_WIN32) && defined(_DEBUG)
    s_conversionRejectAfter = -1;
    s_conversionAllocations = 0;
    const _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(ConversionMemoryHook);
#endif
    bool completed = false;
    try
    {
        CStringManager::ReplaceAll(value, value, value);
        CStringManager::ReplaceAll(value, expected, value);
        CStringManager::ReplaceAll(value, pattern, samePattern);
        completed = true;
    }
    catch (...)
    {
        std::fputs("ReplaceAll threw for identical text\n", stderr);
    }
#if defined(_WIN32) && defined(_DEBUG)
    _CrtSetAllocHook(previous);
    if (s_conversionAllocations != 0)
    {
        std::printf("ReplaceAll unchanged text: %u unexpected heap allocations\n", s_conversionAllocations);
        return 1;
    }
#endif
    if (!completed || value != expected)
    {
        std::fputs("ReplaceAll changed identical binary text\n", stderr);
        return 1;
    }
    std::string equalLength("aaab");
    CStringManager::ReplaceAll(equalLength, "ab", "ba");
    std::string binary("a\0a\0", 4);
    CStringManager::ReplaceAll(binary, std::string("a\0", 2), std::string("\0xy", 3));
    std::string aliased("aba");
    CStringManager::ReplaceAll(aliased, "a", aliased);
    CStringManager::ReplaceAll(aliased, "abababab", aliased);
    if (equalLength != "baaa" || binary != std::string("\0xy\0xy", 6) || aliased != "abababa")
    {
        std::fputs("ReplaceAll boundary or binary expansion mismatch\n", stderr);
        return 1;
    }
    return 0;
}

/** 检查六种编码的内容、内嵌零、系统替代规则和低内存收敛
@return 全部通过返回true
*/
static bool CStringConversionAudit()
{
    uint32_t checks = 0;
#define CONVERSION_CHECK(value) do { ++checks; if (!(value)) { std::printf("CString conversion line=%d FAIL\n", __LINE__); return false; } } while (0)
    const std::string narrow(256, 'a');
    const std::wstring wide(256, L'a');
    const std::string zeroBytes("abc\0def", 7);
    const std::wstring zeroWide(L"abc\0def", 7);
    CONVERSION_CHECK(CStringManager::Utf8ToUnicode(narrow) == wide);
    CONVERSION_CHECK(CStringManager::UnicodeToUtf8(wide) == narrow);
    CONVERSION_CHECK(CStringManager::Utf8ToUnicode("").empty());
    CONVERSION_CHECK(CStringManager::UnicodeToUtf8(L"").empty());
    CONVERSION_CHECK(CStringManager::Utf8ToUnicode(zeroBytes) == zeroWide);
    CONVERSION_CHECK(CStringManager::UnicodeToUtf8(zeroWide) == zeroBytes);
#if CSTRING_MANAGER_HAS_ANSI
    bool ansiAvailable = true;
#ifndef _WIN32
    try
    {
        const std::locale gbk(std::locale::classic(), "zh_CN.GBK", std::locale::ctype);
        CONVERSION_CHECK(std::has_facet<std::ctype<char>>(gbk));
    }
    catch (const std::runtime_error&)
    {
        ansiAvailable = false;
    }
    const std::string gbkText("\xd6\xd0\xce\xc4", 4);
    const std::wstring chinese(L"\u4e2d\u6587");
    CONVERSION_CHECK(CStringManager::AnsiToUnicode(gbkText) == (ansiAvailable ? chinese : std::wstring()));
    CONVERSION_CHECK(CStringManager::UnicodeToAnsi(chinese) == (ansiAvailable ? gbkText : std::string()));
#endif
    CONVERSION_CHECK(CStringManager::AnsiToUnicode(narrow) == (ansiAvailable ? wide : std::wstring()));
    CONVERSION_CHECK(CStringManager::UnicodeToAnsi(wide) == (ansiAvailable ? narrow : std::string()));
    CONVERSION_CHECK(CStringManager::AnsiToUtf8(narrow) == (ansiAvailable ? narrow : std::string()));
    CONVERSION_CHECK(CStringManager::Utf8ToAnsi(narrow) == (ansiAvailable ? narrow : std::string()));
    CONVERSION_CHECK(CStringManager::AnsiToUnicode("").empty());
    CONVERSION_CHECK(CStringManager::UnicodeToAnsi(L"").empty());
    CONVERSION_CHECK(CStringManager::AnsiToUtf8("").empty());
    CONVERSION_CHECK(CStringManager::Utf8ToAnsi("").empty());
    CONVERSION_CHECK(CStringManager::AnsiToUnicode(zeroBytes) == (ansiAvailable ? zeroWide : std::wstring()));
    CONVERSION_CHECK(CStringManager::UnicodeToAnsi(zeroWide) == (ansiAvailable ? zeroBytes : std::string()));
    CONVERSION_CHECK(CStringManager::AnsiToUtf8(zeroBytes) == (ansiAvailable ? zeroBytes : std::string()));
    CONVERSION_CHECK(CStringManager::Utf8ToAnsi(zeroBytes) == (ansiAvailable ? zeroBytes : std::string()));
#endif
#ifdef _WIN32
    const wchar_t unicodeData[] = { L'A', 0, 0x4e2d, 0xd83d, 0xde00, L'Z' };
    const std::wstring unicode(unicodeData, 6);
    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        const UINT codePage = mode == 0 ? CP_UTF8 : CP_ACP;
        const int bytes = WideCharToMultiByte(codePage, 0, unicode.data(), 6, nullptr, 0, nullptr, nullptr);
        CONVERSION_CHECK(bytes > 0);
        std::string expected(static_cast<size_t>(bytes), '\0');
        CONVERSION_CHECK(WideCharToMultiByte(codePage, 0, unicode.data(), 6, &expected[0], bytes, nullptr, nullptr) == bytes);
        const std::string actual = mode == 0 ? CStringManager::UnicodeToUtf8(unicode) : CStringManager::UnicodeToAnsi(unicode);
        CONVERSION_CHECK(actual == expected);
        const int characters = MultiByteToWideChar(codePage, 0, expected.data(), bytes, nullptr, 0);
        CONVERSION_CHECK(characters > 0);
        std::wstring back(static_cast<size_t>(characters), L'\0');
        CONVERSION_CHECK(MultiByteToWideChar(codePage, 0, expected.data(), bytes, &back[0], characters) == characters);
        CONVERSION_CHECK((mode == 0 ? CStringManager::Utf8ToUnicode(expected) : CStringManager::AnsiToUnicode(expected)) == back);
        if (mode == 0)
        {
            CONVERSION_CHECK(back == unicode);
        }
    }
    // flags=0沿用系统替代规则；修复所有权时不暗中改成严格拒绝输入。
    const std::string invalid("\xff\xc0\x80\0Z", 5);
    wchar_t expectedInvalid[16] = {};
    const int invalidCount = MultiByteToWideChar(CP_UTF8, 0, invalid.data(), 5, expectedInvalid, 16);
    CONVERSION_CHECK(invalidCount > 0);
    CONVERSION_CHECK(CStringManager::Utf8ToUnicode(invalid) == std::wstring(expectedInvalid, static_cast<size_t>(invalidCount)));
#endif
#if defined(_WIN32) && defined(_DEBUG)
    uint32_t rejected = 0;
    uint32_t accepted = 0;
    uint32_t allocatorExceptions = 0;
    for (uint32_t mode = 0; mode < 6; ++mode)
    {
        for (int32_t position = 0; position < 8; ++position)
        {
            _CrtMemState before, after;
            _CrtMemCheckpoint(&before);
            bool escaped = false;
            bool allocationFailed = false;
            bool full = false;
            bool empty = false;
            s_conversionRejectAfter = position;
            s_conversionAllocations = 0;
            s_conversionPayloadAllocations = 0;
            const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(ConversionMemoryHook);
            try
            {
                if (mode == 0 || mode == 2)
                {
                    std::wstring result;
                    if (mode == 0)
                    {
                        result = CStringManager::Utf8ToUnicode(narrow);
                    }
                    else
                    {
                        result = CStringManager::AnsiToUnicode(narrow);
                    }
                    full = result == wide;
                    empty = result.empty();
                }
                else
                {
                    std::string result;
                    if (mode == 1)
                    {
                        result = CStringManager::UnicodeToUtf8(wide);
                    }
                    else if (mode == 3)
                    {
                        result = CStringManager::UnicodeToAnsi(wide);
                    }
                    else if (mode == 4)
                    {
                        result = CStringManager::AnsiToUtf8(narrow);
                    }
                    else
                    {
                        result = CStringManager::Utf8ToAnsi(narrow);
                    }
                    full = result == narrow;
                    empty = result.empty();
                }
            }
            catch (const std::bad_alloc&)
            {
                // VS2013 Debug连空字符串的迭代器代理都需要分配；边界捕获此分配异常。
                allocationFailed = true;
            }
            catch (...)
            {
                escaped = true;
            }
            _CrtSetAllocHook(old);
            s_conversionRejectAfter = -1;
            _CrtMemCheckpoint(&after);
            if (escaped || (!full && !empty && !allocationFailed))
            {
                std::printf("CSTRING_FAULT mode=%u position=%d escaped=%u allocations=%u\n", mode, position, escaped ? 1U : 0U, s_conversionAllocations);
            }
            const bool faultInjected = s_conversionAllocations > static_cast<uint32_t>(position);
            CONVERSION_CHECK(!escaped && (full || (faultInjected && (empty || allocationFailed))));
            if (before.lCounts[_NORMAL_BLOCK] != after.lCounts[_NORMAL_BLOCK] || before.lSizes[_NORMAL_BLOCK] != after.lSizes[_NORMAL_BLOCK])
            {
                std::printf("CSTRING_HEAP mode=%u position=%d blocks=%lld bytes=%lld allocation_failed=%u\n", mode, position,
                    static_cast<long long>(after.lCounts[_NORMAL_BLOCK]) - static_cast<long long>(before.lCounts[_NORMAL_BLOCK]),
                    static_cast<long long>(after.lSizes[_NORMAL_BLOCK]) - static_cast<long long>(before.lSizes[_NORMAL_BLOCK]), allocationFailed ? 1U : 0U);
            }
            CONVERSION_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
            if (full)
            {
                ++accepted;
                CONVERSION_CHECK(s_conversionPayloadAllocations == (mode < 4 ? 1U : 2U));
            }
            else
            {
                ++rejected;
                allocatorExceptions += allocationFailed ? 1U : 0U;
            }
        }
    }
    CONVERSION_CHECK(accepted > 0 && rejected >= 8);
    std::printf("CSTRING_CONVERSION allocation_positions=48 accepted=%u rejected=%u allocator_exceptions=%u heap_delta=0 direct_payload_allocations=1\n", accepted, rejected, allocatorExceptions);
#endif
    std::printf("CString conversion checks=%u PASS\n", checks);
    return true;
#undef CONVERSION_CHECK
}

#if defined(_WIN32) && defined(_DEBUG)
// 本次格式化允许成功的普通堆分配次数。
static uint32_t s_formatRemaining = 0;
// 当前故障位置拒绝的分配次数。
static uint32_t s_formatRejected = 0;
/** 从指定普通堆分配开始持续拒绝，其他参数遵循CRT原型
@param [in] operation CRT 分配操作
@param [in] block CRT 内存块类型，其余未命名参数按回调原型忽略
@return 是否允许
*/
static int __cdecl FormatMemoryHook(int operation, void*, size_t, int block, long, const unsigned char*, int)
{
    if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
    {
        if (s_formatRemaining == 0)
        {
            ++s_formatRejected;
            return 0;
        }
        --s_formatRemaining;
    }
    return 1;
}
#endif

/** 验证四个Format重载的独立预期、输入别名、宽文本和分配失败
@return 全部检查通过为true
*/
static bool CStringFormatAudit()
{
    uint32_t checks = 0;
#define FORMAT_CHECK(value) { ++checks; if (!(value)) { std::printf("CString format line=%d FAIL\n", __LINE__); return false; } }
    FORMAT_CHECK(CStringManager::Format("%s:%04d:%.2f", "value", 7, 1.25) == "value:0007:1.25");
    FORMAT_CHECK(CStringManager::Format(L"%ls:%04d:%.2f", L"value", 7, 1.25) == L"value:0007:1.25");
    FORMAT_CHECK(CStringManager::Format(static_cast<const char*>(nullptr)).empty());
    FORMAT_CHECK(CStringManager::Format(static_cast<const wchar_t*>(nullptr)).empty());
    FORMAT_CHECK(CStringManager::Format("").empty() && CStringManager::Format(L"").empty());
    FORMAT_CHECK(CStringManager::Format(L"\u4e2d:%ls:%lc", L"\u6587", L'\u56fd') == L"\u4e2d:\u6587:\u56fd");
    FORMAT_CHECK(CStringManager::Format("a%cb", 0) == std::string("a\0b", 3));
    FORMAT_CHECK(CStringManager::Format(L"a%lcb", static_cast<wint_t>(0)) == std::wstring(L"a\0b", 3));
    const uint32_t sizes[] = {1, 15, 16, 255, 256, 257, 4096, 8192};
    for (uint32_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
    {
        const uint32_t size = sizes[index];
        const std::string expected = std::string(size, 'x') + std::string(4095, ' ') + "7";
        const std::wstring wideExpected = std::wstring(size, L'x') + std::wstring(4095, L' ') + L"7";
        std::string value(size, 'x');
        CStringManager::Format(value, "%s%4096d", value.c_str(), 7);
        FORMAT_CHECK(value == expected);
        std::wstring wide(size, L'x');
        CStringManager::Format(wide, L"%ls%4096d", wide.c_str(), 7);
        FORMAT_CHECK(wide == wideExpected);
        FORMAT_CHECK(CStringManager::Format("%0*d", static_cast<int>(size), 7) == std::string(size - 1, '0') + "7");
        FORMAT_CHECK(CStringManager::Format(L"%0*d", static_cast<int>(size), 7) == std::wstring(size - 1, L'0') + L"7");
    }
    std::string format = "%s%04096d";
    CStringManager::Format(format, format.c_str(), "a", 7);
    FORMAT_CHECK(format == "a" + std::string(4095, '0') + "7");
    std::wstring wideFormat = L"%ls%04096d";
    CStringManager::Format(wideFormat, wideFormat.c_str(), L"a", 7);
    FORMAT_CHECK(wideFormat == L"a" + std::wstring(4095, L'0') + L"7");
    CStringManager::Format(format, static_cast<const char*>(nullptr));
    CStringManager::Format(wideFormat, static_cast<const wchar_t*>(nullptr));
    FORMAT_CHECK(format.empty() && wideFormat.empty());
#if defined(_WIN32) && defined(_DEBUG)
    uint32_t rejected = 0;
    uint32_t accepted = 0;
    const std::string expected = std::string(4095, '0') + "7";
    const std::wstring wideExpected = std::wstring(4095, L'0') + L"7";
    for (uint32_t mode = 0; mode < 4; ++mode)
    {
        for (uint32_t position = 0; position < 8; ++position)
        {
            _CrtMemState before = {};
            _CrtMemCheckpoint(&before);
            bool valid = false;
            bool escaped = false;
            bool unexpected = false;
            {
                // 引用重载必须在失败时清空已有结果。
                std::string output(mode == 0 ? "previous" : "");
                std::wstring wideOutput(mode == 1 ? L"previous" : L"");
                s_formatRemaining = position;
                s_formatRejected = 0;
                const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(FormatMemoryHook);
                try
                {
                    if (mode == 0) { CStringManager::Format(output, "%04096d", 7); }
                    else if (mode == 1) { CStringManager::Format(wideOutput, L"%04096d", 7); }
                    else if (mode == 2) { output = CStringManager::Format("%04096d", 7); }
                    else { wideOutput = CStringManager::Format(L"%04096d", 7); }
                }
                catch (const std::bad_alloc&)
                {
                    escaped = true;
                }
                catch (...)
                {
                    unexpected = true;
                }
                _CrtSetAllocHook(old);
                const bool full = mode == 0 || mode == 2 ? output == expected : wideOutput == wideExpected;
                const bool empty = output.empty() && wideOutput.empty();
                valid = full || (s_formatRejected != 0 && empty);
                if (s_formatRejected != 0) { ++rejected; }
                else { ++accepted; }
            }
            _CrtMemState after = {};
            _CrtMemCheckpoint(&after);
            FORMAT_CHECK(valid && !unexpected && (!escaped || (mode >= 2 && s_formatRejected != 0)));
            FORMAT_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
        }
    }
    FORMAT_CHECK(rejected != 0 && accepted != 0);
    std::printf("CSTRING_FORMAT allocation_positions=32 rejected=%u accepted=%u heap_delta=0\n", rejected, accepted);
#endif
    std::printf("CString format alias/wide/variadic checks=%u PASS\n", checks);
    return true;
#undef FORMAT_CHECK
}

/** 验证编码转换与格式化，Windows 调试版另外逐位置拒绝普通堆分配
@return 每一条已执行的输入输出及资源约束均通过时返回 true
*/
bool RunCStringManagerRegressionTests()
{
#ifndef _WIN32
    // 宽字符格式化需要 UTF-8 分类规则；测试结束后恢复宿主的原设置。
    const char* previous = std::setlocale(LC_CTYPE, nullptr);
    const std::string saved = previous == nullptr ? "C" : previous;
    if (std::setlocale(LC_CTYPE, "C.UTF-8") == nullptr &&
        std::setlocale(LC_CTYPE, "en_US.UTF-8") == nullptr &&
        std::setlocale(LC_CTYPE, "UTF-8") == nullptr)
    {
        std::fprintf(stderr, "No UTF-8 locale available for CStringManager test\n");
        return false;
    }
#endif
    bool passed = false;
    try
    {
        const bool conversion = CStringConversionAudit();
        const bool format = CStringFormatAudit();
        passed = conversion && format;
    }
    catch (...)
    {
        std::fprintf(stderr, "Unexpected CStringManager regression exception\n");
    }
#ifndef _WIN32
    if (std::setlocale(LC_CTYPE, saved.c_str()) == nullptr)
    {
        return false;
    }
#endif
    return passed;
}