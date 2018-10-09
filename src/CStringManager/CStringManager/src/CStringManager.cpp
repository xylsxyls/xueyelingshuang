#include "CStringManager.h"
#include <stdarg.h>
#include <algorithm>
#include <stdint.h>
#include <sstream>

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
			//?nSelect���󣬼������ڵ�nSelectSize
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
			//?�������֮ǰ����size��С����֮ǰ��¼�ģ�˵���ҵ��˶�Ӧ���Ҳ�
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
	//?�ָ��ַ����ĳ���,�����Ϳ���֧���硰,,�����ַ����ķָ���
	size_t separate_characterLen = separate_character.length();
	size_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)splitString.find(separate_character, lastPosition)))
	{
		strs.push_back(splitString.substr(lastPosition, index - lastPosition).c_str());
		lastPosition = index + separate_characterLen;   
	}
	//?��ȡ���һ���ָ����������
	std::string lastString = splitString.substr(lastPosition);
	//?if (!lastString.empty()) //������һ���ָ����������ݾ����
	strs.push_back(lastString.c_str());
	return strs;
}

std::vector<std::wstring> CStringManager::split(const std::wstring& splitString, const std::wstring& separate_character)
{
    std::vector<std::wstring> strs;
	//?�ָ��ַ����ĳ���,�����Ϳ���֧���硰,,�����ַ����ķָ���
	size_t separate_characterLen = separate_character.length();
	size_t lastPosition = 0;
	int32_t index = -1;
	while (-1 != (index = (int32_t)splitString.find(separate_character, lastPosition)))
	{
		strs.push_back(splitString.substr(lastPosition, index - lastPosition).c_str());
		lastPosition = index + separate_characterLen;
	}
	//?��ȡ���һ���ָ����������
    std::wstring lastString = splitString.substr(lastPosition);
	//?if (!lastString.empty()) //������һ���ָ����������ݾ����
	strs.push_back(lastString.c_str());
	return strs;
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

void CStringManager::Format(std::string& str, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int size = _vscprintf(fmt, args);
	//?resize�����string����Զ���������\0��resize(5)���ܳ�6
	str.resize(size);
	if (size != 0)
	{
		//?����������㹻�ڴ棬���ȱ����1����������
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
    //?resize�����string����Զ���������\0��resize(5)���ܳ�6
    result.resize(size);
	if (size != 0)
	{
		//?����������㹻�ڴ棬���ȱ����1����������
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
	//?resize�����string����Զ���������\0��resize(5)���ܳ�6
	str.resize(size);
	if (size != 0)
	{
		//?����������㹻�ڴ棬���ȱ����1����������
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
	//?resize�����string����Զ���������\0��resize(5)���ܳ�6
	result.resize(size);
	if (size != 0)
	{
		//?����������㹻�ڴ棬���ȱ����1����������
		vswprintf_s(&result[0], size + 1, fmt, args);
	}
	va_end(args);
	return result;
}

void CStringManager::MakeReverse(std::string& str)
{
	std::reverse(str.begin(),str.end());
}

std::string CStringManager::MakeUpper(const std::string& src)
{
	std::string dst;
#if (_MSC_VER >= 1800)
	//���dst����ֵ�Ļ��������������dst.begin()����ͷ��ʼ����
	std::transform(src.begin(), src.end(), std::back_inserter(dst), ::toupper);
#endif
	return dst;
}

std::string CStringManager::MakeLower(const std::string& src)
{
	std::string dst;
#if (_MSC_VER >= 1800)
	std::transform(src.begin(), src.end(), std::back_inserter(dst), ::tolower);
#endif
	return dst;
}

std::wstring CStringManager::MakeUpper(const std::wstring& src)
{
	std::wstring dst;
#if (_MSC_VER >= 1800)
	std::transform(src.begin(), src.end(), std::back_inserter(dst), ::toupper);
#endif
	return dst;
}

std::wstring CStringManager::MakeLower(const std::wstring& src)
{
	std::wstring dst;
#if (_MSC_VER >= 1800)
	std::transform(src.begin(), src.end(), std::back_inserter(dst), ::tolower);
#endif
	return dst;
}

std::string CStringManager::GetMidString(const std::string& src, const std::string& leftString, const std::string& rightString)
{
	int left = (int)src.find(leftString);
	int right = (int)src.find(rightString);
	return Mid(src, left + 1, right - left - 1);
}

uint64_t CStringManager::atoui64(const char* str)
{
	uint64_t result;
	std::stringstream strValue;
	strValue << str;
	strValue >> result;
	return result;
}