#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

enum EnumReplaceCommand
{
	//匹配替换
	REPLACE,
	//行部分匹配整行替换
	LINE,
	//文件重命名
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