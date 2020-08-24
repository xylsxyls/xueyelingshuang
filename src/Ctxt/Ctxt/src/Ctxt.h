#pragma once
#include "CtxtMacro.h"
#include <string>
#include <vector>
#include <stdint.h>

class CtxtAPI Ctxt
{
public:
	//��\r\nΪ�н�����
	enum Load
	{
		POINT_TO_POINT = 1,
		SPLIT,
		ONE_LINE
	};

public:
	Ctxt(const std::string& strPath);
	~Ctxt();

public:
	//?�ڲ�ʹ��ifstream��getline���ļ�ĩβ�Ķ�����в��ᱻ���أ�txt�����Ϊ���֣���һ�֣�ÿ��ֻȡ�̶����ӵĶ�������"1-3,4-6"���������1��1-3����ȡ123�񣬵ڶ��֣�ÿ����һ����־�ָ���ÿ���������󳤶�Ĭ��һ�ڣ������֣�һ��һ����
	void LoadTxt(Load flag, const std::string& strSplit = "");
	//?���ڴ��еı��浽�ļ���
	bool Save();
	//?���
	bool SaveAs(const std::string& path);
	//?�ļ���Ⱥ�廻
	int32_t Replace(const std::string& oldstr, const std::string& newstr);
	//?׷��д��һ�в����ļ������л��з�
	void AddLineWithoutOpenFile(const char* fmt, ...);

private:
	//?�õ㵽��ķ�ʽ����
	void LoadTxtWithPointToPoint(const std::string& strSplit);
	//?�÷ָ��ķ�ʽ����
	void LoadTxtWithSplit(const std::string& strSplit);
	//?���м���
	void LoadTxtWithOneLine();
	//?д�룬�����л��з�
	void Write(FILE* pFile, const char* fmt, ...);

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<std::vector<std::string>> m_vectxt;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_strPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	void* m_txt;
};