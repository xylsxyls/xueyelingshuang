#include "PlatformTest/PlatformTestAPI.h"
#include "CStringManagerTest.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstring>
#include <clocale>
#include <atomic>
#include <functional>
#include <iostream>
#include <thread>

/** 校准并测量返回新字符串的变换；计入结果构造和销毁，用长度与轮换字节校验实际输出。
@param [in] name 性能记录名
@param [in] input 计时外已准备的非空输入
*/
template<std::string (*Transform)(const std::string&)>
static void MeasureByteConversion(const char* name, const std::string& input)
{
    LibraryBenchmark::run(name, [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::string value = Transform(input);
            if (value.empty())
            {
                return 0;
            }
            sum += value.size() + static_cast<unsigned char>(value[i % value.size()]);
        }
        return sum;
    });
}

/** 运行本工程的接口性能用例
@return 输入和结果满足各用例约定时返回0
*/
static int RunBenchmarks()
{
    const std::string pattern = "abcdefghijklmno! +%";
    std::string input;
    while (input.size() < 4096)
    {
        input.append(pattern);
    }
    input.resize(4096);
    const std::string encoded = CStringManager::UrlEncode(input);
    if (CStringManager::UrlDecode(encoded) != input)
    {
        return 1;
    }
    LibraryBenchmark::run("CStringManager.UrlEncode.4KiB", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::string value = CStringManager::UrlEncode(input);
            sum += value.size() + static_cast<unsigned char>(value[i % value.size()]);
        }
        return sum;
    });
    LibraryBenchmark::run("CStringManager.UrlDecode.4KiB-output", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::string value = CStringManager::UrlDecode(encoded);
            sum += value.size() + static_cast<unsigned char>(value[i % value.size()]);
        }
        return sum;
    });
    std::string original;
    for (size_t i = 0; i < 1024; ++i)
    {
        original.append("xa");
    }
    std::string check(original);
    CStringManager::ReplaceAll(check, "x", "y");
    if (check.size() != 2048 || check.find('x') != std::string::npos)
    {
        return 1;
    }
    LibraryBenchmark::run("CStringManager.ReplaceAll.1024-matches", [&](uint32_t count) -> uint64_t {
        uint64_t sum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            std::string value(original);
            CStringManager::ReplaceAll(value, "x", "y");
            sum += value.size() + static_cast<unsigned char>(value[i % value.size()]);
        }
        return sum;
    });
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
    if (CStringManager::EncodeUrlPath(binary) != path || CStringManager::StrToHex(binary) != hex ||
        CStringManager::HexToStr(hex) != binary)
    {
        return 1;
    }
    MeasureByteConversion<CStringManager::EncodeUrlPath>("CStringManager.EncodeUrlPath.4KiB", binary);
    MeasureByteConversion<CStringManager::StrToHex>("CStringManager.StrToHex.4KiB", binary);
    MeasureByteConversion<CStringManager::HexToStr>("CStringManager.HexToStr.8KiB", hex);
    return 0;
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
    check(splitCharResult.size() == 3 && splitCharResult[2].empty(), "CStringManager split char");
    std::vector<std::string> aliased(1, "alpha,beta,gamma");
    CStringManager::split(aliased, aliased[0], ',');
    check(aliased.size() == 3 && aliased[0] == "alpha" && aliased[1] == "beta" && aliased[2] == "gamma", "split from its own output");
    std::string duplicated("aba");
    std::wstring wideDuplicated(L"aba");
    check(CStringManager::Replace(duplicated, "a", duplicated) == 2 && duplicated == "abababa", "replacement aliases output");
    check(CStringManager::Replace(wideDuplicated, L"a", wideDuplicated) == 2 && wideDuplicated == L"abababa", "wide replacement aliases output");
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
    check(CStringManager::Delete(editText, 999, 2) == editText.length(), "CStringManager Delete oversize offset");
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
    highByteText.push_back(static_cast<char>(0xff));
    check(CStringManager::MakeUpper(highByteText).size() == 1 && CStringManager::MakeLower(highByteText).size() == 1,
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
    const std::string signed32 = CStringManager::toStringInt32(INT32_MIN);
    const std::string signed64 = CStringManager::toStringInt64(INT64_MAX);
    std::locale::global(savedNumeric);
    check(signed32 == "-2147483648" && signed64 == "9223372036854775807",
        "protocol integers exclude locale grouping separators");
    check(CStringManager::atoi64(" -42tail") == -42 && CStringManager::atoui64("-1") == UINT64_MAX &&
        CStringManager::atoui64("18446744073709551616") == UINT64_MAX, "legacy integer prefix and saturation");
    check(CStringManager::parseInt32(std::wstring(L"23\0tail", 7), 91) == 91 &&
        CStringManager::parseFiniteDouble(std::wstring(L"1.25\0x", 6), -2.0) == -2.0,
        "strict parsers reject embedded NUL followed by more input");
#ifdef _WIN32
    // 1252 代码页把 0xe9 分类为字母，URL 仍须按同一组 ASCII 规则传输
    const std::string savedCtype(std::setlocale(LC_CTYPE, nullptr));
    const bool changed = std::setlocale(LC_CTYPE, ".1252") != nullptr;
    const std::string encodedHigh = CStringManager::UrlEncode(std::string("x\xe9", 2));
    const bool restored = std::setlocale(LC_CTYPE, savedCtype.c_str()) != nullptr;
    check(changed && restored && encodedHigh == "x%E9", "URL encoding is independent of the ANSI code page");
#else
    check(CStringManager::Utf8ToUnicode(std::string("ok\xe2\x82", 4)).empty(),
        "truncated UTF-8 does not return the valid prefix");
#endif
    const uint8_t utf16Data[] = {0xff, 0xfe, 0x41, 0x00, 0x00, 0x00, 0x3d, 0xd8, 0x00, 0xde};
    const std::vector<uint8_t> utf16(utf16Data, utf16Data + sizeof(utf16Data));
    std::wstring wide;
    std::vector<uint8_t> encodedBytes;
    check(CStringManager::utf16LeToWide(utf16, wide) && wide.size() == (sizeof(wchar_t) == 2 ? 4 : 3) &&
        wide[0] == L'A' && wide[1] == 0, "UTF16LE consumes BOM and decodes embedded zero and supplementary character");
    check(CStringManager::wideToUtf16Le(wide, encodedBytes) && encodedBytes == utf16, "UTF16LE reproduces independent byte vector");
    check(CStringManager::wideToUtf16Le(wide, encodedBytes, false) && encodedBytes == std::vector<uint8_t>(utf16.begin() + 2, utf16.end()),
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
    if (sizeof(wchar_t) == 2)
    {
        path.push_back(static_cast<wchar_t>(0xd83d));
        path.push_back(static_cast<wchar_t>(0xde00));
    }
    else
    {
        path.push_back(static_cast<wchar_t>(0x1f600));
    }
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
    std::vector<std::thread> threads;
    for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
    {
        threads.push_back(std::thread([threadIndex, &failedThreadCount]() -> void
        {
            for (int32_t loopIndex = 0; loopIndex < 5000; ++loopIndex)
            {
                std::string value = CStringManager::Format("%d-%d", threadIndex, loopIndex);
                std::vector<std::string> parts = CStringManager::split(value, "-");
                if (parts.size() != 2 || std::to_string(threadIndex) != parts[0] || std::to_string(loopIndex) != parts[1])
                {
                    failedThreadCount.fetch_add(1);
                    return;
                }
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    return failedThreadCount.load() == 0 ? 0 : 1;
}
#endif

int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarks();
    }
    const LibraryTestCase cases[] = {
        {1, false, TestStrings},
        {2, false, TestEncoding},
        {3, false, TestConversions},
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
        {4, true, TestConcurrentFormat},
#endif
        {5, false, TestNumberAndFormat},
        {6, false, TestHexBytes},
        {7, false, TestPathEncoding},
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}