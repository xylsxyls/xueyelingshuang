#pragma once
#include <vector>
#include "CStringManagerMacro.h"
using namespace std;
#include <string>
#include <Windows.h>

class CStringManagerAPI CStringManager{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	string strInside;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
	CStringManager(string str);
	CStringManager operator = (string str);

public:
	//从0开始计数，根据当前选择返回另一侧符号的位置，如果给定左右两侧不存在或选择的位置不是两侧之一则返回-1
	//两侧个数不匹配的时候会根据左侧开始返回对应的右侧
	int FindOther(char cLeft,char cRight,int nSelect);
	//分割字符串
	vector<string> split(string separate_character);
	//vector<CString> vec = split("123,234,345,456,",","); //得到5个字符串
	//vector<CString> vec2 = split("123,,234,,345,,456",",,"); //得到4个字符串

	//生成一个随机字符串，最小长度，最大长度，字符串中可能出现的字符，如果不传则为任何字符
	string RandomString(int nMin,int nMax,vector<char> vecCharacter = vector<char>());

	//替换字符串，循环替换所有字符串，保证结果中不存在原来的字符串
	string ReplaceAll(const string& old_value,const string& new_value);
	//替换字符串，单次替换字符串，等于CString中的Replace
	string ReplaceEvery(const string& old_value,const string& new_value);

	//格式化字符串
	//static void Format(string &str,LPCTSTR szFormat,...);
	//取左边的N个字符形成一个新的字符串
	//void Left();

	static std::string Mid(const std::string & str, size_t offset, size_t count);

	static std::string Left(const std::string & str, size_t count);

	static std::string Right(const std::string & str, size_t count);

	static size_t Find(const std::string & str, const std::string & right, size_t offset = 0);

	static size_t Find(const std::string & str, char ch, size_t offset = 0);

	static size_t ReserveFind(const std::string & str, const std::string & right, size_t offset = 0);

	static size_t ReserveFind(const std::string & str, char ch, size_t offset = 0);

	static size_t GetLength(const std::string & str);

	static size_t Delete(std::string & str, size_t offset, size_t count);

	static size_t Insert(std::string & str, size_t offset, const std::string & right);

	static size_t Insert(std::string & str, size_t offset, char ch);

	static size_t Replace(std::string & str, const std::string & oldstr, const std::string & newstr);

	static size_t Replace(std::string & str, char ch1, char ch2);

	static void Format(std::string & str, const char * fmt, ...);
};