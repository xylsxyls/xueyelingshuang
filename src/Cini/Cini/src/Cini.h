#pragma once
#include <string>
#include <vector>
#include <stdint.h>
#include "CiniMacro.h"

class CiniAPI Cini
{
public:
	Cini(const std::string& iniPath);

public:
	//ȡ������ϢĬ��1024��󳤶ȣ����key�����ڣ�Ĭ�Ϸ���"-1"��·��������·����strSectionĬ��ΪSection
	std::string ReadIni(const std::string& key, const std::string& strSection = "Section", int32_t maxLength = 1024) const;

	//·��������·����strSectionĬ��ΪSection��key�������򴴽����������޸ģ��ļ����Բ�����·���������
	void WriteIni(const std::string& key, const std::string& text, const std::string& strSection = "Section") const;

    //��ȡ����Section���������section����
	std::vector<std::string> GetSection(int32_t maxSectionNum = 1024, int32_t maxLength = 1024) const;

    //��ȡ��section�µ�һ��key��
	std::string GetFirstKey(const std::string& section = "Section", int32_t maxLength = 1024) const;

    //ɾ����section�µ�ָ��key
	void DeleteKey(const std::string& key, const std::string& section = "Section") const;

    //ɾ��ָ��section
	void DeleteSection(const std::string& section = "Section") const;

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_iniPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};