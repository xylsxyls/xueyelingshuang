#include "CStringManager.h"
//#include <stdarg.h>
#include <algorithm>
//#include <sstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#endif
//#include <stdlib.h>
//#include <string.h>
//#include <iterator>
//#include <locale.h>
//#include <wchar.h>
//#include <cstdlib>

size_t CStringManager::FindOther(const std::string& str, char cLeft, char cRight, size_t nSelect)
{
	std::vector<size_t> vecn;
	size_t nSelectSize = 0;
	size_t index = 0;
	while (str[index])
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
				if (vecn.size() <= 0)
				{
					return -1;
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
	return -1;
}

std::vector<std::string> CStringManager::split(const std::string& splitString, const std::string& separate_character)
{
	std::vector<std::string> strs;
	//?分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	size_t separate_characterLen = separate_character.length();
	size_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)splitString.find(separate_character, lastPosition)))
	{
		strs.push_back(splitString.substr(lastPosition, index - lastPosition));
		lastPosition = index + separate_characterLen;   
	}
	//?截取最后一个分隔符后的内容
	//?if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	strs.push_back(splitString.substr(lastPosition));
	return strs;
}

std::vector<std::wstring> CStringManager::split(const std::wstring& splitString, const std::wstring& separate_character)
{
    std::vector<std::wstring> strs;
	//?分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	size_t separate_characterLen = separate_character.length();
	size_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)splitString.find(separate_character, lastPosition)))
	{
		strs.push_back(splitString.substr(lastPosition, index - lastPosition));
		lastPosition = index + separate_characterLen;
	}
	//?截取最后一个分隔符后的内容
	//?if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	strs.push_back(splitString.substr(lastPosition));
	return strs;
}

void CStringManager::split(std::vector<std::string>& result, const std::string& splitString, char separate_character)
{
	result.clear();
	size_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)splitString.find(separate_character, lastPosition)))
	{
		result.push_back(splitString.substr(lastPosition, index - lastPosition));
		lastPosition = index + 1;
	}
	result.push_back(splitString.substr(lastPosition));
}

void CStringManager::ReplaceAll(std::string& str, const std::string& old_value, const std::string& new_value)
{
	while(true)
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
	return str.substr(offset, count);
}

std::string CStringManager::Left(const std::string& str, size_t count)
{
	return Mid(str, 0, count);
}

std::string CStringManager::Right(const std::string& str, size_t count)
{
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
	return str.erase(offset, count).length();
}

size_t CStringManager::Insert(std::string& str, size_t offset, const std::string& right)
{
	return str.insert(offset, right).length();
}

size_t CStringManager::Insert(std::string& str, size_t offset, char ch)
{
	return str.insert(offset, &ch, 1).length();
}

size_t CStringManager::Replace(std::string& str, const std::string& oldstr, const std::string& newstr)
{
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

#ifdef _WIN32

void CStringManager::Format(std::string& str, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int size = _vscprintf(fmt, args);
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	str.resize(size);
	if (size != 0)
	{
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vsprintf_s(&str[0], size + 1, fmt, args);
	}
	va_end(args);
}

std::string CStringManager::Format(const char* fmt, ...)
{
	std::string result;
    va_list args;
    va_start(args, fmt);
    int size = _vscprintf(fmt, args);
    //?resize分配后string类会自动在最后分配\0，resize(5)则总长6
    result.resize(size);
	if (size != 0)
	{
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vsprintf_s(&result[0], size + 1, fmt, args);
	}
    va_end(args);
    return result;
}

void CStringManager::Format(std::wstring& str, const wchar_t* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int size = _vscwprintf(fmt, args);
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	str.resize(size);
	if (size != 0)
	{
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vswprintf_s(&str[0], size + 1, fmt, args);
	}
	va_end(args);
}

std::wstring CStringManager::Format(const wchar_t* fmt, ...)
{
	std::wstring result;
	va_list args;
	va_start(args, fmt);
	int size = _vscwprintf(fmt, args);
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	result.resize(size);
	if (size != 0)
	{
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vswprintf_s(&result[0], size + 1, fmt, args);
	}
	va_end(args);
	return result;
}

#endif

void CStringManager::MakeReverse(std::string& str)
{
	std::reverse(str.begin(),str.end());
}

std::string CStringManager::MakeUpper(const std::string& src)
{
	std::string dst;
#if defined _MSC_VER && (_MSC_VER < 1800)
    return dst;
#endif
    //如果dst是有值的话则第三个参数传dst.begin()，从头开始覆盖
    std::transform(src.begin(), src.end(), std::back_inserter(dst), ::toupper);
	return dst;
}

std::string CStringManager::MakeLower(const std::string& src)
{
	std::string dst;
#if defined _MSC_VER && (_MSC_VER < 1800)
    return dst;
#endif
    std::transform(src.begin(), src.end(), std::back_inserter(dst), ::tolower);
	return dst;
}

std::wstring CStringManager::MakeUpper(const std::wstring& src)
{
	std::wstring dst;
#if defined _MSC_VER && (_MSC_VER < 1800)
    return dst;
#endif
    std::transform(src.begin(), src.end(), std::back_inserter(dst), ::toupper);
	return dst;
}

std::wstring CStringManager::MakeLower(const std::wstring& src)
{
	std::wstring dst;
#if defined _MSC_VER && (_MSC_VER < 1800)
    return dst;
#endif
    std::transform(src.begin(), src.end(), std::back_inserter(dst), ::tolower);
	return dst;
}

std::string CStringManager::GetMidString(const std::string& src, const std::string& leftString, const std::string& rightString)
{
    int32_t left = (int32_t)src.find(leftString);
    int32_t right = (int32_t)src.find(rightString);
	return Mid(src, left + 1, right - left - 1);
}

uint64_t CStringManager::atoui64(const char* str)
{
#ifdef _WIN32
	return _strtoui64(str, nullptr, 10);
#elif __linux__
	return 0;
#endif
}

int64_t CStringManager::atoi64(const char* str)
{
#ifdef _WIN32
	return _atoi64(str);
#elif __linux__
	return 0;
#endif
}

#ifdef __linux__

static std::wstring StringToWString(const char *pc)
{
	if (pc == nullptr)
	{
		return L"";
	}
	char* old = setlocale(LC_CTYPE, "zh_CN.gbk");//"Chinese-simplified" windows "zh_CN.gb2312" linux
	size_t destlen = mbstowcs(0, pc, 0);
	if (destlen == (size_t)(-1))
	{
		setlocale(LC_CTYPE, old);
		return L"";
	}
	size_t size_of_wc = destlen + 1;
	wchar_t* pw = (wchar_t*)calloc(size_of_wc, sizeof(wchar_t));
	auto mbstowcs_result = mbstowcs(pw, pc, size_of_wc);
	if (mbstowcs_result == (size_t)(-1))
	{
		setlocale(LC_CTYPE, old);
		return L"";
	}
	std::wstring result = pw;
	delete[] pw;
	setlocale(LC_CTYPE, old);
	return result;
}

static std::string WStringToString(const wchar_t* pw)
{
	if (pw == nullptr)
	{
		return "";
	}

	char* old = setlocale(LC_CTYPE, "zh_CN.gbk");
	size_t destlen = wcstombs(0, pw, 0);
	if (destlen == (size_t)(-1))
	{
		setlocale(LC_CTYPE, old);
		return "";
	}

	size_t size_of_wc = destlen + 1;
	char* pc = (char*)calloc(size_of_wc, sizeof(char));
	if (pc == nullptr)
	{
		setlocale(LC_CTYPE, old);
		return "";
	}

	auto wcstombs_result = wcstombs(pc, pw, size_of_wc);
	if (wcstombs_result == (size_t)(-1))
	{
		setlocale(LC_CTYPE, old);
		return "";
	}
	std::string result = pc;
	delete[] pc;
	setlocale(LC_CTYPE, old);
	return result;
}

static int32_t code_convert(const char* from_charset, const char* to_charset, const char* inbuf, size_t inlen, char* outbuf, size_t* outlen)
{
	iconv_t cd;
	char** pin = (char**)&inbuf;
	char** pout = &outbuf;

	cd = iconv_open(to_charset, from_charset);
	if (cd == 0)
	{
		return -1;
	}
	size_t outLength = *outlen;
	//memset(outbuf, 0, outLength);
	if (iconv(cd, pin, &inlen, pout, outlen) == -1)
	{
		return -1;
	}
	*outlen = outLength - *outlen;
	iconv_close(cd);
	return 0;
}

static std::string code_convert_string(const char* from_charset, const char* to_charset, const std::string& src)
{
	if(src.empty())
	{
		return "";
	}
	size_t outLength = src.size() * 6;
	char* out = new char[outLength];
	if (code_convert(from_charset, to_charset, src.c_str(), src.size(), out, &outLength) != 0)
	{
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
#elif __linux__
	return WStringToString(wstrSrc.c_str());
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
#elif __linux__
	return StringToWString(strSrc.c_str());
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
#elif __linux__
	return code_convert_string("gb2312", "utf-8//TRANSLIT", strSrc);
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
#elif __linux__
	return code_convert_string("utf-8", "gb2312//TRANSLIT", strSrc);
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
#elif __linux__
	return AnsiToUtf8(UnicodeToAnsi(wstrSrc));
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
#elif __linux__
	return AnsiToUnicode(Utf8ToAnsi(strSrc));
#endif
}

static inline unsigned char toHex(const unsigned char &x)
{
    return x > 9 ? x -10 + 'A': x + '0';
}

static inline unsigned char fromHex(const unsigned char &x)
{
    return isdigit(x) ? x-'0' : x-'A'+10;
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
        //    buf[0] = '+';
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
        if(sIn[ix]=='%')
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