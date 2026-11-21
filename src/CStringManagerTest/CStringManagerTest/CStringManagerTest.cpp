#include "PlatformTest/PlatformTestAPI.h"
#include "CStringManagerTest.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstring>
#include <clocale>
#include <cctype>
#include <atomic>
#include <functional>
#include <iostream>
#include <thread>
#include <limits>
#include <mutex>
#include <cwchar>
#include <climits>
#include <cstdio>
#include <exception>

/** 核对完整转换结果，启用计时时计算每批的长度与轮换字节校验值
@param [in] name 性能记录名
@param [in] input 固定输入
@param [in] expected 独立生成的非空预期输出
@param [in] measure 为 true 时启用计时
@return 输出匹配且每次采样长度有效时返回 true
*/
template<std::string (*Transform)(const std::string&)>
static bool CheckByteConversion(const char* name, const std::string& input, const std::string& expected, bool measure)
{
    if (expected.empty() || Transform(input) != expected)
    {
        std::fprintf(stderr, "%s: unexpected output\n", name);
        return false;
    }
    bool valid = true;
    if (measure)
    {
        LibraryBenchmark::run(name, [&](uint32_t count) -> uint64_t
        {
            uint64_t sum = 0;
            for (uint32_t i = 0; i < count; ++i)
            {
                const std::string value = Transform(input);
                if (value.size() != expected.size())
                {
                    valid = false;
                    return 0;
                }
                sum += value.size() + static_cast<unsigned char>(value[i % value.size()]);
            }
            return sum;
        });
    }
    if (!valid)
    {
        std::fprintf(stderr, "%s: output length changed during sampling; discard result\n", name);
    }
    return valid;
}

/** 核对固定负载的独立预期，并按调用入口选择是否计时
@param [in] measure 为 true 时测量各接口，为 false 时仅检查功能
@return 输入和结果满足各用例约定时返回零
*/
static int RunBenchmarkScenarios(bool measure)
{
    const std::string pattern = "abcdefghijklmno! +%";
    std::string input;
    while (input.size() < 4096)
    {
        input.append(pattern);
    }
    input.resize(4096);
    std::string expectedUrl;
    for (size_t i = 0; i < input.size(); ++i)
    {
        switch (input[i])
        {
        case '!': expectedUrl += "%21"; break;
        case ' ': expectedUrl += "%20"; break;
        case '+': expectedUrl += "%2B"; break;
        case '%': expectedUrl += "%25"; break;
        default: expectedUrl += input[i]; break;
        }
    }
    if (!CheckByteConversion<CStringManager::UrlEncode>("CStringManager.UrlEncode.4KiB", input, expectedUrl, measure) ||
        !CheckByteConversion<CStringManager::UrlDecode>("CStringManager.UrlDecode.4KiB-output", expectedUrl, input, measure))
    {
        return 1;
    }
    std::string original;
    std::string expectedReplacement;
    for (size_t i = 0; i < 1024; ++i)
    {
        original.append("xa");
        expectedReplacement.append("ya");
    }
    std::string check(original);
    CStringManager::ReplaceAll(check, "x", "y");
    if (check != expectedReplacement)
    {
        return 1;
    }
    const std::string from("x"), to("y");
    CStringManager::ReplaceAll(check, to, from);
    if (check != original)
    {
        return 1;
    }
    // 每轮先替换再还原，同一块已分配的文本可供下一轮复用。抽样位置始终是被替换的字节。
    bool validReplacement = true;
    const auto roundTrips = [&](uint32_t count) -> uint64_t
    {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const size_t offset = 2 * (i % 1024);
            CStringManager::ReplaceAll(check, from, to);
            if (check.size() != original.size())
            {
                validReplacement = false;
                return 0;
            }
            sum += static_cast<unsigned char>(check[offset]);
            CStringManager::ReplaceAll(check, to, from);
            if (check.size() != original.size())
            {
                validReplacement = false;
                return 0;
            }
            sum += static_cast<unsigned char>(check[offset]);
        }
        return sum;
    };
    if (roundTrips(1) != static_cast<unsigned>('y' + 'x') || check != original)
    {
        return 1;
    }
    if (measure)
    {
        LibraryBenchmark::run("CStringManager.ReplaceAll.1024-matches", [&](uint32_t count) -> uint64_t {
            uint64_t sum = 0;
            for (uint32_t i = 0; i < count; ++i)
            {
                std::string value(original);
                CStringManager::ReplaceAll(value, "x", "y");
                if (value.size() != original.size())
                {
                    validReplacement = false;
                    return 0;
                }
                sum += value.size() + static_cast<unsigned char>(value[i % value.size()]);
            }
            return sum;
        });
        LibraryBenchmark::run("CStringManager.ReplaceAll.in-place.1024-matches", roundTrips, 2);
        if (!validReplacement || check != original)
        {
            std::fputs("ReplaceAll: invalid output during sampling; discard result\n", stderr);
            return 1;
        }
    }
    const std::string block("A/:\0\xff ~?", 8);
    const std::string pathBlock("A/:\0%FF%20~%3F", 14);
    std::string binary;
    std::string hex;
    std::string path;
    for (size_t i = 0; i < 512; ++i)
    {
        binary += block;
        hex += "412f3a00ff207e3f";
        path += pathBlock;
    }
    if (!CheckByteConversion<CStringManager::EncodeUrlPath>("CStringManager.EncodeUrlPath.4KiB", binary, path, measure) ||
        !CheckByteConversion<CStringManager::StrToHex>("CStringManager.StrToHex.4KiB", binary, hex, measure) ||
        !CheckByteConversion<CStringManager::HexToStr>("CStringManager.HexToStr.8KiB", hex, binary, measure))
    {
        return 1;
    }
    return 0;
}

/** 核对性能场景使用的固定输入、预期字节和调用次数，全程不计时
@return 所有负载的功能检查成功时返回零
*/
static int TestBenchmarkInputs()
{
    return RunBenchmarkScenarios(false);
}

std::string CStringManagerTestGrouping::do_grouping() const
{
    return std::string(1, static_cast<char>(3));
}

/** 核对分隔、替换、编辑、格式化及数值边界
@return 全部断言通过时返回0
*/
static int TestStrings()
{
    int32_t totalCount = 0;
    int32_t failCount = 0;
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
        if (!ok)
        {
            ++failCount;
        }
    };

    check(CStringManager::FindOther("(a(b)c)", '(', ')', 0) == 6, "CStringManager FindOther left select");
    check(CStringManager::FindOther("(a(b)c)", '(', ')', 4) == 2, "CStringManager FindOther right select");
    check(CStringManager::FindOther("(abc", '(', ')', 0) == std::string::npos, "CStringManager FindOther missing right");
    const std::string symmetric("|one|two||tail|");
    check(CStringManager::FindOther(symmetric, '|', '|', 0) == 4 &&
        CStringManager::FindOther(symmetric, '|', '|', 4) == 0, "symmetric first pair in both directions");
    check(CStringManager::FindOther(symmetric, '|', '|', 8) == 9 &&
        CStringManager::FindOther(symmetric, '|', '|', 9) == 8, "adjacent symmetric delimiters");
    check(CStringManager::FindOther(symmetric, '|', '|', 14) == std::string::npos &&
        CStringManager::FindOther(symmetric, '|', '|', 1) == std::string::npos &&
        CStringManager::FindOther(symmetric, '|', '|', std::string::npos) == std::string::npos, "unpaired and invalid selection");
    check(CStringManager::FindOther(")(())(", '(', ')', 1) == 4 &&
        CStringManager::FindOther(")(())(", '(', ')', 4) == 1 &&
        CStringManager::FindOther(")(())(", '(', ')', 0) == std::string::npos, "nested pair with unmatched neighbors");
    std::vector<std::string> splitResult = CStringManager::split("a,,b", ",");
    check(splitResult.size() == 3 && splitResult[0] == "a" && splitResult[1].empty() && splitResult[2] == "b", "CStringManager split string");
    std::vector<std::string> emptySplitResult = CStringManager::split("abc", "");
    check(emptySplitResult.size() == 1 && emptySplitResult[0] == "abc", "CStringManager split empty separator");
    std::vector<std::string> multiSplitResult = CStringManager::split("aa<>bb<><>", "<>");
    check(multiSplitResult.size() == 4 && multiSplitResult[0] == "aa" && multiSplitResult[1] == "bb" &&
        multiSplitResult[2].empty() && multiSplitResult[3].empty(), "CStringManager split multi separator");
    std::vector<std::wstring> wideSplitResult = CStringManager::split(L"a|b|", L"|");
    check(wideSplitResult.size() == 3 && wideSplitResult[0] == L"a" && wideSplitResult[1] == L"b" &&
        wideSplitResult[2].empty(), "CStringManager split wide string");
    std::vector<std::string> splitCharResult;
    CStringManager::split(splitCharResult, "a,b,", ',');
    check(splitCharResult.size() == 3 && splitCharResult[0] == "a" &&
        splitCharResult[1] == "b" && splitCharResult[2].empty(), "CStringManager split char");
    std::vector<std::string> aliased(1, "alpha,beta,gamma");
    CStringManager::split(aliased, aliased[0], ',');
    check(aliased.size() == 3 && aliased[0] == "alpha" && aliased[1] == "beta" && aliased[2] == "gamma", "split from its own output");
    std::string duplicated("aba");
    std::wstring wideDuplicated(L"aba");
    check(CStringManager::Replace(duplicated, "a", duplicated) == 2 && duplicated == "abababa", "replacement aliases output");
    check(CStringManager::Replace(wideDuplicated, L"a", wideDuplicated) == 2 && wideDuplicated == L"abababa", "wide replacement aliases output");
    check(CStringManager::Replace(duplicated, duplicated, "x") == 1 && duplicated == "x" &&
        CStringManager::Replace(duplicated, duplicated, duplicated) == 1 && duplicated == "x",
        "whole narrow pattern aliases target");
    check(CStringManager::Replace(wideDuplicated, wideDuplicated, L"") == 1 && wideDuplicated.empty() &&
        CStringManager::Replace(wideDuplicated, wideDuplicated, L"x") == 0 && wideDuplicated.empty(),
        "whole wide pattern and empty pattern alias target");
    std::string overlap("abcde");
    check(CStringManager::CopyToBuffer(overlap, &overlap[1], 4) == 6 && overlap == std::string("aabc\0", 5), "bounded overlapping copy");
    std::string replaceText = "aa-bb-aa";
    check(CStringManager::Replace(replaceText, "aa", "cc") == 2 && replaceText == "cc-bb-cc", "CStringManager Replace string");
    CStringManager::ReplaceAll(replaceText, "c", "d");
    check(replaceText == "dd-bb-dd", "CStringManager ReplaceAll string");
    std::string replaceEmptyOldText = "abc";
    CStringManager::ReplaceAll(replaceEmptyOldText, "", "x");
    check(replaceEmptyOldText == "abc", "CStringManager ReplaceAll empty old value");
    std::string replaceShrinkText = "aaaa";
    CStringManager::ReplaceAll(replaceShrinkText, "aa", "a");
    check(replaceShrinkText == "a", "CStringManager ReplaceAll shrink loop");
    std::string replaceGrowText = "a";
    CStringManager::ReplaceAll(replaceGrowText, "a", "aa");
    check(replaceGrowText == "aa", "CStringManager ReplaceAll contains old value");
    std::string expanding("aaab");
    CStringManager::ReplaceAll(expanding, "ab", "bbaa");
    check(expanding == "aabbaa", "expansion across a replacement boundary terminates");
    std::string boundary = "cabbbd";
    CStringManager::ReplaceAll(boundary, "ab", "a");
    check(boundary == "cad", "ReplaceAll rechecks left boundary after shrinking");
    std::string alias = "ab";
    CStringManager::ReplaceAll(alias, alias, "token");
    check(alias == "token", "ReplaceAll old token aliases input");
    check(CStringManager::Mid("abcdef", 2, 3) == "cde", "CStringManager Mid");
    check(CStringManager::Mid("abc", 3, 1).empty(), "CStringManager Mid offset at end");
    check(CStringManager::Left("abcdef", 2) == "ab", "CStringManager Left");
    check(CStringManager::Right("abcdef", 2) == "ef", "CStringManager Right");
    check(CStringManager::Right("abc", 99) == "abc", "CStringManager Right oversize");
    check(CStringManager::Find("abcdef", "cd") == 2, "CStringManager Find string");
    check(CStringManager::ReserveFind("ababa", "ba") == 3, "CStringManager ReserveFind string");
    std::string editText = "abc";
    CStringManager::Insert(editText, 1, "XX");
    CStringManager::Delete(editText, 2, 1);
    check(editText == "aXbc", "CStringManager Insert Delete");
    CStringManager::Insert(editText, 999, "Z");
    check(editText == "aXbcZ", "CStringManager Insert oversize offset");
    check(CStringManager::Delete(editText, 999, 2) == 5 && editText == "aXbcZ", "CStringManager Delete oversize offset");
    check(CStringManager::Format("%s-%d", "value", 7) == "value-7", "CStringManager Format string");
    check(CStringManager::Format("") == "", "CStringManager Format empty string");
    check(CStringManager::Format(static_cast<const char*>(nullptr)) == "", "CStringManager Format null string");
    std::string nullFormatText = "old";
    CStringManager::Format(nullFormatText, static_cast<const char*>(nullptr));
    check(nullFormatText.empty(), "CStringManager Format output null string");
    check(CStringManager::Format(L"%ls-%d", L"value", 7) == L"value-7", "CStringManager Format wide string");
    std::string reverseText = "abc";
    CStringManager::MakeReverse(reverseText);
    check(reverseText == "cba", "CStringManager MakeReverse");
    check(CStringManager::MakeUpper("aBc") == "ABC", "CStringManager MakeUpper");
    check(CStringManager::MakeLower("aBc") == "abc", "CStringManager MakeLower");
    std::string highByteText;
    highByteText.push_back('\xff');
    const unsigned char highByte = static_cast<unsigned char>(highByteText[0]);
    check(CStringManager::MakeUpper(highByteText) == std::string(1, static_cast<char>(std::toupper(highByte))) &&
        CStringManager::MakeLower(highByteText) == std::string(1, static_cast<char>(std::tolower(highByte))),
        "CStringManager MakeUpper MakeLower high byte");
    check(CStringManager::GetMidString("left[value]right", "[", "]") == "value", "CStringManager GetMidString");
    check(CStringManager::GetMidString("left[[value]]right", "[[", "]]") == "value", "CStringManager GetMidString multi tag");
    check(CStringManager::GetMidString("left[value", "[", "]").empty(), "CStringManager GetMidString missing right");
    check(CStringManager::atoi64(nullptr) == 0, "CStringManager atoi64 null");
    check(CStringManager::atoi64("-12345678901") == -12345678901LL, "CStringManager atoi64");
    check(CStringManager::atoui64(nullptr) == 0, "CStringManager atoui64 null");
    check(CStringManager::atoui64("12345678901") == 12345678901ULL, "CStringManager atoui64");
    check(CStringManager::toStringInt32(-123) == "-123", "CStringManager toStringInt32");
    check(CStringManager::toStringInt64(-12345678901LL) == "-12345678901", "CStringManager toStringInt64");
    std::string urlEncoded = CStringManager::UrlEncode("a b+c");
    check(CStringManager::UrlDecode(urlEncoded) == "a b+c", "CStringManager UrlEncode UrlDecode");
    check(CStringManager::UrlDecode("%") == "%" && CStringManager::UrlDecode("%GZ") == "%GZ", "CStringManager UrlDecode invalid percent");
    check(CStringManager::UrlDecode("%2f%2F") == "//", "CStringManager UrlDecode lower upper hex");
    check(CStringManager::UrlDecode("a+b%2Bc%20") == "a b+c ", "URL literal plus and escaped plus");
    std::wstring unicodeText = CStringManager::Utf8ToUnicode("abc");
    check(CStringManager::UnicodeToUtf8(unicodeText) == "abc", "CStringManager Utf8 Unicode");
    std::cout << "CStringManager test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << std::endl;
    return failCount == 0 ? 0 : 1;
}

/** 用独立字节表和固定样本核对 URL、UTF-16 及缓冲区边界，全部断言通过时返回 0。 */
static int TestEncoding()
{
    int32_t totalCount = 0;
    int32_t failCount = 0;
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
        if (!ok)
        {
            ++failCount;
        }
    };

    std::string bytes;
    std::string expected;
    const char* digits = "0123456789ABCDEF";
    for (unsigned index = 0; index < 256; ++index)
    {
        const char byte = static_cast<char>(index);
        bytes.push_back(byte);
        if ((index >= '0' && index <= '9') || (index >= 'a' && index <= 'z') || (index >= 'A' && index <= 'Z'))
        {
            expected.push_back(byte);
        }
        else
        {
            expected.push_back('%');
            expected.push_back(digits[index >> 4]);
            expected.push_back(digits[index & 15]);
        }
    }
    check(CStringManager::UrlEncode(bytes) == expected && CStringManager::UrlDecode(expected) == bytes,
        "URL encoding C locale uses uppercase hex for all 256 byte values");
    const std::locale savedNumeric = std::locale::global(std::locale(std::locale::classic(), new CStringManagerTestGrouping));
    bool numbersMatch = false;
    try
    {
        numbersMatch = CStringManager::toStringInt32(INT32_MIN) == "-2147483648" &&
            CStringManager::toStringInt64(INT64_MAX) == "9223372036854775807";
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Integer conversion under grouped locale: %s\n", error.what());
    }
    std::locale::global(savedNumeric);
    check(numbersMatch,
        "protocol integers exclude locale grouping separators");
    check(CStringManager::atoi64(" -42tail") == -42 && CStringManager::atoui64("-1") == UINT64_MAX &&
        CStringManager::atoui64("18446744073709551616") == UINT64_MAX, "legacy integer prefix and saturation");
    check(CStringManager::parseInt32(std::wstring(L"23\0tail", 7), 91) == 91 &&
        CStringManager::parseFiniteDouble(std::wstring(L"1.25\0x", 6), -2.0) == -2.0,
        "strict parsers reject embedded NUL followed by more input");
#ifdef _WIN32
    // 1252 代码页把 0xe9 分类为字母，URL 仍须按同一组 ASCII 规则传输
    const char* currentCtype = std::setlocale(LC_CTYPE, nullptr);
    if (currentCtype == nullptr)
    {
        std::fputs("Cannot read LC_CTYPE\n", stderr);
        return 1;
    }
    const std::string savedCtype(currentCtype);
    const bool changed = std::setlocale(LC_CTYPE, ".1252") != nullptr;
    bool encodedHigh = false;
    try
    {
        encodedHigh = CStringManager::UrlEncode(std::string("x\xe9", 2)) == "x%E9";
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "URL encoding under Windows locale: %s\n", error.what());
    }
    const bool restored = std::setlocale(LC_CTYPE, savedCtype.c_str()) != nullptr;
    check(changed && restored && encodedHigh, "URL encoding is independent of the ANSI code page");
#else
    check(CStringManager::Utf8ToUnicode(std::string("ok\xe2\x82", 4)).empty(),
        "truncated UTF-8 does not return the valid prefix");
#endif
    const uint8_t utf16Data[] = {0xff, 0xfe, 0x41, 0x00, 0x00, 0x00, 0x3d, 0xd8, 0x00, 0xde};
    const std::vector<uint8_t> utf16(utf16Data, utf16Data + sizeof(utf16Data));
    std::wstring wide;
    std::vector<uint8_t> encodedBytes;
    std::wstring expectedWide(L"A\0", 2);
#if WCHAR_MAX <= 0xffff
    expectedWide.push_back(static_cast<wchar_t>(0xd83d));
    expectedWide.push_back(static_cast<wchar_t>(0xde00));
#else
    expectedWide.push_back(static_cast<wchar_t>(0x1f600));
#endif
    check(CStringManager::utf16LeToWide(utf16, wide) && wide == expectedWide, "UTF16LE consumes BOM and decodes embedded zero and supplementary character");
    check(CStringManager::wideToUtf16Le(expectedWide, encodedBytes) && encodedBytes == utf16, "UTF16LE reproduces independent byte vector");
    check(CStringManager::wideToUtf16Le(expectedWide, encodedBytes, false) && encodedBytes == std::vector<uint8_t>(utf16.begin() + 2, utf16.end()),
        "UTF16LE optional BOM");
    const std::wstring preserved = wide;
    const std::vector<uint8_t> highSurrogate(utf16.begin() + 6, utf16.begin() + 8);
    check(!CStringManager::utf16LeToWide(highSurrogate, wide) && wide == preserved, "unpaired high surrogate preserves destination");
    check(!CStringManager::utf16LeToWide(std::vector<uint8_t>(1, 65), wide) && wide == preserved, "odd UTF16 byte count preserves destination");
    const std::vector<uint8_t> priorBytes = encodedBytes;
    check(!CStringManager::wideToUtf16Le(std::wstring(1, static_cast<wchar_t>(0xdc00)), encodedBytes) &&
        encodedBytes == priorBytes, "unpaired low surrogate preserves destination");
    check(CStringManager::parseInt32(L" -2147483648 ", 7) == (-2147483647 - 1) &&
        CStringManager::parseInt32(L"2147483648", 7) == 7 && CStringManager::parseInt32(L"3x", 7) == 7, "strict integer bounds and trailing text");
    check(CStringManager::parseFiniteDouble(L" -1.25 ", 7.0) == -1.25 &&
        CStringManager::parseFiniteDouble(L"nan", 7.0) == 7.0 && CStringManager::parseFiniteDouble(L"1e9999", 7.0) == 7.0, "reject non-finite and overflow");
    check(CStringManager::formatFixedDouble(1.25, 3) == L"1.250" && CStringManager::formatFixedDouble(1.25, 18).empty(), "fixed decimal precision");
    char bounded[] = {'!', '!', '!', '!'};
    check(CStringManager::CopyToBuffer("abcd", bounded, 3) == 5 && bounded[0] == 'a' && bounded[1] == 'b' &&
        bounded[2] == 0 && bounded[3] == '!' && CStringManager::CopyToBuffer("abcd", nullptr, 0) == 5, "bounded copy termination and required size");
    check(CStringManager::MakeUpper(std::wstring(L"ab")) == L"AB" && CStringManager::MakeLower(std::wstring(L"AB")) == L"ab",
        "wide upper and lower case");
    std::cout << "CStringManager test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << std::endl;
    return failCount == 0 ? 0 : 1;
}

/** 验证宽格式化、区域设置与编码失败恢复，成功返回 0。 */
static int TestConversions()
{
    return RunCStringManagerRegressionTests() ? 0 : 1;
}

/** 检查可变参数格式化、整数前缀与32位溢出约定，全部通过返回0。 */
static int TestNumberAndFormat()
{
    const std::string payload(2048, 'x');
    if (CStringManager::StrFormat("[%s]-%03d", payload.c_str(), 7) != "[" + payload + "]-007" ||
        !CStringManager::StrFormat(nullptr).empty() || !CStringManager::StrFormat("").empty() ||
        CStringManager::StrFormat("A%cB", 0) != "A" ||
        CStringManager::Format("A%cB", 0) != std::string("A\0B", 3))
    {
        std::cerr << "[FAIL] StrFormat length, null or embedded zero" << std::endl;
        return 1;
    }
    if (CStringManager::IntToStr(0) != "0" || CStringManager::IntToStr(INT32_MIN) != "-2147483648" ||
        CStringManager::IntToStr(INT32_MAX) != "2147483647" || CStringManager::IntToStr(0x2b, true) != "2b" ||
        CStringManager::IntToStr(-1, true) != "ffffffff" || CStringManager::IntToStr(INT32_MIN, true) != "80000000")
    {
        std::cerr << "[FAIL] IntToStr decimal or hexadecimal representation" << std::endl;
        return 1;
    }
    const std::string decimal[] = {"", "+", "no digits", " \t+27km", "-0", "01x", "0x10",
        "2147483647", "-2147483648.", "2147483648", "-2147483649", std::string("42\0more", 7), std::string(200, '9')};
    const int decimalExpected[] = {0, 0, 0, 27, 0, 1, 0, INT32_MAX, INT32_MIN, 0, 0, 42, 0};
    for (size_t i = 0; i < sizeof(decimal) / sizeof(decimal[0]); ++i)
    {
        if (CStringManager::StrToInt(decimal[i]) != decimalExpected[i])
        {
            std::cerr << "[FAIL] decimal prefix case " << i << std::endl;
            return 1;
        }
    }
    const char* hex[] = {"0", "fFtail", " \n+0X2b;", "-0xff", "0000000000a", "7FFFFFFF", "80000000",
        "FFFFFFFF", "-1", "-FFFFFFFF", "100000000", "-100000000", "0X", "?"};
    const int hexExpected[] = {0, 255, 43, -255, 10, INT32_MAX, INT32_MIN, -1, -1, 1, 0, 0, 0, 0};
    for (size_t i = 0; i < sizeof(hex) / sizeof(hex[0]); ++i)
    {
        if (CStringManager::StrToInt(hex[i], true) != hexExpected[i])
        {
            std::cerr << "[FAIL] hexadecimal prefix case " << i << std::endl;
            return 1;
        }
    }
    std::cout << "[PASS] formatting and integer conversion" << std::endl;
    return 0;
}

/** 以两张固定数位表验证任意字节、非法字符及单独半字节，全部通过返回0。 */
static int TestHexBytes()
{
    const std::string upper("0123456789ABCDEF");
    const std::string lower("0123456789abcdef");
    const std::string validDigits("0123456789ABCDEFabcdef");
    std::string bytes;
    std::string expected;
    for (unsigned i = 0; i < 256; ++i)
    {
        const char value = static_cast<char>(i);
        const size_t digit = validDigits.find(value);
        const int expectedDigit = digit == std::string::npos ? -1 : static_cast<int>(digit > 15 ? digit - 6 : digit);
        std::string encoded;
        encoded.push_back(upper[i / 16]);
        encoded.push_back(upper[i % 16]);
        if (CStringManager::Char2Hex(static_cast<unsigned char>(i)) != encoded || CStringManager::CharToHex(value) != expectedDigit)
        {
            std::cerr << "[FAIL] hexadecimal byte " << i << std::endl;
            return 1;
        }
        bytes.push_back(value);
        expected.push_back(lower[i / 16]);
        expected.push_back(lower[i % 16]);
    }
    if (CStringManager::StrToHex(bytes) != expected || CStringManager::HexToStr(expected) != bytes ||
        !CStringManager::StrToHex("").empty() || !CStringManager::HexToStr("").empty() ||
        CStringManager::HexToStr("aFz2B?7") != std::string("\xaf\x02\xb0\x70", 4) ||
        CStringManager::HexToStr(std::string("0\0FF", 4)) != std::string("\0\xff", 2))
    {
        std::cerr << "[FAIL] hexadecimal binary data or partial nibble" << std::endl;
        return 1;
    }
    std::cout << "[PASS] hexadecimal conversion for all 256 bytes" << std::endl;
    return 0;
}

/** 以固定的路径字符白名单和手工 UTF-8 百分号序列检查窄、宽入口，全部通过返回0。 */
static int TestPathEncoding()
{
    const std::string literal("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz~-_./:()'");
    const char digits[] = "0123456789ABCDEF";
    std::string bytes;
    std::string expected;
    for (unsigned i = 0; i < 256; ++i)
    {
        const char value = static_cast<char>(i);
        bytes.push_back(value);
        if (i == 0 || literal.find(value) != std::string::npos)
        {
            expected.push_back(value);
        }
        else
        {
            expected.push_back('%');
            expected.push_back(digits[i / 16]);
            expected.push_back(digits[i % 16]);
        }
    }
    if (CStringManager::EncodeUrlPath(bytes) != expected || !CStringManager::EncodeUrlPath("").empty() ||
        CStringManager::UrlEncode(std::string("/:\0", 3)) != "%2F%3A%00")
    {
        std::cerr << "[FAIL] path byte whitelist or strict URL encoding" << std::endl;
        return 1;
    }
    std::wstring path(L"/\u4e2d:");
#if WCHAR_MAX <= 0xffff
    path.push_back(static_cast<wchar_t>(0xd83d));
    path.push_back(static_cast<wchar_t>(0xde00));
#else
    path.push_back(static_cast<wchar_t>(0x1f600));
#endif
    path.append(L" ?#\0X", 5);
    std::wstring wideExpected(L"/%E4%B8%AD:%F0%9F%98%80%20%3F%23");
    wideExpected.append(L"\0X", 2);
    CStringManager::EncodeUnicodeUrl(path);
    std::wstring empty;
    CStringManager::EncodeUnicodeUrl(empty);
    if (path != wideExpected || !empty.empty())
    {
        std::cerr << "[FAIL] wide path encoding replaces input with UTF-8 escapes" << std::endl;
        return 1;
    }
    std::cout << "[PASS] narrow and wide path encoding" << std::endl;
    return 0;
}

#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
/** 八个工作线程分别核对 5000 次格式化和分隔，成功返回 0。 */
static int TestConcurrentFormat()
{
    std::atomic<int32_t> failedThreadCount(0);
    std::thread threads[8];
    std::mutex start;
    {
        // 等所有线程均已创建再放行，避免前几个线程在其他线程启动之前完成。
        std::lock_guard<std::mutex> holdStart(start);
        try
        {
            for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
            {
                threads[threadIndex] = std::thread([threadIndex, &start, &failedThreadCount]()
                {
                    try
                    {
                        {
                            std::lock_guard<std::mutex> waitForStart(start);
                        }
                        for (int32_t loopIndex = 0; loopIndex < 5000; ++loopIndex)
                        {
                            const std::string value = CStringManager::Format("%d-%d", threadIndex, loopIndex);
                            const std::vector<std::string> parts = CStringManager::split(value, "-");
                            if (parts.size() != 2 || parts[0] != std::to_string(threadIndex) ||
                                parts[1] != std::to_string(loopIndex))
                            {
                                ++failedThreadCount;
                                return;
                            }
                        }
                    }
                    catch (...)
                    {
                        ++failedThreadCount;
                    }
                });
            }
        }
        catch (...)
        {
            // 无法创建全部线程也要放行并回收已经启动的线程。
            ++failedThreadCount;
        }
    }
    for (size_t i = 0; i < sizeof(threads) / sizeof(threads[0]); ++i)
    {
        if (threads[i].joinable())
        {
            threads[i].join();
        }
    }
    return failedThreadCount.load() == 0 ? 0 : 1;
}
#endif

/** 验证容量、下标、精度和字节序参数的生效边界
@return 满足所有返回值及输出约束时为零
*/
static int TestParameterBounds()
{
    char buffer[] = {'x', 'y'};
    if (CStringManager::CopyToBuffer("abc", buffer, 0) != 4 || buffer[0] != 'x' ||
        CStringManager::CopyToBuffer("abc", buffer, 1) != 4 || buffer[0] != 0 || buffer[1] != 'y')
    {
        std::cerr << "CopyToBuffer capacity boundary" << std::endl;
        return 1;
    }
    const std::string binary("A\0B", 3);
    char exact[] = {'?', '?', '?', '?', '!'};
    if (CStringManager::CopyToBuffer(binary, nullptr, sizeof(exact)) != 4 ||
        CStringManager::CopyToBuffer(binary, exact, 4) != 4 || std::memcmp(exact, "A\0B\0!", 5) != 0)
    {
        std::cerr << "CopyToBuffer exact capacity with embedded NUL" << std::endl;
        return 1;
    }
    char partial[] = {'?', '?', '!'};
    if (CStringManager::CopyToBuffer("\xe4\xb8\xad", partial, 2) != 4 ||
        std::memcmp(partial, "\xe4\0!", 3) != 0)
    {
        std::cerr << "CopyToBuffer truncates UTF-8 by byte" << std::endl;
        return 1;
    }
    std::string edited("aba");
    if (CStringManager::Find(edited, 'a', 1) != 2 || CStringManager::Find(edited, "a", 3) != std::string::npos ||
        CStringManager::ReserveFind(edited, 'a') != 2 || CStringManager::GetLength(edited) != 3 ||
        CStringManager::Insert(edited, 9, '!') != 4 || edited != "aba!" ||
        CStringManager::Delete(edited, 1, 99) != 1 || edited != "a" ||
        CStringManager::Replace(edited, 'a', 'b') != 1 || edited != "b" ||
        CStringManager::Replace("abba", "bb", "x") != "axa" || !CStringManager::Replace(nullptr, "x", "").empty())
    {
        std::cerr << "Find, edit or length result" << std::endl;
        return 1;
    }
    if (CStringManager::formatFixedDouble(1.25, 0) != L"1" ||
        CStringManager::formatFixedDouble(0.5, 17) != L"0.50000000000000000" ||
        !CStringManager::formatFixedDouble(0.0, -1).empty() ||
        !CStringManager::formatFixedDouble(std::numeric_limits<double>::infinity(), 2).empty())
    {
        std::cerr << "Fixed precision limits" << std::endl;
        return 1;
    }
    const uint8_t invalid[][4] = {{0xfe, 0xff, 65, 0}, {0, 0xd8, 65, 0}, {0, 0xdc, 65, 0}};
    std::wstring text(L"keep");
    for (size_t i = 0; i < 3; ++i)
    {
        if (CStringManager::utf16LeToWide(std::vector<uint8_t>(invalid[i], invalid[i] + 4), text) || text != L"keep")
        {
            std::cerr << "Invalid UTF-16 case " << i << std::endl;
            return 1;
        }
    }
    const uint8_t bom[] = {0xff, 0xfe};
    const std::vector<uint8_t> marker(bom, bom + 2);
    std::vector<uint8_t> bytes;
    if (!CStringManager::utf16LeToWide(marker, text) || !text.empty() ||
        !CStringManager::wideToUtf16Le(text, bytes) || bytes != marker ||
        !CStringManager::wideToUtf16Le(text, bytes, false) || !bytes.empty())
    {
        std::cerr << "Empty UTF-16 stream with and without BOM" << std::endl;
        return 1;
    }
    const uint8_t wire[] = {0xff, 0xfe, 0xff, 0xfe, 0xff, 0xd7, 0x00, 0xe0,
        0x00, 0xd8, 0x00, 0xdc, 0xff, 0xdb, 0xff, 0xdf};
    const std::vector<uint8_t> expectedBytes(wire, wire + sizeof(wire));
    std::wstring expectedText(1, static_cast<wchar_t>(0xfeff));
    expectedText.push_back(static_cast<wchar_t>(0xd7ff));
    expectedText.push_back(static_cast<wchar_t>(0xe000));
#if WCHAR_MAX <= 0xffff
    expectedText.push_back(static_cast<wchar_t>(0xd800));
    expectedText.push_back(static_cast<wchar_t>(0xdc00));
    expectedText.push_back(static_cast<wchar_t>(0xdbff));
    expectedText.push_back(static_cast<wchar_t>(0xdfff));
#else
    expectedText.push_back(static_cast<wchar_t>(0x10000));
    expectedText.push_back(static_cast<wchar_t>(0x10ffff));
#endif
    if (!CStringManager::wideToUtf16Le(expectedText, bytes) || bytes != expectedBytes ||
        !CStringManager::utf16LeToWide(expectedBytes, text) || text != expectedText ||
        !CStringManager::wideToUtf16Le(expectedText, bytes, false) ||
        bytes != std::vector<uint8_t>(expectedBytes.begin() + 2, expectedBytes.end()) ||
        !CStringManager::utf16LeToWide(bytes, text) || text != expectedText.substr(1))
    {
        std::cerr << "UTF-16 text BOM and scalar boundaries" << std::endl;
        return 1;
    }
    const std::vector<uint8_t> savedBytes(bytes);
    std::wstring invalidText(L"valid prefix");
    invalidText.push_back(static_cast<wchar_t>(0xd800));
    if (CStringManager::wideToUtf16Le(invalidText, bytes) || bytes != savedBytes)
    {
        std::cerr << "Trailing high surrogate changed output" << std::endl;
        return 1;
    }
#if WCHAR_MAX > 0xffff
    invalidText.back() = static_cast<wchar_t>(0x110000);
    if (CStringManager::wideToUtf16Le(invalidText, bytes, false) || bytes != savedBytes)
    {
        std::cerr << "Out-of-range Unicode scalar changed output" << std::endl;
        return 1;
    }
#endif
    return 0;
}

/** 转发两次同一份参数表；可能覆盖借用源的调用放在最后
@param [out] first 独立输出
@param [in,out] second 允许格式或实参借用的输出
@param [in] format 与变参匹配的窄或宽格式，可空
*/
template<class String, class Char>
static void ForwardTwice(String& first, String& second, const Char* format, ...)
{
    va_list args;
    va_start(args, format);
    CStringManager::formatV(first, format, args);
    CStringManager::formatV(second, format, args);
    va_end(args);
}

/** 检查调用方可重复转发参数以及原位格式化和空格式
@return 两种字符宽度均满足契约时返回零
*/
static int TestForwardedArguments()
{
    std::string a, b("data");
    ForwardTwice(a, b, "%s%c%d", b.c_str(), 0, 17);
    if (a != std::string("data\0" "17", 7) || a != b)
    {
        return 1;
    }
    std::wstring c, d(L"%ls%lc%d");
    ForwardTwice(c, d, d.c_str(), L"data", static_cast<wint_t>(0), 17);
    if (c != std::wstring(L"data\0" L"17", 7) || c != d)
    {
        return 1;
    }
    ForwardTwice(a, b, static_cast<const char*>(nullptr));
    ForwardTwice(c, d, static_cast<const wchar_t*>(nullptr));
    return a.empty() && b.empty() && c.empty() && d.empty() ? 0 : 1;
}

/** 执行所选用例，报告标准库异常，使后续独立用例仍能运行
@param [in] item 本平台已登记的用例
@return 用例状态，发生异常返回 1
*/
static int RunSelectedCase(const LibraryTestCase& item)
{
    try
    {
        return item.run();
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "case %u: %s\n", item.id, error.what());
    }
    catch (...)
    {
        std::fprintf(stderr, "case %u: unexpected exception\n", item.id);
    }
    return 1;
}

/** 按控制台参数选择功能或计时入口
@param [in] argc 参数数量
@param [in] argv 参数数组
@return 成功为 0，用例失败为 1，选择无效为 2
*/
int main(int argc, char** argv)
try
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarkScenarios(true);
    }
    const LibraryTestCase cases[] = {
        {kCaseStrings, false, TestStrings},
        {kCaseEncoding, false, TestEncoding},
        {kCaseConversions, false, TestConversions},
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
        {kCaseConcurrentFormat, true, TestConcurrentFormat},
#endif
        {kCaseNumberAndFormat, false, TestNumberAndFormat},
        {kCaseHexBytes, false, TestHexBytes},
        {kCasePathEncoding, false, TestPathEncoding},
        {kCaseParameterBounds, false, TestParameterBounds},
        {kCaseForwardedArguments, false, TestForwardedArguments},
        {kCaseReplaceAll, false, TestReplaceAll},
        {kCaseBenchmarkInputs, false, TestBenchmarkInputs},
    };
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    // 全量先完成瞬时用例，再执行并发压力；指定编号仍按 ID 升序调度。
    if (argc == 1 || (argc == 2 && std::strcmp(argv[1], "--all") == 0))
    {
        char instant[] = "--instant";
        char stress[] = "--stress";
        char* selection[] = {argv[0], instant, nullptr};
        const int instantResult = LibraryCaseSelection::run(2, selection, cases, RunSelectedCase);
        selection[1] = stress;
        const int stressResult = LibraryCaseSelection::run(2, selection, cases, RunSelectedCase);
        return instantResult != 0 ? instantResult : stressResult;
    }
#endif
    return LibraryCaseSelection::run(argc, argv, cases, RunSelectedCase);
}
catch (const std::exception& error)
{
    std::fprintf(stderr, "CStringManagerTest: %s\n", error.what());
    return 1;
}
catch (...)
{
    std::fputs("CStringManagerTest: unexpected exception\n", stderr);
    return 1;
}