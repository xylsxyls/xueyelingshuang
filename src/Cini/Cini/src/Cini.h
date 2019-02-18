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
	//取出的信息默认1024最大长度，如果key不存在，默认返回"-1"，路径传绝对路径，strSection默认为Section
	std::string ReadIni(const std::string& key, const std::string& strSection = "Section", int32_t maxLength = 1024) const;

	//路径传绝对路径，strSection默认为Section，key不存在则创建，存在则修改，文件可以不存在路径必须存在
	void WriteIni(const std::string& key, const std::string& text, const std::string& strSection = "Section") const;

    //获取所有Section，传入最大section个数
	std::vector<std::string> GetSection(int32_t maxSectionNum = 1024, int32_t maxLength = 1024) const;

    //获取该section下第一个key名
	std::string GetFirstKey(const std::string& section = "Section", int32_t maxLength = 1024) const;

    //删除该section下的指定key
	void DeleteKey(const std::string& key, const std::string& section = "Section") const;

    //删除指定section
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