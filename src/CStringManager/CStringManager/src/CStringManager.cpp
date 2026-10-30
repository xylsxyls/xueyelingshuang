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
#include <new>
#include <sstream>
#include <cmath>
#include <locale>
#include <limits>
#ifdef _WIN32
#include <windows.h>
#else
#include <string.h>
#include <stdarg.h>
#include <codecvt>
#include <locale>
#endif
#if (_MSC_VER == 1900)
#include <iterator>
#endif

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
            std::memcpy(buffer, value.data(), length);
        }
        buffer[length] = '\0';
    }
    return value.size() + 1;
}

size_t CStringManager::FindOther(const std::string& str, char cLeft, char cRight, size_t nSelect)
{
	std::vector<size_t> vecn;
	size_t nSelectSize = 0;
	size_t index = 0;
	while (index < str.size())
	{
		if (str[index] == cLeft)
		{
			vecn.push_back(index);
			//?nSelect在左，记下现在的nSelectSize
			if (nSelect == index)
			{
				nSelectSize = vecn.size();
			}
		}
		if (str[index] == cRight)
		{
			if(nSelect == index)
			{
				if (vecn.empty())
				{
					return std::string::npos;
				}
				return vecn.at(vecn.size() - 1);
			}
			//?如果弹出之前发现size大小等于之前记录的，说明找到了对应的右侧
			if(vecn.size() == nSelectSize && nSelectSize > 0)
			{
				return index;
			}
			if (vecn.size() > 0)
			{
				vecn.pop_back();
			}
		}
		++index;
	}
	return std::string::npos;
}

std::vector<std::string> CStringManager::split(const std::string& splitString, const std::string& separate_character)
{
	std::vector<std::string> strs;
	if (separate_character.empty())
	{
		strs.emplace_back(splitString);
		return strs;
	}
	//?分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	size_t separate_characterLen = separate_character.length();
	size_t splitCount = 1;
	size_t lastPosition = 0;
	size_t index = splitString.find(separate_character, lastPosition);
	while (index != std::string::npos)
	{
		++splitCount;
		lastPosition = index + separate_characterLen;
		index = splitString.find(separate_character, lastPosition);
	}
	strs.reserve(splitCount);
	lastPosition = 0;
	index = splitString.find(separate_character, lastPosition);
	while (index != std::string::npos)
	{
		strs.emplace_back(splitString, lastPosition, index - lastPosition);
		lastPosition = index + separate_characterLen;
		index = splitString.find(separate_character, lastPosition);
	}
	//?截取最后一个分隔符后的内容
	//?if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	strs.emplace_back(splitString, lastPosition);
	return strs;
}

std::vector<std::wstring> CStringManager::split(const std::wstring& splitString, const std::wstring& separate_character)
{
	std::vector<std::wstring> strs;
	if (separate_character.empty())
	{
		strs.emplace_back(splitString);
		return strs;
	}
	//?分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	size_t separate_characterLen = separate_character.length();
	size_t splitCount = 1;
	size_t lastPosition = 0;
	size_t index = splitString.find(separate_character, lastPosition);
	while (index != std::wstring::npos)
	{
		++splitCount;
		lastPosition = index + separate_characterLen;
		index = splitString.find(separate_character, lastPosition);
	}
	strs.reserve(splitCount);
	lastPosition = 0;
	index = splitString.find(separate_character, lastPosition);
	while (index != std::wstring::npos)
	{
		strs.emplace_back(splitString, lastPosition, index - lastPosition);
		lastPosition = index + separate_characterLen;
		index = splitString.find(separate_character, lastPosition);
	}
	//?截取最后一个分隔符后的内容
	//?if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	strs.emplace_back(splitString, lastPosition);
	return strs;
}

void CStringManager::split(std::vector<std::string>& result, const std::string& splitString, char separate_character)
{
	result.clear();
	size_t splitCount = 1;
	size_t lastPosition = 0;
	size_t index = splitString.find(separate_character, lastPosition);
	while (index != std::string::npos)
	{
		++splitCount;
		lastPosition = index + 1;
		index = splitString.find(separate_character, lastPosition);
	}
	result.reserve(splitCount);
	lastPosition = 0;
	index = splitString.find(separate_character, lastPosition);
	while (index != std::string::npos)
	{
		result.emplace_back(splitString, lastPosition, index - lastPosition);
		lastPosition = index + 1;
		index = splitString.find(separate_character, lastPosition);
	}
	result.emplace_back(splitString, lastPosition);
}

void CStringManager::ReplaceAll(std::string& str, const std::string& old_value, const std::string& new_value)
{
	if (old_value.empty())
	{
		return;
	}
	if (new_value.find(old_value) != std::string::npos)
	{
		Replace(str, old_value, new_value);
		return;
	}
	while (true)
	{
		std::string::size_type pos(0);
		if ((pos = str.find(old_value)) == std::string::npos)
		{
			break;
		}
		str.replace(pos, old_value.length(), new_value);
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

size_t CStringManager::Replace(std::string& str, const std::string& oldstr, const std::string& newstr)
{
	if (oldstr.empty())
	{
		return 0;
	}
	size_t count = 0;
	size_t pos = 0;
	while (true)
	{
		pos = str.find(oldstr, pos);
		if (pos == std::string::npos)
		{
			break;
		}
		str.replace(pos, oldstr.length(), newstr);
		pos += newstr.length();
		++count;
	}
	return count;
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
	if (oldstr.empty())
	{
		return 0;
	}
	size_t count = 0;
	size_t pos = 0;
	while (true)
	{
		pos = str.find(oldstr, pos);
		if (pos == std::wstring::npos)
		{
			break;
		}
		str.replace(pos, oldstr.length(), newstr);
		pos += newstr.length();
		++count;
	}
	return count;
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

/** 使用独立输出完成窄格式化，调用者负责保留原参数至本函数结束
@param [out] output 临时结果，不能与fmt或可变参数指针重叠
@param [in] format 有效且可信的printf格式；参数类型须与格式匹配
@param [in] arguments 未消费的参数表；每次CRT调用都复制并配对结束
*/
static void FormatNarrowArguments(std::string& output, const char* format, va_list arguments)
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

/** 保持原生宽格式语义，不将宽文本或格式转成系统ANSI
@param [out] output 独立临时结果，不能与输入参数重叠
@param [in] format 有效且可信的宽printf格式；跨平台宽文本使用%ls
@param [in] arguments 未消费的参数表；所有复制均配对va_end
*/
static void FormatWideArguments(std::wstring& output, const wchar_t* format, va_list arguments)
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
        if ((written < 0 && error != 0) || capacity == maximum)
        {
            break;
        }
        // 截断可能仅报告负值且不设errno；编码或格式错误则不反复扩容。
        capacity = written >= 0 ? static_cast<size_t>(written) + 1 :
            (capacity > maximum / 2 ? maximum : capacity * 2);
    }
    output.clear();
#endif
}

void CStringManager::Format(std::string& str, const char* fmt, ...)
{
    if (fmt == nullptr)
    {
        str.clear();
        return;
    }
    va_list args;
    va_start(args, fmt);
    try
    {
        // fmt及%s参数可能借用str，格式化完成前不得resize或清空原输出。
        std::string output;
        FormatNarrowArguments(output, fmt, args);
        str.swap(output);
    }
    catch (...)
    {
        str.clear();
    }
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
        FormatNarrowArguments(result, fmt, args);
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
    if (fmt == nullptr)
    {
        str.clear();
        return;
    }
    va_list args;
    va_start(args, fmt);
    try
    {
        std::wstring output;
        FormatWideArguments(output, fmt, args);
        str.swap(output);
    }
    catch (...)
    {
        str.clear();
    }
    va_end(args);
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
        FormatWideArguments(result, fmt, args);
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
#if defined _MSC_VER && (_MSC_VER < 1800)
	return dst;
#endif
	//如果dst是有值的话则第三个参数传dst.begin()，从头开始覆盖
	std::transform(src.begin(), src.end(), std::back_inserter(dst),
		[](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
	return dst;
}

std::string CStringManager::MakeLower(const std::string& src)
{
	std::string dst;
#if defined _MSC_VER && (_MSC_VER < 1800)
	return dst;
#endif
	std::transform(src.begin(), src.end(), std::back_inserter(dst),
		[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	return dst;
}

std::wstring CStringManager::MakeUpper(const std::wstring& src)
{
	std::wstring dst;
#if defined _MSC_VER && (_MSC_VER < 1800)
	return dst;
#endif
	std::transform(src.begin(), src.end(), std::back_inserter(dst),
		[](wchar_t ch) { return static_cast<wchar_t>(::towupper(ch)); });
	return dst;
}

std::wstring CStringManager::MakeLower(const std::wstring& src)
{
	std::wstring dst;
#if defined _MSC_VER && (_MSC_VER < 1800)
	return dst;
#endif
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
	if (right == std::string::npos || right < begin)
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
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

std::string CStringManager::toStringInt64(int64_t value)
{
	std::ostringstream oss;
	oss << value;
	return oss.str();
}

#ifdef _WIN32
/** 先向Windows查询精确字节数，再直接写入拥有结果的字符串
@param [in] source 调用期间借用的宽字符串，允许内嵌零字符
@param [in] codePage CP_UTF8或CP_ACP，保留系统原有替代字符规则
@return 完整转换结果；输入无效或转换失败返回空；无法构造返回字符串时仍可抛出分配异常
*/
static std::string ConvertWindowsWide(const std::wstring& source, uint32_t codePage)
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

/** 直接将Windows转换结果写入唯一拥有缓冲的宽字符串
@param [in] source 调用期间借用的字节串，允许内嵌零字符
@param [in] codePage CP_UTF8或CP_ACP，沿用flags=0的系统行为
@return 完整转换结果；输入无效或转换失败返回空；无法构造返回字符串时仍可抛出分配异常
*/
static std::wstring ConvertWindowsBytes(const std::string& source, uint32_t codePage)
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
#elif defined(__unix__) || defined(__APPLE__)
std::locale CStringManager::createAnsiLocale()
{
    // locale拥有facet，通过标准库释放；不再为公开析构权限单独派生辅助类。
    return std::locale(std::locale::classic(), new std::codecvt_byname<wchar_t, char, std::mbstate_t>("zh_CN.GBK"));
}
#endif

std::string CStringManager::UnicodeToAnsi(const std::wstring& wstrSrc)
{
#ifdef _WIN32
    return ConvertWindowsWide(wstrSrc, CP_ACP);
#elif defined(__unix__) || defined(__APPLE__)
    try
    {
        if (wstrSrc.empty()) return "";
        const std::locale locale = createAnsiLocale();
        const std::codecvt<wchar_t, char, std::mbstate_t>& converter =
            std::use_facet<std::codecvt<wchar_t, char, std::mbstate_t>>(locale);
        std::string result;
        const int32_t maximum = converter.max_length();
        // GBK每个宽字符最多maximum字节，额外预留一次结束转换的空间；先检查乘法溢出。
        if (maximum <= 0 || wstrSrc.size() >= result.max_size() / static_cast<size_t>(maximum)) return "";
        result.resize((wstrSrc.size() + 1) * static_cast<size_t>(maximum));
        std::mbstate_t state = {};
        const wchar_t* inputEnd = wstrSrc.data() + wstrSrc.size();
        const wchar_t* inputNext = wstrSrc.data();
        char* outputNext = &result[0];
        char* outputEnd = &result[0] + result.size();
        if (converter.out(state, wstrSrc.data(), inputEnd, inputNext, &result[0], outputEnd, outputNext) !=
            std::codecvt_base::ok || inputNext != inputEnd) return "";
        char* finalNext = outputNext;
        const std::codecvt_base::result ending = converter.unshift(state, outputNext, outputEnd, finalNext);
        if (ending != std::codecvt_base::ok && ending != std::codecvt_base::noconv) return "";
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
    return ConvertWindowsBytes(strSrc, CP_ACP);
#elif defined(__unix__) || defined(__APPLE__)
    try
    {
        if (strSrc.empty()) return L"";
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
            std::codecvt_base::ok || inputNext != inputEnd) return L"";
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

std::string CStringManager::UnicodeToUtf8(const std::wstring& wstrSrc)
{
#ifdef _WIN32
    return ConvertWindowsWide(wstrSrc, CP_UTF8);
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
    return ConvertWindowsBytes(strSrc, CP_UTF8);
#elif defined(__unix__) || defined(__APPLE__)
    try
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.from_bytes(strSrc);
    }
    catch (...)
    {
        return L"";
    }
#else
    return L"";
#endif
}

// 将0到15转换为URL编码使用的大写十六进制字符。
static unsigned char toHex(const unsigned char& x)
{
	return x > 9 ? x - 10 + 'A' : x + '0';
}

// 将URL编码里的十六进制字符转换为数值，非法字符返回0。
static unsigned char fromHex(const unsigned char& x)
{
	if (std::isdigit(x))
	{
		return x - '0';
	}
	if (x >= 'a' && x <= 'f')
	{
		return x - 'a' + 10;
	}
	if (x >= 'A' && x <= 'F')
	{
		return x - 'A' + 10;
	}
	return 0;
}

std::string CStringManager::UrlEncode(const std::string& sIn)
{
	std::string sOut;
	for( size_t ix = 0; ix < sIn.size(); ix++ )
	{
		unsigned char buf[4];
		::memset( buf, 0, 4 );
		if( isalnum( (unsigned char)sIn[ix] ) )
		{
			buf[0] = sIn[ix];
		}
		//else if ( isspace( (BYTE)sIn[ix] ) ) //貌似把空格编码成%20或者+都可以
		//{
		//	buf[0] = '+';
		//}
		else
		{
			buf[0] = '%';
			buf[1] = toHex( (unsigned char)sIn[ix] >> 4 );
			buf[2] = toHex( (unsigned char)sIn[ix] % 16);
		}
		sOut += (char *)buf;
	}
	return sOut;
}

std::string CStringManager::UrlDecode(const std::string& sIn)
{
	std::string sOut;
	for( size_t ix = 0; ix < sIn.size(); ix++ )
	{
		unsigned char ch = 0;
		if(sIn[ix]=='%' && ix + 2 < sIn.size() &&
			isxdigit(static_cast<unsigned char>(sIn[ix + 1])) &&
			isxdigit(static_cast<unsigned char>(sIn[ix + 2])))
		{
			ch = (fromHex(sIn[ix+1])<<4);
			ch |= fromHex(sIn[ix+2]);
			ix += 2;
		}
		else if(sIn[ix] == '+')
		{
			ch = ' ';
		}
		else
		{
			ch = sIn[ix];
		}
		sOut += (char)ch;
	}
	return sOut;
}

//#include <atomic>
//#include <functional>
//#include <iostream>
//#include <thread>
//
//int main()
//{
//	int32_t totalCount = 0;
//	int32_t failCount = 0;
//	std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
//	{
//		++totalCount;
//		std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
//		if (!ok)
//		{
//			++failCount;
//		}
//	};
//
//	check(CStringManager::FindOther("(a(b)c)", '(', ')', 0) == 6, "CStringManager FindOther left select");
//	check(CStringManager::FindOther("(a(b)c)", '(', ')', 4) == 2, "CStringManager FindOther right select");
//	check(CStringManager::FindOther("(abc", '(', ')', 0) == std::string::npos, "CStringManager FindOther missing right");
//	std::vector<std::string> splitResult = CStringManager::split("a,,b", ",");
//	check(splitResult.size() == 3 && splitResult[0] == "a" && splitResult[1].empty() && splitResult[2] == "b", "CStringManager split string");
//	std::vector<std::string> emptySplitResult = CStringManager::split("abc", "");
//	check(emptySplitResult.size() == 1 && emptySplitResult[0] == "abc", "CStringManager split empty separator");
//	std::vector<std::string> multiSplitResult = CStringManager::split("aa<>bb<><>", "<>");
//	check(multiSplitResult.size() == 4 && multiSplitResult[0] == "aa" && multiSplitResult[1] == "bb" &&
//		multiSplitResult[2].empty() && multiSplitResult[3].empty(), "CStringManager split multi separator");
//	std::vector<std::wstring> wideSplitResult = CStringManager::split(L"a|b|", L"|");
//	check(wideSplitResult.size() == 3 && wideSplitResult[0] == L"a" && wideSplitResult[1] == L"b" &&
//		wideSplitResult[2].empty(), "CStringManager split wide string");
//	std::vector<std::string> splitCharResult;
//	CStringManager::split(splitCharResult, "a,b,", ',');
//	check(splitCharResult.size() == 3 && splitCharResult[2].empty(), "CStringManager split char");
//	std::string replaceText = "aa-bb-aa";
//	check(CStringManager::Replace(replaceText, "aa", "cc") == 2 && replaceText == "cc-bb-cc", "CStringManager Replace string");
//	CStringManager::ReplaceAll(replaceText, "c", "d");
//	check(replaceText == "dd-bb-dd", "CStringManager ReplaceAll string");
//	std::string replaceEmptyOldText = "abc";
//	CStringManager::ReplaceAll(replaceEmptyOldText, "", "x");
//	check(replaceEmptyOldText == "abc", "CStringManager ReplaceAll empty old value");
//	std::string replaceShrinkText = "aaaa";
//	CStringManager::ReplaceAll(replaceShrinkText, "aa", "a");
//	check(replaceShrinkText == "a", "CStringManager ReplaceAll shrink loop");
//	std::string replaceGrowText = "a";
//	CStringManager::ReplaceAll(replaceGrowText, "a", "aa");
//	check(replaceGrowText == "aa", "CStringManager ReplaceAll contains old value");
//	check(CStringManager::Mid("abcdef", 2, 3) == "cde", "CStringManager Mid");
//	check(CStringManager::Mid("abc", 3, 1).empty(), "CStringManager Mid offset at end");
//	check(CStringManager::Left("abcdef", 2) == "ab", "CStringManager Left");
//	check(CStringManager::Right("abcdef", 2) == "ef", "CStringManager Right");
//	check(CStringManager::Right("abc", 99) == "abc", "CStringManager Right oversize");
//	check(CStringManager::Find("abcdef", "cd") == 2, "CStringManager Find string");
//	check(CStringManager::ReserveFind("ababa", "ba") == 3, "CStringManager ReserveFind string");
//	std::string editText = "abc";
//	CStringManager::Insert(editText, 1, "XX");
//	CStringManager::Delete(editText, 2, 1);
//	check(editText == "aXbc", "CStringManager Insert Delete");
//	CStringManager::Insert(editText, 999, "Z");
//	check(editText == "aXbcZ", "CStringManager Insert oversize offset");
//	check(CStringManager::Delete(editText, 999, 2) == editText.length(), "CStringManager Delete oversize offset");
//	check(CStringManager::Format("%s-%d", "value", 7) == "value-7", "CStringManager Format string");
//	check(CStringManager::Format("") == "", "CStringManager Format empty string");
//	check(CStringManager::Format(static_cast<const char*>(nullptr)) == "", "CStringManager Format null string");
//	std::string nullFormatText = "old";
//	CStringManager::Format(nullFormatText, static_cast<const char*>(nullptr));
//	check(nullFormatText.empty(), "CStringManager Format output null string");
//	check(CStringManager::Format(L"%ls-%d", L"value", 7) == L"value-7", "CStringManager Format wide string");
//	std::string reverseText = "abc";
//	CStringManager::MakeReverse(reverseText);
//	check(reverseText == "cba", "CStringManager MakeReverse");
//	check(CStringManager::MakeUpper("aBc") == "ABC", "CStringManager MakeUpper");
//	check(CStringManager::MakeLower("aBc") == "abc", "CStringManager MakeLower");
//	std::string highByteText;
//	highByteText.push_back(static_cast<char>(0xff));
//	check(CStringManager::MakeUpper(highByteText).size() == 1 && CStringManager::MakeLower(highByteText).size() == 1,
//		"CStringManager MakeUpper MakeLower high byte");
//	check(CStringManager::GetMidString("left[value]right", "[", "]") == "value", "CStringManager GetMidString");
//	check(CStringManager::GetMidString("left[[value]]right", "[[", "]]") == "value", "CStringManager GetMidString multi tag");
//	check(CStringManager::GetMidString("left[value", "[", "]").empty(), "CStringManager GetMidString missing right");
//	check(CStringManager::atoi64(nullptr) == 0, "CStringManager atoi64 null");
//	check(CStringManager::atoi64("-12345678901") == -12345678901LL, "CStringManager atoi64");
//	check(CStringManager::atoui64(nullptr) == 0, "CStringManager atoui64 null");
//	check(CStringManager::atoui64("12345678901") == 12345678901ULL, "CStringManager atoui64");
//	check(CStringManager::toStringInt32(-123) == "-123", "CStringManager toStringInt32");
//	check(CStringManager::toStringInt64(-12345678901LL) == "-12345678901", "CStringManager toStringInt64");
//	std::string urlEncoded = CStringManager::UrlEncode("a b+c");
//	check(CStringManager::UrlDecode(urlEncoded) == "a b+c", "CStringManager UrlEncode UrlDecode");
//	check(CStringManager::UrlDecode("%") == "%" && CStringManager::UrlDecode("%GZ") == "%GZ", "CStringManager UrlDecode invalid percent");
//	check(CStringManager::UrlDecode("%2f%2F") == "//", "CStringManager UrlDecode lower upper hex");
//	std::wstring unicodeText = CStringManager::Utf8ToUnicode("abc");
//	check(CStringManager::UnicodeToUtf8(unicodeText) == "abc", "CStringManager Utf8 Unicode");
//
//	std::atomic<int32_t> failedThreadCount(0);
//	std::vector<std::thread> threads;
//	for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
//	{
//		threads.push_back(std::thread([threadIndex, &failedThreadCount]() -> void
//		{
//			for (int32_t loopIndex = 0; loopIndex < 5000; ++loopIndex)
//			{
//				std::string value = CStringManager::Format("%d-%d", threadIndex, loopIndex);
//				std::vector<std::string> parts = CStringManager::split(value, "-");
//				if (parts.size() != 2 || CStringManager::toStringInt32(threadIndex) != parts[0])
//				{
//					failedThreadCount.fetch_add(1);
//					return;
//				}
//			}
//		}));
//	}
//	for (size_t i = 0; i < threads.size(); ++i)
//	{
//		threads[i].join();
//	}
//	check(failedThreadCount.load() == 0, "CStringManager multithread pressure");
//
//	std::cout << "CStringManager test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << std::endl;
//	return failCount == 0 ? 0 : 1;
//}

// CSTRING_CONVERSION_UNIT_BEGIN
// #include <CStringManager/CStringManagerAPI.h>
// #include <cstdio>
// #include <new>
// #ifdef _WIN32
// #include <windows.h>
// #endif
// #if defined(_WIN32) && defined(_DEBUG) && !defined(CSTRING_CONVERSION_NO_HEAP)
// #include <crtdbg.h>
// // 独立main串行调用编码接口，计数器不供其他线程访问。
// static int32_t s_conversionRejectAfter = -1;
// static uint32_t s_conversionAllocations = 0;
// static uint32_t s_conversionPayloadAllocations = 0;
// /** 对编码分配逐点持续拒绝，验证异常收敛和结果缓冲的唯一所有权
// @param [in] operation CRT操作，其他参数沿用CRT原型
// @return 非零允许分配，零拒绝
// */
// static int __cdecl ConversionMemoryHook(int operation, void*, size_t size, int block, long, const unsigned char*, int)
// {
//     if ((operation == _HOOK_ALLOC || operation == _HOOK_REALLOC) && block == _NORMAL_BLOCK)
//     {
//         ++s_conversionAllocations;
//         if (size >= 256)
//         {
//             ++s_conversionPayloadAllocations;
//         }
//         if (s_conversionRejectAfter == 0)
//         {
//             return 0;
//         }
//         if (s_conversionRejectAfter > 0)
//         {
//             --s_conversionRejectAfter;
//         }
//     }
//     return 1;
// }
// #endif
//
// /** 检查六种编码的内容、内嵌零、系统替代规则和低内存收敛
// @return 全部通过返回true
// */
// static bool CStringConversionAudit()
// {
//     uint32_t checks = 0;
// #define CONVERSION_CHECK(value) do { ++checks; if (!(value)) { std::printf("CString conversion line=%d FAIL\n", __LINE__); return false; } } while (0)
//     const std::string narrow(256, 'a');
//     const std::wstring wide(256, L'a');
//     CONVERSION_CHECK(CStringManager::AnsiToUnicode(narrow) == wide);
//     CONVERSION_CHECK(CStringManager::UnicodeToAnsi(wide) == narrow);
//     CONVERSION_CHECK(CStringManager::Utf8ToUnicode(narrow) == wide);
//     CONVERSION_CHECK(CStringManager::UnicodeToUtf8(wide) == narrow);
//     CONVERSION_CHECK(CStringManager::AnsiToUtf8(narrow) == narrow);
//     CONVERSION_CHECK(CStringManager::Utf8ToAnsi(narrow) == narrow);
//     CONVERSION_CHECK(CStringManager::AnsiToUnicode("").empty());
//     CONVERSION_CHECK(CStringManager::UnicodeToAnsi(L"").empty());
//     CONVERSION_CHECK(CStringManager::Utf8ToUnicode("").empty());
//     CONVERSION_CHECK(CStringManager::UnicodeToUtf8(L"").empty());
//     CONVERSION_CHECK(CStringManager::AnsiToUtf8("").empty());
//     CONVERSION_CHECK(CStringManager::Utf8ToAnsi("").empty());
//     const std::string zeroBytes("abc\0def", 7);
//     const std::wstring zeroWide(L"abc\0def", 7);
//     CONVERSION_CHECK(CStringManager::AnsiToUnicode(zeroBytes) == zeroWide);
//     CONVERSION_CHECK(CStringManager::UnicodeToAnsi(zeroWide) == zeroBytes);
//     CONVERSION_CHECK(CStringManager::Utf8ToUnicode(zeroBytes) == zeroWide);
//     CONVERSION_CHECK(CStringManager::UnicodeToUtf8(zeroWide) == zeroBytes);
//     CONVERSION_CHECK(CStringManager::AnsiToUtf8(zeroBytes) == zeroBytes);
//     CONVERSION_CHECK(CStringManager::Utf8ToAnsi(zeroBytes) == zeroBytes);
// #ifdef _WIN32
//     const wchar_t unicodeData[] = { L'A', 0, 0x4e2d, 0xd83d, 0xde00, L'Z' };
//     const std::wstring unicode(unicodeData, 6);
//     for (uint32_t mode = 0; mode < 2; ++mode)
//     {
//         const UINT codePage = mode == 0 ? CP_UTF8 : CP_ACP;
//         const int bytes = WideCharToMultiByte(codePage, 0, unicode.data(), 6, nullptr, 0, nullptr, nullptr);
//         CONVERSION_CHECK(bytes > 0);
//         std::string expected(static_cast<size_t>(bytes), '\0');
//         CONVERSION_CHECK(WideCharToMultiByte(codePage, 0, unicode.data(), 6, &expected[0], bytes, nullptr, nullptr) == bytes);
//         const std::string actual = mode == 0 ? CStringManager::UnicodeToUtf8(unicode) : CStringManager::UnicodeToAnsi(unicode);
//         CONVERSION_CHECK(actual == expected);
//         const int characters = MultiByteToWideChar(codePage, 0, expected.data(), bytes, nullptr, 0);
//         CONVERSION_CHECK(characters > 0);
//         std::wstring back(static_cast<size_t>(characters), L'\0');
//         CONVERSION_CHECK(MultiByteToWideChar(codePage, 0, expected.data(), bytes, &back[0], characters) == characters);
//         CONVERSION_CHECK((mode == 0 ? CStringManager::Utf8ToUnicode(expected) : CStringManager::AnsiToUnicode(expected)) == back);
//         if (mode == 0)
//         {
//             CONVERSION_CHECK(back == unicode);
//         }
//     }
//     // flags=0沿用系统替代规则；修复所有权时不暗中改成严格拒绝输入。
//     const std::string invalid("\xff\xc0\x80\0Z", 5);
//     wchar_t expectedInvalid[16] = {};
//     const int invalidCount = MultiByteToWideChar(CP_UTF8, 0, invalid.data(), 5, expectedInvalid, 16);
//     CONVERSION_CHECK(invalidCount > 0);
//     CONVERSION_CHECK(CStringManager::Utf8ToUnicode(invalid) == std::wstring(expectedInvalid, static_cast<size_t>(invalidCount)));
// #endif
// #if defined(_WIN32) && defined(_DEBUG) && !defined(CSTRING_CONVERSION_NO_HEAP)
//     uint32_t rejected = 0;
//     uint32_t accepted = 0;
//     uint32_t allocatorExceptions = 0;
//     for (uint32_t mode = 0; mode < 6; ++mode)
//     {
//         for (int32_t position = 0; position < 8; ++position)
//         {
//             _CrtMemState before, after;
//             _CrtMemCheckpoint(&before);
//             bool escaped = false;
//             bool allocationFailed = false;
//             bool full = false;
//             bool empty = false;
//             s_conversionRejectAfter = position;
//             s_conversionAllocations = 0;
//             s_conversionPayloadAllocations = 0;
//             const _CRT_ALLOC_HOOK old = _CrtSetAllocHook(ConversionMemoryHook);
//             try
//             {
//                 if (mode == 0 || mode == 2)
//                 {
//                     std::wstring result;
//                     if (mode == 0)
//                     {
//                         result = CStringManager::Utf8ToUnicode(narrow);
//                     }
//                     else
//                     {
//                         result = CStringManager::AnsiToUnicode(narrow);
//                     }
//                     full = result == wide;
//                     empty = result.empty();
//                 }
//                 else
//                 {
//                     std::string result;
//                     if (mode == 1)
//                     {
//                         result = CStringManager::UnicodeToUtf8(wide);
//                     }
//                     else if (mode == 3)
//                     {
//                         result = CStringManager::UnicodeToAnsi(wide);
//                     }
//                     else if (mode == 4)
//                     {
//                         result = CStringManager::AnsiToUtf8(narrow);
//                     }
//                     else
//                     {
//                         result = CStringManager::Utf8ToAnsi(narrow);
//                     }
//                     full = result == narrow;
//                     empty = result.empty();
//                 }
//             }
//             catch (const std::bad_alloc&)
//             {
//                 // VS2013 Debug连空字符串的迭代器代理都需要分配；边界捕获此分配异常。
//                 allocationFailed = true;
//             }
//             catch (...)
//             {
//                 escaped = true;
//             }
//             _CrtSetAllocHook(old);
//             s_conversionRejectAfter = -1;
//             _CrtMemCheckpoint(&after);
//             if (escaped || (!full && !empty && !allocationFailed))
//             {
//                 std::printf("CSTRING_FAULT mode=%u position=%d escaped=%u allocations=%u\n", mode, position, escaped ? 1U : 0U, s_conversionAllocations);
//             }
//             CONVERSION_CHECK(!escaped && (full || empty || allocationFailed));
//             if (before.lCounts[_NORMAL_BLOCK] != after.lCounts[_NORMAL_BLOCK] || before.lSizes[_NORMAL_BLOCK] != after.lSizes[_NORMAL_BLOCK])
//             {
//                 std::printf("CSTRING_HEAP mode=%u position=%d blocks=%lld bytes=%lld allocation_failed=%u\n", mode, position,
//                     static_cast<long long>(after.lCounts[_NORMAL_BLOCK]) - static_cast<long long>(before.lCounts[_NORMAL_BLOCK]),
//                     static_cast<long long>(after.lSizes[_NORMAL_BLOCK]) - static_cast<long long>(before.lSizes[_NORMAL_BLOCK]), allocationFailed ? 1U : 0U);
//             }
//             CONVERSION_CHECK(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK]);
//             if (full)
//             {
//                 ++accepted;
//                 CONVERSION_CHECK(s_conversionPayloadAllocations == (mode < 4 ? 1U : 2U));
//             }
//             else
//             {
//                 ++rejected;
//                 allocatorExceptions += allocationFailed ? 1U : 0U;
//             }
//         }
//     }
//     CONVERSION_CHECK(accepted > 0 && rejected >= 8);
//     std::printf("CSTRING_CONVERSION allocation_positions=48 accepted=%u rejected=%u allocator_exceptions=%u heap_delta=0 direct_payload_allocations=1\n", accepted, rejected, allocatorExceptions);
// #endif
//     std::printf("CString conversion checks=%u PASS\n", checks);
//     return true;
// #undef CONVERSION_CHECK
// }
//
// #ifndef CSTRING_CONVERSION_NO_MAIN
// /** 独立回归入口；启用前解除整段注释，测试后恢复
// @return 零表示通过
// */
// int main()
// {
//     try
//     {
//         return CStringConversionAudit() ? 0 : 1;
//     }
//     catch (...)
//     {
//         std::puts("CString conversion unexpected exception FAIL");
//         return 2;
//     }
// }
// #endif
// CSTRING_CONVERSION_UNIT_END

#if 0 // CSTRING_FORMAT_UNIT_BEGIN
#include <CStringManager/CStringManagerAPI.h>
#include <cstdio>
#include <new>
#include <clocale>
#if defined(_WIN32) && defined(_DEBUG) && !defined(CSTRING_FORMAT_NO_HEAP)
#include <crtdbg.h>
// 单线程main的持续故障位置，不与后台任务或其他CRT注入并行。
static uint32_t s_formatRemaining = 0;
static uint32_t s_formatRejected = 0;
/** 从指定普通堆分配开始持续拒绝，其他参数遵循CRT原型
@param [in] operation 本次操作
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
#if defined(_WIN32) && defined(_DEBUG) && !defined(CSTRING_FORMAT_NO_HEAP)
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
            {
                std::string output;
                std::wstring wideOutput;
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
                _CrtSetAllocHook(old);
                valid = (output.empty() || output == expected) && (wideOutput.empty() || wideOutput == wideExpected);
                if (s_formatRejected != 0) { ++rejected; }
                else { ++accepted; }
            }
            _CrtMemState after = {};
            _CrtMemCheckpoint(&after);
            FORMAT_CHECK(valid && (!escaped || mode >= 2));
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

#ifndef CSTRING_FORMAT_NO_MAIN
/** 独立测试入口，正式库保留整段注释
@return 零表示成功
*/
int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
#ifndef _WIN32
    // ?CRT????????????locale????main????UTF-8 locale?
    if (std::setlocale(LC_CTYPE, "C.UTF-8") == nullptr)
    {
        std::puts("UTF-8 test locale unavailable");
        return 1;
    }
#endif
    return CStringFormatAudit() ? 0 : 1;
}
#endif

#endif // CSTRING_FORMAT_UNIT_END