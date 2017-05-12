#include "CStringManager.h"
#include <stdarg.h>

int CStringManager::FindOther(const string& str, char cLeft, char cRight, int nSelect){
	vector<int> vecn;
	int nSelectSize = 0;
	int n = 0;
	while (str[n]){
		if (str[n] == cLeft){
			vecn.push_back(n);
			//?nSelect在左，记下现在的nSelectSize
			if(nSelect == n) nSelectSize = vecn.size();
		}
		if (str[n] == cRight){
			if(nSelect == n){
				if(vecn.size() > 0) return vecn.at(vecn.size() - 1);
				else return -1;
			}
			//?如果弹出之前发现size大小等于之前记录的，说明找到了对应的右侧
			if(vecn.size() == nSelectSize && nSelectSize > 0){
				return n;
			}
			if(vecn.size() > 0) vecn.pop_back();
		}
		n++;
	}
	return -1;
}

vector<string> CStringManager::split(string splitString,string separate_character){
	vector<string> strs;
	//?分割字符串的长度,这样就可以支持如“,,”多字符串的分隔符
	int separate_characterLen = separate_character.length();
	int lastPosition = 0,index = -1;
	while (-1 != (index = splitString.find(separate_character, lastPosition))){
		strs.push_back(splitString.substr(lastPosition, index - lastPosition).c_str());
		lastPosition = index + separate_characterLen;   
	}
	//?截取最后一个分隔符后的内容
	string lastString = splitString.substr(lastPosition);
	//?if (!lastString.empty()) //如果最后一个分隔符后还有内容就入队
	strs.push_back(lastString.c_str());
	return strs;
}

void CStringManager::ReplaceAll(string& str, const string& old_value, const string& new_value){
	while(true){
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
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	str.resize(size);
	//?即便分配了足够内存，长度必须加1，否则会崩溃
	vsprintf_s(&str[0], size + 1, fmt, args);
	va_end(args);
}

std::string CStringManager::Format(const char * fmt, ...){
    string result;
    va_list args;
    va_start(args, fmt);
    int size = _vscprintf(fmt, args);
    //?resize分配后string类会自动在最后分配\0，resize(5)则总长6
    result.resize(size);
    //?即便分配了足够内存，长度必须加1，否则会崩溃
    vsprintf_s(&result[0], size + 1, fmt, args);
    va_end(args);
    return result;
}

void CStringManager::MakeReverse(std::string & str){
	reverse(str.begin(),str.end());
}