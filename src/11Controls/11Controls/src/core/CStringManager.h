#pragma once
#include <vector>
#include <string>
#include "CoreMacro.h"
using namespace std;

//����ʹ�÷�����ӦMFC�е�CString
class CoreAPI CStringManager
{
public:
	//?��0��ʼ���������ݵ�ǰѡ�񷵻���һ����ŵ�λ�ã���������������಻���ڻ�ѡ���λ�ò�������֮һ�򷵻�-1
	//?���������ƥ���ʱ��������࿪ʼ���ض�Ӧ���Ҳ�
	static int FindOther(const string& str, char cLeft, char cRight, int nSelect);
	//?�ָ��ַ���
	static vector<string> split(string splitString, string separate_character);
	//?vector<CString> vec = split("123,234,345,456,",","); //�õ�5���ַ���
	//?vector<CString> vec2 = split("123,,234,,345,,456",",,"); //�õ�4���ַ���

	static vector<wstring> split(wstring splitString, wstring separate_character);

	//?�滻�ַ�����ѭ���滻�����ַ�������֤����в�����ԭ�����ַ���
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

	static std::string Replace(const char * str, const std::string & oldstr, const std::string & newstr);

	static size_t Replace(std::wstring & str, const std::wstring & oldstr, const std::wstring & newstr);

	static size_t Replace(std::string & str, char ch1, char ch2);

	static void Format(std::string & str, const char * fmt, ...);

	static std::string Format(const char * fmt, ...);

	static void Format(std::wstring& str, const wchar_t * fmt, ...);

	static std::wstring Format(const wchar_t * fmt, ...);

	static void MakeReverse(std::string & str);

	static std::string UnicodeToAnsi(const std::wstring& wstrSrc);
};