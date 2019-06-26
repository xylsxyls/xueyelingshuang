#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

enum EnumReplaceCommand
{
	//ƥ���滻
	REPLACE,
	//�в���ƥ�������滻
	LINE,
	//�ļ�������
	NAME
};

class FileTask : public CTask
{
public:
	FileTask();

public:
	void DoTask();

	void setParam(const std::string& filePath, EnumReplaceCommand replaceCommand, const std::string& oldStr, const std::string& newStr);

private:
	std::string m_filePath;
	EnumReplaceCommand m_replaceCommand;
	std::string m_oldStr;
	std::string m_newStr;
};