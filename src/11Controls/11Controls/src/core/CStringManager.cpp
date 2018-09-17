#include "CStringManager.h"
#include <stdarg.h>
#include <Windows.h>

int CStringManager::FindOther(const string& str, char cLeft, char cRight, int nSelect)
{
	vector<int> vecn;
	int nSelectSize = 0;
	int n = 0;
	while (str[n])
	{
		if (str[n] == cLeft)
		{
			vecn.push_back(n);
			//?nSelect���󣬼������ڵ�nSelectSize
			if (nSelect == n) nSelectSize = vecn.size();
		}
		if (str[n] == cRight)
		{
			if (nSelect == n)
			{
				if (vecn.size() > 0) return vecn.at(vecn.size() - 1);
				else return -1;
			}
			//?�������֮ǰ����size��С����֮ǰ��¼�ģ�˵���ҵ��˶�Ӧ���Ҳ�
			if (vecn.size() == nSelectSize && nSelectSize > 0)
			{
				return n;
			}
			if (vecn.size() > 0) vecn.pop_back();
		}
		n++;
	}
	return -1;
}

vector<string> CStringManager::split(string splitString, string separate_character)
{
	vector<string> strs;
	//?�ָ��ַ����ĳ���,�����Ϳ���֧���硰,,�����ַ����ķָ���
	int separate_characterLen = separate_character.length();
	int lastPosition = 0, index = -1;
	while (-1 != (index = splitString.find(separate_character, lastPosition)))
	{
		strs.push_back(splitString.substr(lastPosition, index - lastPosition).c_str());
		lastPosition = index + separate_characterLen;
	}
	//?��ȡ���һ���ָ����������
	string lastString = splitString.substr(lastPosition);
	//?if (!lastString.empty()) //������һ���ָ����������ݾ����
	strs.push_back(lastString.c_str());
	return strs;
}

vector<wstring> CStringManager::split(wstring splitString, wstring separate_character)
{
	vector<wstring> strs;
	//?�ָ��ַ����ĳ���,�����Ϳ���֧���硰,,�����ַ����ķָ���
	int separate_characterLen = separate_character.length();
	int lastPosition = 0, index = -1;
	while (-1 != (index = splitString.find(separate_character, lastPosition)))
	{
		strs.push_back(splitString.substr(lastPosition, index - lastPosition).c_str());
		lastPosition = index + separate_characterLen;
	}
	//?��ȡ���һ���ָ����������
	wstring lastString = splitString.substr(lastPosition);
	//?if (!lastString.empty()) //������һ���ָ����������ݾ����
	strs.push_back(lastString.c_str());
	return strs;
}


void CStringManager::ReplaceAll(string& str, const string& old_value, const string& new_value)
{
	while (true)
	{
		string::size_type pos(0);
		if ((pos = str.find(old_value)) != string::npos) str.replace(pos, old_value.length(), new_value);
		else break;
	}
}

std::string CStringManager::Mid(const std::string & str, size_t offset, size_t count)
{
	return str.substr(offset, count);
}

std::string CStringManager::Left(const std::string & str, size_t count)
{
	return Mid(str, 0, count);
}

std::string CStringManager::Right(const std::string & str, size_t count)
{
	return Mid(str, str.size() - count, count);
}

size_t CStringManager::Find(const std::string & str, const std::string & right, size_t offset)
{
	return str.find(right, offset);
}

size_t CStringManager::Find(const std::string & str, char ch, size_t offset)
{
	return str.find(ch, offset);
}

size_t CStringManager::ReserveFind(const std::string & str, const std::string & right)
{
	return str.rfind(right);
}

size_t CStringManager::ReserveFind(const std::string & str, char ch)
{
	return str.rfind(ch);
}

size_t CStringManager::GetLength(const std::string & str)
{
	return str.length();
}

size_t CStringManager::Delete(std::string & str, size_t offset, size_t count)
{
	return str.erase(offset, count).length();
}

size_t CStringManager::Insert(std::string & str, size_t offset, const std::string & right)
{
	return str.insert(offset, right).length();
}

size_t CStringManager::Insert(std::string & str, size_t offset, char ch)
{
	return str.insert(offset, &ch, 1).length();
}

size_t CStringManager::Replace(std::string & str, const std::string & oldstr, const std::string & newstr)
{
	size_t count = 0;
	size_t pos = 0;
	while (true)
	{
		pos = str.find(oldstr, pos);
		if (pos != std::string::npos)
		{
			str.replace(pos, oldstr.length(), newstr);
			pos += newstr.length();
			++count;
		}
		else
			break;
	}
	return count;
}

std::string CStringManager::Replace(const char * str, const std::string & oldstr, const std::string & newstr)
{
	if (str == nullptr)
	{
		return std::string();
	}
	std::string result = str;
	Replace(result, oldstr, newstr);
	return result;
}

size_t CStringManager::Replace(std::wstring & str, const std::wstring & oldstr, const std::wstring & newstr)
{
	size_t count = 0;
	size_t pos = 0;
	while (true)
	{
		pos = str.find(oldstr, pos);
		if (pos != std::wstring::npos)
		{
			str.replace(pos, oldstr.length(), newstr);
			pos += newstr.length();
			++count;
		}
		else
			break;
	}
	return count;
}

size_t CStringManager::Replace(std::string & str, char ch1, char ch2)
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

void CStringManager::Format(std::string & str, const char * fmt, ...)
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

std::string CStringManager::Format(const char * fmt, ...)
{
	string result;
	va_list args;
	va_start(args, fmt);
	int size = _vscprintf(fmt, args);
	//?resize�����string����Զ���������\0��resize(5)���ܳ�6
	result.resize(size);
	//?��sizeΪ0ʱ���۳���+���������
	if (size != 0)
	{
		//?����������㹻�ڴ棬���ȱ����1����������
		vsprintf_s(&result[0], size + 1, fmt, args);
	}
	va_end(args);
	return result;
}

void CStringManager::Format(std::wstring& str, const wchar_t * fmt, ...)
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

std::wstring CStringManager::Format(const wchar_t * fmt, ...)
{
	wstring result;
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

void CStringManager::MakeReverse(std::string & str)
{
	reverse(str.begin(), str.end());
}

std::string CStringManager::UnicodeToAnsi(const std::wstring& wstrSrc)
{
	// ����Ŀ��ռ�, һ��16λUnicode�ַ�������תΪ4���ֽ�
	int iAllocSize = static_cast<int>(wstrSrc.size() * 4 + 10);
	char* pwszBuffer = new char[iAllocSize];
	if (NULL == pwszBuffer)
	{
		return "";
	}
	int iCharsRet = WideCharToMultiByte(CP_ACP, 0, wstrSrc.c_str(),
		static_cast<int>(wstrSrc.size()),
		pwszBuffer, iAllocSize, NULL, NULL);
	std::string strRet;
	if (0 < iCharsRet)
	{
		strRet.assign(pwszBuffer, static_cast<size_t>(iCharsRet));
	}

	delete[] pwszBuffer;

	return strRet;
}