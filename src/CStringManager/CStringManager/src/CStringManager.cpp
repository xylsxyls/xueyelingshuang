#include "CStringManager.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cwctype>
#include <new>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#include <string.h>
#include <stdarg.h>
#include <codecvt>
#include <locale>
#endif
#if (_MSC_VER == 1900)
#include <iterator>
#endif

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

void CStringManager::Format(std::string& str, const char* fmt, ...)
{
	if (fmt == nullptr)
	{
		str.clear();
		return;
	}
	va_list args;
	va_start(args, fmt);
#ifdef _WIN32
	va_list argcopy = args;
	int32_t size = static_cast<int32_t>(_vscprintf(fmt, argcopy));
#elif __unix__
	va_list argcopy;
	va_copy(argcopy, args);
	int32_t size = static_cast<int32_t>(vsnprintf(nullptr, 0, fmt, argcopy));
	va_end(argcopy);
#endif
	if (size < 0)
	{
		str.clear();
		va_end(args);
		return;
	}
	str.resize(static_cast<size_t>(size) + 1);
#ifdef _WIN32
	int32_t writeSize = static_cast<int32_t>(vsprintf_s(&str[0], str.size(), fmt, args));
#elif __unix__
	int32_t writeSize = static_cast<int32_t>(vsnprintf(&str[0], str.size(), fmt, args));
#endif
	if (writeSize < 0)
	{
		str.clear();
		va_end(args);
		return;
	}
	str.resize(static_cast<size_t>(writeSize));
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
#ifdef _WIN32
	va_list argcopy = args;
	int32_t size = static_cast<int32_t>(_vscprintf(fmt, argcopy));
#elif __unix__
	va_list argcopy;
	va_copy(argcopy, args);
	int32_t size = static_cast<int32_t>(vsnprintf(nullptr, 0, fmt, argcopy));
	va_end(argcopy);
#endif
	if (size < 0)
	{
		va_end(args);
		return result;
	}
	result.resize(static_cast<size_t>(size) + 1);
#ifdef _WIN32
	int32_t writeSize = static_cast<int32_t>(vsprintf_s(&result[0], result.size(), fmt, args));
#elif __unix__
	int32_t writeSize = static_cast<int32_t>(vsnprintf(&result[0], result.size(), fmt, args));
#endif
	if (writeSize < 0)
	{
		result.clear();
		va_end(args);
		return result;
	}
	result.resize(static_cast<size_t>(writeSize));
	va_end(args);
	return result;
}

void CStringManager::Format(std::wstring& wstr, const wchar_t* wfmt, ...)
{
	if (wfmt == nullptr)
	{
		wstr.clear();
		return;
	}
#ifdef _WIN32
	va_list args;
	va_start(args, wfmt);
	va_list argcopy = args;
	int32_t size = static_cast<int32_t>(_vscwprintf(wfmt, argcopy));
	if (size < 0)
	{
		wstr.clear();
		va_end(args);
		return;
	}
	wstr.resize(static_cast<size_t>(size) + 1);
	int32_t writeSize = static_cast<int32_t>(vswprintf_s(&wstr[0], wstr.size(), wfmt, args));
	if (writeSize < 0)
	{
		wstr.clear();
		va_end(args);
		return;
	}
	wstr.resize(static_cast<size_t>(writeSize));
	va_end(args);
#elif __unix__
	//linux下unicode参数传入了vswprintf也无法识别，传入ansi参数在vswprintf下也会自动转成unicode，所以可以整体转换
	//vswprintf函数无法在不提供长度时获取缓冲区长度
	std::string strFmt = UnicodeToAnsi(wfmt);
	std::string str;
	va_list args;
	va_start(args, wfmt);
	va_list argcopy;
	va_copy(argcopy, args);
	int32_t size = static_cast<int32_t>(vsnprintf(nullptr, 0, strFmt.c_str(), argcopy));
	va_end(argcopy);
	if (size < 0)
	{
		va_end(args);
		wstr.clear();
		return;
	}
	str.resize(static_cast<size_t>(size) + 1);
	int32_t writeSize = static_cast<int32_t>(vsnprintf(&str[0], str.size(), strFmt.c_str(), args));
	if (writeSize < 0)
	{
		va_end(args);
		wstr.clear();
		return;
	}
	str.resize(static_cast<size_t>(writeSize));
	va_end(args);
	wstr = AnsiToUnicode(str);
#endif
}

std::wstring CStringManager::Format(const wchar_t* wfmt, ...)
{
	if (wfmt == nullptr)
	{
		return std::wstring();
	}
#ifdef _WIN32
	std::wstring result;
	va_list args;
	va_start(args, wfmt);
	va_list argcopy = args;
	int32_t size = static_cast<int32_t>(_vscwprintf(wfmt, argcopy));
	if (size < 0)
	{
		va_end(args);
		return result;
	}
	result.resize(static_cast<size_t>(size) + 1);
	int32_t writeSize = static_cast<int32_t>(vswprintf_s(&result[0], result.size(), wfmt, args));
	if (writeSize < 0)
	{
		result.clear();
		va_end(args);
		return result;
	}
	result.resize(static_cast<size_t>(writeSize));
	va_end(args);
	return result;
#elif __unix__
	//linux下unicode参数传入了vswprintf也无法识别，传入ansi参数在vswprintf下也会自动转成unicode，所以可以整体转换
	//vswprintf函数无法在不提供长度时获取缓冲区长度
	std::string strFmt = UnicodeToAnsi(wfmt);
	std::string str;
	va_list args;
	va_start(args, wfmt);
	va_list argcopy;
	va_copy(argcopy, args);
	int32_t size = static_cast<int32_t>(vsnprintf(nullptr, 0, strFmt.c_str(), argcopy));
	va_end(argcopy);
	if (size < 0)
	{
		va_end(args);
		return std::wstring();
	}
	str.resize(static_cast<size_t>(size) + 1);
	int32_t writeSize = static_cast<int32_t>(vsnprintf(&str[0], str.size(), strFmt.c_str(), args));
	if (writeSize < 0)
	{
		va_end(args);
		return std::wstring();
	}
	str.resize(static_cast<size_t>(writeSize));
	va_end(args);
	return AnsiToUnicode(str);
#endif
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

#ifdef __unix__

class chs_codecvt : public std::codecvt_byname<wchar_t, char, std::mbstate_t>
{
public:
	//zh_CN.GBK or .936
	chs_codecvt():
		codecvt_byname("zh_CN.GBK")
	{

	}
};

// 转换字符集，outlen传入输出缓冲容量，返回实际输出长度。
static int32_t codeConvert(const char* from_charset, const char* to_charset, const char* inbuf, size_t inlen, char* outbuf, size_t* outlen)
{
	iconv_t cd = iconv_open(to_charset, from_charset);
	if (cd == (iconv_t)-1)
	{
		return -1;
	}

	char* input = const_cast<char*>(inbuf);
	char* output = outbuf;
	char** pin = &input;
	char** pout = &output;
	size_t outLength = *outlen;
	if (iconv(cd, pin, &inlen, pout, outlen) == -1)
	{
		iconv_close(cd);
		return -1;
	}
	*outlen = outLength - *outlen;
	iconv_close(cd);
	return 0;
}

// 按指定字符集转换字符串，转换失败返回空字符串。
static std::string codeConvertString(const char* from_charset, const char* to_charset, const std::string& src)
{
	if (src.empty())
	{
		return "";
	}

	size_t outLength = src.size() * 6;
	char* out = new (std::nothrow) char[outLength];
	if (out == nullptr)
	{
		return "";
	}
	if (codeConvert(from_charset, to_charset, src.c_str(), src.size(), out, &outLength) != 0)
	{
		delete[] out;
		return "";
	}
	std::string result(out, outLength);
	delete[] out;
	return result;
}

#endif

std::string CStringManager::UnicodeToAnsi(const std::wstring& wstrSrc)
{
#ifdef _WIN32
	// 分配目标空间, 一个16位Unicode字符最多可以转为4个字节
	int iAllocSize = static_cast<int>(wstrSrc.size() * 4 + 10);
	char* pwszBuffer = new char[iAllocSize];
	if (NULL == pwszBuffer)
	{
		return "";
	}
	int iCharsRet = ::WideCharToMultiByte(CP_ACP, 0, wstrSrc.c_str(),
		static_cast<int>(wstrSrc.size()),
		pwszBuffer, iAllocSize, NULL, NULL);
	std::string strRet;
	if (0 < iCharsRet)
	{
		strRet.assign(pwszBuffer, static_cast<size_t>(iCharsRet));
	}

	delete[] pwszBuffer;
	return strRet;
#elif __unix__
	std::string str;
	try
	{
		std::wstring_convert<chs_codecvt> converter;
		str = converter.to_bytes(wstrSrc);
	}
	catch (const std::exception & e)
	{
		//std::cout << e.what() << std::endl;
		return str;
	}
	return str;
#endif
}

std::wstring CStringManager::AnsiToUnicode(const std::string& strSrc)
{
#ifdef _WIN32
	// 分配目标空间
	int iAllocSize = static_cast<int>(strSrc.size() + 10);
	WCHAR* pwszBuffer = new WCHAR[iAllocSize];
	if (NULL == pwszBuffer)
	{
		return L"";
	}
	int iCharsRet = MultiByteToWideChar(CP_ACP, 0, strSrc.c_str(),
		static_cast<int>(strSrc.size()),
		pwszBuffer, iAllocSize);
	std::wstring wstrRet;
	if (0 < iCharsRet)
	{
		wstrRet.assign(pwszBuffer, static_cast<size_t>(iCharsRet));
	}

	delete[] pwszBuffer;
	return wstrRet;
#elif __unix__
	std::wstring wstr;
	try
	{
		std::wstring_convert<chs_codecvt> converter;
		wstr = converter.from_bytes(strSrc);
	}
	catch (const std::exception & e)
	{
		//std::cout << e.what() << std::endl;
		return wstr;
	}
	return wstr;
#endif
}

std::string CStringManager::AnsiToUtf8(const std::string& strSrc)
{
#ifdef _WIN32
	// 分配目标空间, 长度为 Ansi 编码的两倍
	int iAllocSize = static_cast<int>(strSrc.size() * 2 + 10);
	WCHAR* pwszBuffer = new WCHAR[iAllocSize];
	if (NULL == pwszBuffer)
	{
		return "";
	}
	int iCharsRet = MultiByteToWideChar(CP_ACP, 0, strSrc.c_str(),
		static_cast<int>(strSrc.size()),
		pwszBuffer, iAllocSize);
	//成功
	std::wstring wstrTemp;
	if (0 < iCharsRet)
	{
		wstrTemp.assign(pwszBuffer, static_cast<size_t>(iCharsRet));
	}

	// 释放内存
	delete[] pwszBuffer;
	return UnicodeToUtf8(wstrTemp);
#elif __unix__
	return UnicodeToUtf8(AnsiToUnicode(strSrc));
	//return codeConvertString("gb2312", "utf-8//TRANSLIT", strSrc);
#endif
}

std::string CStringManager::Utf8ToAnsi(const std::string& strSrc)
{
#ifdef _WIN32
	std::wstring wstrTemp = Utf8ToUnicode(strSrc);

	// 分配目标空间, 长度为 Ansi 编码的两倍
	int iAllocSize = static_cast<int>(strSrc.size() * 2 + 10);
	char* pszBuffer = new char[iAllocSize];
	if (NULL == pszBuffer)
	{
		return "";
	}
	int iCharsRet = WideCharToMultiByte(CP_ACP, 0, wstrTemp.c_str(),
		static_cast<int>(wstrTemp.size()),
		pszBuffer, iAllocSize, NULL, NULL);
	// 成功
	std::string strRet;
	if (0 < iCharsRet)
	{
		strRet.assign(pszBuffer, static_cast<size_t>(iCharsRet));
	}

	// 释放内存
	delete[] pszBuffer;
	return strRet;
#elif __unix__
	return UnicodeToAnsi(Utf8ToUnicode(strSrc));
	//return codeConvertString("utf-8", "gb2312//TRANSLIT", strSrc);
#endif
}

std::string CStringManager::UnicodeToUtf8(const std::wstring& wstrSrc)
{
#ifdef _WIN32
	// 分配目标空间, 一个16位Unicode字符最多可以转为4个字节
	int iAllocSize = static_cast<int>(wstrSrc.size() * 4 + 10);
	char* pszBuffer = new char[iAllocSize];
	if (NULL == pszBuffer)
	{
		return "";
	}
	int iCharsRet = WideCharToMultiByte(CP_UTF8, 0, wstrSrc.c_str(),
		static_cast<int>(wstrSrc.size()),
		pszBuffer, iAllocSize, NULL, NULL);
	// 成功
	std::string strRet;
	if (0 < iCharsRet)
	{
		strRet.assign(pszBuffer, static_cast<size_t>(iCharsRet));
	}

	// 释放内存
	delete[] pszBuffer;
	return strRet;
#elif __unix__
	std::string str;
	try
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> wcv;
		str = wcv.to_bytes(wstrSrc);
	}
	catch (const std::exception& e)
	{
		//e.what();
		return str;
	}
	return str;
	//return AnsiToUtf8(UnicodeToAnsi(wstrSrc));
#endif
}

std::wstring CStringManager::Utf8ToUnicode(const std::string& strSrc)
{
#ifdef _WIN32
	// 分配目标空间
	int iAllocSize = static_cast<int>(strSrc.size() + 10);
	WCHAR* pwszBuffer = new WCHAR[iAllocSize];
	if (NULL == pwszBuffer)
	{
		return L"";
	}
	int iCharsRet = MultiByteToWideChar(CP_UTF8, 0, strSrc.c_str(),
		static_cast<int>(strSrc.size()),
		pwszBuffer, iAllocSize);
	// 成功
	std::wstring wstrRet;
	if (0 < iCharsRet)
	{
		wstrRet.assign(pwszBuffer, static_cast<size_t>(iCharsRet));
	}

	// 释放内存
	delete[] pwszBuffer;
	return wstrRet;
#elif __unix__
	std::wstring wstr;
	try
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> wcv;
		wstr = wcv.from_bytes(strSrc);
	}
	catch (const std::exception& e)
	{
		//e.what();
		return wstr;
	}
	return wstr;
	//return AnsiToUnicode(Utf8ToAnsi(strSrc));
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