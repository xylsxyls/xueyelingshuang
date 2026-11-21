#include "CStringManager.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cwchar>
#include <cstring>
#include <cstdlib>
#include <cwctype>
#include <sstream>
#include <cmath>
#include <locale>
#include <limits>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <codecvt>
#endif
#include <iterator>

// URL 百分号转义和单字节十六进制文本使用大写数字。
static const char kUpperHexDigits[] = "0123456789ABCDEF";

int32_t CStringManager::parseInt32(const std::wstring& text, int32_t defaultValue)
{
    std::wistringstream stream(text);
    stream.imbue(std::locale::classic());
    int64_t value = 0;
    if (!(stream >> value) || value < (std::numeric_limits<int32_t>::min)() ||
        value > (std::numeric_limits<int32_t>::max)())
    {
        return defaultValue;
    }
    stream >> std::ws;
    return stream.eof() ? static_cast<int32_t>(value) : defaultValue;
}

double CStringManager::parseFiniteDouble(const std::wstring& text, double defaultValue)
{
    std::wistringstream stream(text);
    stream.imbue(std::locale::classic());
    double value = 0.0;
    if (!(stream >> value) || !std::isfinite(value))
    {
        return defaultValue;
    }
    stream >> std::ws;
    return stream.eof() ? value : defaultValue;
}

std::wstring CStringManager::formatFixedDouble(double value, int32_t precision)
{
    if (!std::isfinite(value) || precision < 0 || precision > 17)
    {
        return std::wstring();
    }
    std::wostringstream stream;
    stream.imbue(std::locale::classic());
    stream.setf(std::ios::fixed);
    stream.precision(precision);
    stream << value;
    return stream.str();
}

bool CStringManager::utf16LeToWide(const std::vector<uint8_t>& bytes, std::wstring& text)
{
    if ((bytes.size() & 1) != 0 ||
        (bytes.size() >= 2 && bytes[0] == 0xfe && bytes[1] == 0xff))
    {
        return false;
    }
    const size_t offset = bytes.size() >= 2 && bytes[0] == 0xff && bytes[1] == 0xfe ? 2 : 0;
    std::wstring result;
    result.reserve((bytes.size() - offset) / 2);
    for (size_t index = offset; index < bytes.size(); index += 2)
    {
        const uint32_t unit = bytes[index] | (static_cast<uint32_t>(bytes[index + 1]) << 8);
        if (unit >= 0xdc00 && unit <= 0xdfff)
        {
            return false;
        }
        if (unit >= 0xd800 && unit <= 0xdbff)
        {
            if (bytes.size() - index < 4)
            {
                return false;
            }
            const uint32_t low = bytes[index + 2] | (static_cast<uint32_t>(bytes[index + 3]) << 8);
            if (low < 0xdc00 || low > 0xdfff)
            {
                return false;
            }
            if (sizeof(wchar_t) == 2)
            {
                result.push_back(static_cast<wchar_t>(unit));
                result.push_back(static_cast<wchar_t>(low));
            }
            else
            {
                result.push_back(static_cast<wchar_t>(0x10000 + ((unit - 0xd800) << 10) + low - 0xdc00));
            }
            index += 2;
        }
        else
        {
            result.push_back(static_cast<wchar_t>(unit));
        }
    }
    text.swap(result);
    return true;
}

bool CStringManager::wideToUtf16Le(const std::wstring& text, std::vector<uint8_t>& bytes, bool addBom)
{
    std::vector<uint8_t> result;
    if (addBom)
    {
        result.push_back(0xff);
        result.push_back(0xfe);
    }
    for (size_t index = 0; index < text.size(); ++index)
    {
        uint32_t scalar = static_cast<uint32_t>(text[index]);
        if (sizeof(wchar_t) == 2 && scalar >= 0xd800 && scalar <= 0xdbff)
        {
            if (text.size() - index < 2)
            {
                return false;
            }
            const uint32_t low = static_cast<uint32_t>(text[++index]);
            if (low < 0xdc00 || low > 0xdfff)
            {
                return false;
            }
            scalar = 0x10000 + ((scalar - 0xd800) << 10) + low - 0xdc00;
        }
        else if (scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff))
        {
            return false;
        }
        if (scalar > 0xffff)
        {
            scalar -= 0x10000;
            const uint32_t high = 0xd800 + (scalar >> 10);
            result.push_back(static_cast<uint8_t>(high & 0xff));
            result.push_back(static_cast<uint8_t>(high >> 8));
            scalar = 0xdc00 + (scalar & 0x3ff);
        }
        result.push_back(static_cast<uint8_t>(scalar & 0xff));
        result.push_back(static_cast<uint8_t>(scalar >> 8));
    }
    bytes.swap(result);
    return true;
}

size_t CStringManager::CopyToBuffer(const std::string& value, char* buffer, size_t capacity)
{
    if (buffer != nullptr && capacity != 0)
    {
        const size_t length = (std::min)(capacity - 1, value.size());
        if (length != 0)
        {
            std::memmove(buffer, value.data(), length);
        }
        buffer[length] = '\0';
    }
    return value.size() + 1;
}

size_t CStringManager::FindOther(const std::string& str, char cLeft, char cRight, size_t nSelect)
{
    if (nSelect >= str.size() || (str[nSelect] != cLeft && str[nSelect] != cRight))
    {
        return std::string::npos;
    }
    if (cLeft == cRight)
    {
        // 相同分隔符按出现顺序两两配对，最后一个未配对时返回 npos
        bool opening = true;
        size_t previous = std::string::npos;
        for (size_t i = str.find(cLeft); i != std::string::npos; i = str.find(cLeft, i + 1))
        {
            if (i == nSelect)
            {
                return opening ? str.find(cRight, i + 1) : previous;
            }
            previous = i;
            opening = !opening;
        }
        return std::string::npos;
    }
    size_t depth = 1;
    if (str[nSelect] == cLeft)
    {
        for (size_t i = nSelect + 1; i < str.size(); ++i)
        {
            if (str[i] == cLeft)
            {
                ++depth;
            }
            else if (str[i] == cRight && --depth == 0)
            {
                return i;
            }
        }
    }
    else
    {
        for (size_t i = nSelect; i > 0;)
        {
            const char current = str[--i];
            if (current == cRight)
            {
                ++depth;
            }
            else if (current == cLeft && --depth == 0)
            {
                return i;
            }
        }
    }
    return std::string::npos;
}

/** 用完整分隔符拆分文本，保存相邻分隔符和尾部产生的空字段
@param [in] text 输入文本，允许含零值字符
@param [in] separator 字符或同类型字符串分隔符
@param [in] width 分隔符的字符数，零表示整串作为一个字段
@return 按输入顺序排列的字段
*/
template<class String, class Separator>
static std::vector<String> SplitFields(const String& text, const Separator& separator, size_t width)
{
    std::vector<String> fields;
    if (width == 0)
    {
        fields.push_back(text);
        return fields;
    }
    size_t count = 1;
    for (size_t match = text.find(separator); match != String::npos; match = text.find(separator, match + width))
    {
        ++count;
    }
    fields.reserve(count);
    size_t begin = 0;
    size_t match = text.find(separator);
    while (match != String::npos)
    {
        fields.emplace_back(text, begin, match - begin);
        begin = match + width;
        match = text.find(separator, begin);
    }
    fields.emplace_back(text, begin);
    return fields;
}

std::vector<std::string> CStringManager::split(const std::string& splitString, const std::string& separate_character)
{
    return SplitFields(splitString, separate_character, separate_character.size());
}

std::vector<std::wstring> CStringManager::split(const std::wstring& splitString, const std::wstring& separate_character)
{
    return SplitFields(splitString, separate_character, separate_character.size());
}

void CStringManager::split(std::vector<std::string>& result, const std::string& splitString, char separate_character)
{
    SplitFields(splitString, separate_character, 1).swap(result);
}

void CStringManager::ReplaceAll(std::string& str, const std::string& old_value, const std::string& new_value)
{
    if (old_value.empty() || old_value == new_value)
    {
        return;
    }
    if (&str == &old_value)
    {
        // 整个目标恰好匹配一次；缩短或等长替换都无法再匹配原来的整串。
        str = new_value;
        return;
    }
    if (old_value.size() == 1 && new_value.size() == 1)
    {
        Replace(str, old_value[0], new_value[0]);
        return;
    }
    if (new_value.size() > old_value.size())
    {
        Replace(str, old_value, new_value);
        return;
    }
    // 此处若 new_value 就是目标，旧模式至少与目标等长且内容不同，find 必然失败。
    size_t position = str.find(old_value);
    while (position != std::string::npos)
    {
        str.replace(position, old_value.size(), new_value);
        // 仅替换位置左侧最多 old_value.size()-1 个字符可能构成新的跨边界匹配
        const size_t overlap = old_value.size() - 1;
        const size_t restart = position > overlap ? position - overlap : 0;
        position = str.find(old_value, restart);
    }
}

std::string CStringManager::Mid(const std::string& str, size_t offset, size_t count)
{
	if (offset >= str.size())
	{
		return "";
	}
	return str.substr(offset, count);
}

std::string CStringManager::Left(const std::string& str, size_t count)
{
	return Mid(str, 0, count);
}

std::string CStringManager::Right(const std::string& str, size_t count)
{
	if (count >= str.size())
	{
		return str;
	}
	return Mid(str, str.size() - count, count);
}

size_t CStringManager::Find(const std::string& str, const std::string& right, size_t offset)
{
	return str.find(right, offset);
}

size_t CStringManager::Find(const std::string& str, char ch, size_t offset)
{
	return str.find(ch, offset);
}

size_t CStringManager::ReserveFind(const std::string& str, const std::string& right)
{
	return str.rfind(right);
}

size_t CStringManager::ReserveFind(const std::string& str, char ch)
{
	return str.rfind(ch);
}

size_t CStringManager::GetLength(const std::string& str)
{
	return str.length();
}

size_t CStringManager::Delete(std::string& str, size_t offset, size_t count)
{
	if (offset >= str.size())
	{
		return str.length();
	}
	return str.erase(offset, count).length();
}

size_t CStringManager::Insert(std::string& str, size_t offset, const std::string& right)
{
	if (offset > str.size())
	{
		offset = str.size();
	}
	return str.insert(offset, right).length();
}

size_t CStringManager::Insert(std::string& str, size_t offset, char ch)
{
	if (offset > str.size())
	{
		offset = str.size();
	}
	return str.insert(offset, &ch, 1).length();
}

/** 按从左到右的顺序替换非重叠匹配，跳过刚写入的内容
@param [in,out] str 待编辑的窄或宽字符串
@param [in] oldstr 匹配文本，可借用 str；空串不匹配
@param [in] newstr 替换文本，可借用 str
@return 完成替换的次数
*/
template<class String>
static size_t ReplaceMatches(String& str, const String& oldstr, const String& newstr)
{
    if (oldstr.empty())
    {
        return 0;
    }
    if (&str == &oldstr)
    {
        str = newstr;
        return 1;
    }
    if (&str == &newstr)
    {
        const String replacement(newstr);
        return ReplaceMatches(str, oldstr, replacement);
    }
    size_t count = 0;
    size_t pos = str.find(oldstr);
    while (pos != String::npos)
    {
        str.replace(pos, oldstr.size(), newstr);
        ++count;
        pos = str.find(oldstr, pos + newstr.size());
    }
    return count;
}

size_t CStringManager::Replace(std::string& str, const std::string& oldstr, const std::string& newstr)
{
    return ReplaceMatches(str, oldstr, newstr);
}

std::string CStringManager::Replace(const char* str, const std::string& oldstr, const std::string& newstr)
{
	if (str == nullptr)
	{
		return std::string();
	}
	std::string result = str;
	Replace(result, oldstr, newstr);
	return result;
}

size_t CStringManager::Replace(std::wstring& str, const std::wstring& oldstr, const std::wstring& newstr)
{
    return ReplaceMatches(str, oldstr, newstr);
}

size_t CStringManager::Replace(std::string& str, char ch1, char ch2)
{
	size_t count = 0;
	for (size_t pos = 0; pos != str.size(); ++pos)
	{
		if (str[pos] == ch1)
		{
			str[pos] = ch2;
			++count;
		}
	}
	return count;
}

void CStringManager::Format(std::string& str, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    formatV(str, fmt, args);
    va_end(args);
}

std::string CStringManager::Format(const char* fmt, ...)
{
    std::string result;
    if (fmt == nullptr)
    {
        return result;
    }
    va_list args;
    va_start(args, fmt);
    try
    {
        formatNarrowArguments(result, fmt, args);
    }
    catch (...)
    {
        result.clear();
    }
    va_end(args);
    return result;
}

void CStringManager::Format(std::wstring& str, const wchar_t* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    formatV(str, fmt, args);
    va_end(args);
}

std::string CStringManager::StrFormat(const char* fmt, ...)
{
    std::string result;
    va_list args;
    va_start(args, fmt);
    formatV(result, fmt, args);
    va_end(args);
    const size_t terminator = result.find('\0');
    if (terminator != std::string::npos)
    {
        result.resize(terminator);
    }
    return result;
}

std::wstring CStringManager::Format(const wchar_t* fmt, ...)
{
    std::wstring result;
    if (fmt == nullptr)
    {
        return result;
    }
    va_list args;
    va_start(args, fmt);
    try
    {
        formatWideArguments(result, fmt, args);
    }
    catch (...)
    {
        result.clear();
    }
    va_end(args);
    return result;
}

void CStringManager::MakeReverse(std::string& str)
{
	std::reverse(str.begin(), str.end());
}

std::string CStringManager::MakeUpper(const std::string& src)
{
	std::string dst;
	std::transform(src.begin(), src.end(), std::back_inserter(dst),
		[](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
	return dst;
}

std::string CStringManager::MakeLower(const std::string& src)
{
	std::string dst;
	std::transform(src.begin(), src.end(), std::back_inserter(dst),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	return dst;
}

std::wstring CStringManager::MakeUpper(const std::wstring& src)
{
	std::wstring dst;
	std::transform(src.begin(), src.end(), std::back_inserter(dst),
		[](wchar_t ch) { return static_cast<wchar_t>(::towupper(ch)); });
	return dst;
}

std::wstring CStringManager::MakeLower(const std::wstring& src)
{
	std::wstring dst;
	std::transform(src.begin(), src.end(), std::back_inserter(dst),
		[](wchar_t ch) { return static_cast<wchar_t>(::towlower(ch)); });
	return dst;
}

std::string CStringManager::GetMidString(const std::string& src, const std::string& leftString, const std::string& rightString)
{
	size_t left = src.find(leftString);
	if (left == std::string::npos)
	{
		return "";
	}
	size_t begin = left + leftString.size();
	size_t right = src.find(rightString, begin);
	if (right == std::string::npos)
	{
		return "";
	}
	return Mid(src, begin, right - begin);
}

uint64_t CStringManager::atoui64(const char* str)
{
	if (str == nullptr)
	{
		return 0;
	}
	return strtoull(str, nullptr, 10);
}

int64_t CStringManager::atoi64(const char* str)
{
	if (str == nullptr)
	{
		return 0;
	}
	return strtoll(str, nullptr, 10);
}

std::string CStringManager::toStringInt32(int32_t value)
{
    return toStringInt64(value);
}

std::string CStringManager::toStringInt64(int64_t value)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << value;
    return stream.str();
}

std::string CStringManager::IntToStr(int value, bool hex)
{
    return hex ? Format("%x", static_cast<unsigned int>(value)) : toStringInt32(value);
}

int CStringManager::StrToInt(const std::string& text, bool hex)
{
    const int64_t value = strtoll(text.c_str(), nullptr, hex ? 16 : 10);
    const int maximum = (std::numeric_limits<int>::max)();
    if (!hex)
    {
        return value < (std::numeric_limits<int>::min)() || value > maximum ? 0 : static_cast<int>(value);
    }
    const unsigned int mask = (std::numeric_limits<unsigned int>::max)();
    if (value < -static_cast<int64_t>(mask) || value > static_cast<int64_t>(mask))
    {
        return 0;
    }
    const unsigned int bits = static_cast<unsigned int>(value);
    // 用可表示的有符号运算还原负值，避免将超出 int 范围的无符号数直接窄化。
    return bits <= static_cast<unsigned int>(maximum) ? static_cast<int>(bits) : -1 - static_cast<int>(mask - bits);
}

std::string CStringManager::Char2Hex(unsigned char byte)
{
    const char digits[] = { kUpperHexDigits[byte >> 4], kUpperHexDigits[byte & 15] };
    return std::string(digits, sizeof(digits));
}

std::string CStringManager::StrToHex(const std::string& bytes)
{
    static const char digits[] = "0123456789abcdef";
    std::string result;
    if (bytes.size() > result.max_size() / 2)
    {
        throw std::length_error("StrToHex");
    }
    result.resize(bytes.size() * 2);
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        const unsigned char byte = static_cast<unsigned char>(bytes[i]);
        result[2 * i] = digits[byte >> 4];
        result[2 * i + 1] = digits[byte & 15];
    }
    return result;
}

std::string CStringManager::HexToStr(const std::string& digits)
{
    std::string result(digits.size() / 2 + digits.size() % 2, '\0');
    for (size_t i = 0; i < result.size(); ++i)
    {
        const int high = CharToHex(digits[2 * i]);
        const int low = 2 * i + 1 < digits.size() ? CharToHex(digits[2 * i + 1]) : 0;
        result[i] = static_cast<char>((high < 0 ? 0 : high << 4) | (low < 0 ? 0 : low));
    }
    return result;
}

#if CSTRING_MANAGER_HAS_ANSI
std::string CStringManager::UnicodeToAnsi(const std::wstring& wstrSrc)
{
#ifdef _WIN32
    return convertWindowsWide(wstrSrc, CP_ACP);
#elif defined(__unix__) || defined(__APPLE__)
    try
    {
        if (wstrSrc.empty())
        {
            return "";
        }
        const std::locale locale = createAnsiLocale();
        const std::codecvt<wchar_t, char, std::mbstate_t>& converter =
            std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(locale);
        std::string result;
        const int32_t maximum = converter.max_length();
        // GBK每个宽字符最多maximum字节，额外预留一次结束转换的空间；先检查乘法溢出。
        if (maximum <= 0 || wstrSrc.size() >= result.max_size() / static_cast<size_t>(maximum))
        {
            return "";
        }
        result.resize((wstrSrc.size() + 1) * static_cast<size_t>(maximum));
        std::mbstate_t state = {};
        const wchar_t* inputEnd = wstrSrc.data() + wstrSrc.size();
        const wchar_t* inputNext = wstrSrc.data();
        char* outputNext = &result[0];
        char* outputEnd = &result[0] + result.size();
        if (converter.out(state, wstrSrc.data(), inputEnd, inputNext, &result[0], outputEnd, outputNext) !=
            std::codecvt_base::ok || inputNext != inputEnd)
        {
            return "";
        }
        char* finalNext = outputNext;
        const std::codecvt_base::result ending = converter.unshift(state, outputNext, outputEnd, finalNext);
        if (ending != std::codecvt_base::ok && ending != std::codecvt_base::noconv)
        {
            return "";
        }
        result.resize(static_cast<size_t>(finalNext - &result[0]));
        return result;
    }
    catch (...)
    {
        return "";
    }
#else
    return "";
#endif
}

std::wstring CStringManager::AnsiToUnicode(const std::string& strSrc)
{
#ifdef _WIN32
    return convertWindowsBytes(strSrc, CP_ACP);
#elif defined(__unix__) || defined(__APPLE__)
    try
    {
        if (strSrc.empty())
        {
            return L"";
        }
        const std::locale locale = createAnsiLocale();
        const std::codecvt<wchar_t, char, std::mbstate_t>& converter =
            std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(locale);
        // GBK每个输入字节至多产生一个宽字符，按长度转换保留内嵌NUL。
        std::wstring result(strSrc.size(), L'\0');
        std::mbstate_t state = {};
        const char* inputEnd = strSrc.data() + strSrc.size();
        const char* inputNext = strSrc.data();
        wchar_t* outputNext = &result[0];
        if (converter.in(state, strSrc.data(), inputEnd, inputNext, &result[0], &result[0] + result.size(), outputNext) !=
            std::codecvt_base::ok || inputNext != inputEnd)
        {
            return L"";
        }
        result.resize(static_cast<size_t>(outputNext - &result[0]));
        return result;
    }
    catch (...)
    {
        return L"";
    }
#else
    return L"";
#endif
}

std::string CStringManager::AnsiToUtf8(const std::string& strSrc)
{
    return UnicodeToUtf8(AnsiToUnicode(strSrc));
}

std::string CStringManager::Utf8ToAnsi(const std::string& strSrc)
{
    return UnicodeToAnsi(Utf8ToUnicode(strSrc));
}
#endif

std::string CStringManager::UnicodeToUtf8(const std::wstring& wstrSrc)
{
#ifdef _WIN32
    return convertWindowsWide(wstrSrc, CP_UTF8);
#elif defined(__unix__) || defined(__APPLE__)
    try
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(wstrSrc);
    }
    catch (...)
    {
        return "";
    }
#else
    return "";
#endif
}

std::wstring CStringManager::Utf8ToUnicode(const std::string& strSrc)
{
#ifdef _WIN32
    return convertWindowsBytes(strSrc, CP_UTF8);
#elif defined(__unix__) || defined(__APPLE__)
    try
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::wstring converted = converter.from_bytes(strSrc);
        // 某些标准库遇到末尾缺失的 UTF-8 续字节时只返回已转换前缀
        if (converter.converted() != strSrc.size())
        {
            return std::wstring();
        }
        return converted;
    }
    catch (...)
    {
        return L"";
    }
#else
    return L"";
#endif
}

/** 批量写入百分号转义结果，按入口选择允许原样保留的字节
@param [in] sIn 待编码的字节串
@return 使用大写十六进制数字的编码结果
*/
template<bool preservePath>
static std::string EncodeUrlBytes(const std::string& sIn)
{
    std::string output;
    output.reserve(sIn.size());
    char block[768];
    size_t used = 0;
    for (size_t index = 0; index < sIn.size(); ++index)
    {
        if (used > sizeof(block) - 3)
        {
            output.append(block, used);
            used = 0;
        }
        const unsigned char byte = static_cast<unsigned char>(sIn[index]);
        if ((byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z') ||
            (byte >= '0' && byte <= '9') ||
            (preservePath && std::strchr("~-_./:()'", byte) != nullptr))
        {
            block[used++] = sIn[index];
        }
        else
        {
            block[used++] = '%';
            block[used++] = kUpperHexDigits[byte >> 4];
            block[used++] = kUpperHexDigits[byte & 15];
        }
    }
    output.append(block, used);
    return output;
}

std::string CStringManager::UrlEncode(const std::string& sIn)
{
    return EncodeUrlBytes<false>(sIn);
}

std::string CStringManager::EncodeUrlPath(const std::string& path)
{
    return EncodeUrlBytes<true>(path);
}

void CStringManager::EncodeUnicodeUrl(std::wstring& path)
{
    const std::string encoded = EncodeUrlPath(UnicodeToUtf8(path));
    path.assign(encoded.begin(), encoded.end());
}

std::string CStringManager::UrlDecode(const std::string& sIn)
{
    std::string output(sIn);
    const size_t length = sIn.size();
    size_t written = 0;
    for (size_t read = 0; read < length; ++read)
    {
        char byte = sIn[read];
        if (byte == '%' && length - read >= 3)
        {
            const int high = CharToHex(sIn[read + 1]);
            const int low = CharToHex(sIn[read + 2]);
            if (high >= 0 && low >= 0)
            {
                byte = static_cast<char>((high << 4) | low);
                read += 2;
            }
        }
        else if (byte == '+')
        {
            byte = ' ';
        }
        output[written++] = byte;
    }
    output.resize(written);
    return output;
}

void CStringManager::formatV(std::string& output, const char* format, va_list arguments)
{
    if (format == nullptr)
    {
        output.clear();
        return;
    }
    try
    {
        std::string result;
        formatNarrowArguments(result, format, arguments);
        output.swap(result);
    }
    catch (...)
    {
        output.clear();
    }
}

void CStringManager::formatV(std::wstring& output, const wchar_t* format, va_list arguments)
{
    if (format == nullptr)
    {
        output.clear();
        return;
    }
    try
    {
        std::wstring result;
        formatWideArguments(result, format, arguments);
        output.swap(result);
    }
    catch (...)
    {
        output.clear();
    }
}

#ifdef _WIN32
std::string CStringManager::convertWindowsWide(const std::wstring& source, uint32_t codePage)
{
    if (source.empty() || source.size() > static_cast<size_t>((std::numeric_limits<int32_t>::max)()))
    {
        return "";
    }
    const int32_t length = static_cast<int32_t>(source.size());
    const int32_t required = WideCharToMultiByte(codePage, 0, source.data(), length, nullptr, 0, nullptr, nullptr);
    if (required <= 0)
    {
        return "";
    }
    try
    {
        std::string result(static_cast<size_t>(required), '\0');
        if (WideCharToMultiByte(codePage, 0, source.data(), length, &result[0], required, nullptr, nullptr) != required)
        {
            return "";
        }
        return result;
    }
    catch (...)
    {
        return "";
    }
}
#endif

#ifdef _WIN32
std::wstring CStringManager::convertWindowsBytes(const std::string& source, uint32_t codePage)
{
    if (source.empty() || source.size() > static_cast<size_t>((std::numeric_limits<int32_t>::max)()))
    {
        return L"";
    }
    const int32_t length = static_cast<int32_t>(source.size());
    const int32_t required = MultiByteToWideChar(codePage, 0, source.data(), length, nullptr, 0);
    if (required <= 0)
    {
        return L"";
    }
    try
    {
        std::wstring result(static_cast<size_t>(required), L'\0');
        if (MultiByteToWideChar(codePage, 0, source.data(), length, &result[0], required) != required)
        {
            return L"";
        }
        return result;
    }
    catch (...)
    {
        return L"";
    }
}
#endif

#if CSTRING_MANAGER_HAS_ANSI && !defined(_WIN32)
std::locale CStringManager::createAnsiLocale()
{
    // locale 接管 facet 并在最后一份引用销毁时释放
    return std::locale(std::locale::classic(), new std::codecvt_byname<wchar_t, char, std::mbstate_t>("zh_CN.GBK"));
}
#endif

int CStringManager::CharToHex(char digit)
{
	if (digit >= '0' && digit <= '9')
	{
		return digit - '0';
	}
	if (digit >= 'a' && digit <= 'f')
	{
		return digit - 'a' + 10;
	}
	if (digit >= 'A' && digit <= 'F')
	{
		return digit - 'A' + 10;
	}
	return -1;
}

void CStringManager::formatNarrowArguments(std::string& output, const char* format, va_list arguments)
{
    va_list copy;
#ifdef _WIN32
    copy = arguments;
    const int32_t required = _vscprintf(format, copy);
#else
    va_copy(copy, arguments);
    const int32_t required = vsnprintf(nullptr, 0, format, copy);
#endif
    va_end(copy);
    if (required < 0)
    {
        output.clear();
        return;
    }
    output.resize(static_cast<size_t>(required) + 1);
#ifdef _WIN32
    copy = arguments;
    const int32_t written = vsprintf_s(&output[0], output.size(), format, copy);
#else
    va_copy(copy, arguments);
    const int32_t written = vsnprintf(&output[0], output.size(), format, copy);
#endif
    va_end(copy);
    if (written < 0 || static_cast<size_t>(written) >= output.size())
    {
        output.clear();
        return;
    }
    output.resize(static_cast<size_t>(written));
}

void CStringManager::formatWideArguments(std::wstring& output, const wchar_t* format, va_list arguments)
{
    va_list copy;
#ifdef _WIN32
    copy = arguments;
    const int32_t required = _vscwprintf(format, copy);
    va_end(copy);
    if (required < 0)
    {
        output.clear();
        return;
    }
    output.resize(static_cast<size_t>(required) + 1);
    copy = arguments;
    const int32_t written = vswprintf_s(&output[0], output.size(), format, copy);
    va_end(copy);
    if (written < 0 || static_cast<size_t>(written) >= output.size())
    {
        output.clear();
        return;
    }
    output.resize(static_cast<size_t>(written));
#else
    // POSIX不要求vswprintf(nullptr, 0, ...)返回所需长度；保留原参数表，按需扩容。
    size_t capacity = 256;
    const size_t maximum = (std::min)(output.max_size(), static_cast<size_t>(INT32_MAX));
    while (capacity <= maximum)
    {
        output.resize(capacity);
        va_copy(copy, arguments);
        errno = 0;
        const int32_t written = vswprintf(&output[0], output.size(), format, copy);
        const int error = errno;
        va_end(copy);
        if (written >= 0 && static_cast<size_t>(written) < capacity)
        {
            output.resize(static_cast<size_t>(written));
            return;
        }
        if ((written < 0 && error != 0 && error != EOVERFLOW && error != E2BIG) || capacity == maximum)
        {
            break;
        }
        // 截断可只返回负值，也可报告EOVERFLOW/E2BIG；容量不足继续扩容，编码或格式错误不重试。
        capacity = written >= 0 ? static_cast<size_t>(written) + 1 :
            (capacity > maximum / 2 ? maximum : capacity * 2);
    }
    output.clear();
#endif
}

// 使用示例：在本翻译单元末尾启用以下 main，单独构建控制台程序
// int main()
// {
//     const std::string title = "Track(01)";
//     const std::string label = CStringManager::GetMidString(title, "(", ")");
//     if (CStringManager::FindOther(title, '(', ')', 5) != 8 || CStringManager::atoi64(label.c_str()) != 1)
//     {
//         return 1;
//     }
//     const std::vector<std::string> fields = CStringManager::split("album::track::", "::");
//     std::vector<std::string> tags;
//     CStringManager::split(tags, "live,stereo", ',');
//     const std::vector<std::wstring> wideTags = CStringManager::split(L"live;stereo", L";");
//     std::string path = CStringManager::Left(title, 5) + CStringManager::Right("0001", 2);
//     CStringManager::Insert(path, path.size(), '.');
//     CStringManager::Insert(path, path.size(), "audio");
//     CStringManager::Delete(path, CStringManager::Find(path, "audio"), 5);
//     path += "RAW";
//     CStringManager::Replace(path, "RAW", "wav");
//     CStringManager::Replace(path, 'T', 't');
//     CStringManager::ReplaceAll(path, "track", "sample");
//     const std::string stem = CStringManager::Mid(path, 0, CStringManager::ReserveFind(path, '.'));
//     const std::string upper = CStringManager::MakeUpper(stem);
//     std::string reversed = CStringManager::MakeLower(upper);
//     CStringManager::MakeReverse(reversed);
//     const std::wstring caption = CStringManager::Utf8ToUnicode(path);
//     std::vector<uint8_t> utf16;
//     std::wstring decoded;
//     if (!CStringManager::wideToUtf16Le(caption, utf16) || !CStringManager::utf16LeToWide(utf16, decoded))
//     {
//         return 2;
//     }
//     std::string uri = CStringManager::UrlEncode(CStringManager::UnicodeToUtf8(decoded));
//     std::wstring mediaPath = L"/live/a b";
//     CStringManager::EncodeUnicodeUrl(mediaPath);
//     const std::string packet("\0\xFA", 2);
//     if (mediaPath != L"/live/a%20b" || CStringManager::EncodeUrlPath("/a?b") != "/a%3Fb" ||
//         CStringManager::HexToStr(CStringManager::StrToHex(packet)) != packet ||
//         CStringManager::StrToInt(CStringManager::IntToStr(-42, true), true) != -42 ||
//         CStringManager::StrFormat("%s:%s", label.c_str(), CStringManager::Char2Hex(250).c_str()) != "01:FA")
//     {
//         return 3;
//     }
//     std::string displayName = path;
//     if (CStringManager::Replace(displayName, displayName, "Live recording") != 1 || displayName != "Live recording")
//     {
//         return 4;
//     }
//     std::string message;
//     CStringManager::Format(message, "%s [%s]", uri.c_str(), tags.front().c_str());
//     std::wstring formatted;
//     CStringManager::Format(formatted, L"%ls", caption.c_str());
//     char buffer[64] = {};
//     const size_t required = CStringManager::CopyToBuffer(message, buffer, sizeof(buffer));
//     std::printf("%s\n", buffer);
//     if (CStringManager::Find(path, '.', 0) != 8 || CStringManager::CharToHex('F') != 15 ||
//         CStringManager::MakeLower(CStringManager::MakeUpper(std::wstring(L"wav"))) != L"wav")
//     {
//         return 4;
//     }
//     return fields.size() == 3 && wideTags.size() == 2 && path == "sample01.wav" &&
//         CStringManager::UrlDecode(uri) == path && formatted == caption &&
//         CStringManager::GetLength(message) + 1 == required && reversed == "10elpmas" &&
//         CStringManager::atoui64("42") == 42 && CStringManager::parseInt32(L"42", 0) == 42 &&
//         CStringManager::toStringInt32(42) == CStringManager::toStringInt64(42) &&
//         CStringManager::parseFiniteDouble(CStringManager::formatFixedDouble(1.25, 2), 0.0) == 1.25 ? 0 : 3;
// }