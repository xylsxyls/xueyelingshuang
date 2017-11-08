#pragma once
#include <vector>
#include <string>
#include "CStringManagerMacro.h"
using namespace std;

class CStringManagerAPI CStringManager{
public:
	//?从0开始计数，根据当前选择返回另一侧符号的位置，如果给定左右两侧不存在或选择的位置不是两侧之一则返回-1
	//?两侧个数不匹配的时候会根据左侧开始返回对应的右侧
	static int FindOther(const string& str, char cLeft, char cRight, int nSelect);
	//?分割字符串
	static vector<string> split(string splitString, string separate_character);
	//?vector<CString> vec = split("123,234,345,456,",","); //得到5个字符串
	//?vector<CString> vec2 = split("123,,234,,345,,456",",,"); //得到4个字符串

	//?替换字符串，循环替换所有字符串，保证结果中不存在原来的字符串
	static void ReplaceAll(string& str, const string& old_value, const string& new_value);

	static std::string Mid(const std::string & str, size_t offset, size_t count);

	static std::string Left(const std::string & str, size_t count);

	static std::string Right(const std::string & str, size_t count);

	static size_t Find(const std::string & str, const std::string & right, size_t offset = 0);

	static size_t Find(const std::string & str, char ch, size_t offset = 0);

	static size_t ReserveFind(const std::string & str, const std::string & right);

	static size_t ReserveFind(const std::string & str, char ch);

	static size_t GetLength(const std::string & str);

	static size_t Delete(std::string & str, size_t offset, size_t count);

	static size_t Insert(std::string & str, size_t offset, const std::string & right);

	static size_t Insert(std::string & str, size_t offset, char ch);

	static size_t Replace(std::string & str, const std::string & oldstr, const std::string & newstr);

	static size_t Replace(std::string & str, char ch1, char ch2);

	static void Format(std::string & str, const char * fmt, ...);

    static std::string Format(const char * fmt, ...);

	static std::wstring Format(const wchar_t * fmt, ...);

	static void MakeReverse(std::string & str);

	static std::string MakeUpper(const std::string& src);

	static std::string MakeLower(const std::string& src);

	static std::wstring MakeUpper(const std::wstring& src);

	static std::wstring MakeLower(const std::wstring& src);

	//取出两个字符串中间的字符串，不包括头尾
	static std::string GetMidString(const std::string& src, const std::string& leftString, const std::string& rightString);
};