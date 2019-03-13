#pragma once
#include "CtxtMacro.h"
#include <vector>
#include <stdint.h>
#include <map>

class CtxtAPI Ctxt
{
public:
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
	//?ת��Ϊmap��ʽ��ͨ��[string][/string]��ʽ�ҵ���ʼ�кͽ�����
	void ToMap();
	//?���ڴ��еı��浽�ļ���
	void Save();
	//?����д��ʽ��һ���ļ�
	bool OpenFile_w();
	//?��׷�ӷ�ʽ��һ���ļ�
	bool OpenFile_a();
	//?׷��д��һ��
	void AddWriteLine(const char* fmt, ...);
	//?�ر��ļ�
	void CloseFile();
	//?׷��д��һ�в����ļ�
	void AddLine(const char* fmt, ...);
	//?���
	void SaveAs(const std::string& path);
	//?����ļ�
	void ClearFile();
	//?�ļ���Ⱥ�廻
	int32_t Replace(const std::string& oldstr, const std::string& newstr);
	//��ȡ�ļ���std::string��
	std::string ReadFile(const std::string& path);

private:
	//?�õ㵽��ķ�ʽ����
	void LoadTxtWithPointToPoint(const std::string& strSplit);
	//?�÷ָ��ķ�ʽ����
	void LoadTxtWithSplit(const std::string& strSplit);
	//?���м���
	void LoadTxtWithOneLine();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<std::vector<std::string>> m_vectxt;
	//?ת����map��ʽ
	std::map<std::string, std::vector<int32_t>> m_maptxt;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	FILE *m_pFile;
	std::string m_strPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	void* m_txt;
};