#pragma once
#include "CiniMacro.h"
#include <string>
#include <vector>
using namespace std;

class CiniAPI Cini{
public:

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	string iniPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
public:
	Cini(const string& iniPath);

public:
	//取出的信息默认1024最大长度，如果key不存在，默认返回"-1"，路径传绝对路径，strSection默认为Section
	string ReadIni(const string& key, const string& strSection = "Section", int MaxLength = 1024)const;

	//路径传绝对路径，strSection默认为Section，key不存在则创建，存在则修改，文件可以不存在路径必须存在
	void WriteIni(const string& key, const string& text, const string& strSection = "Section")const;

    //获取所有Section，传入最大section个数
	vector<string> GetSection(int MaxSectionNum, int MaxLength = 1024)const;

    //获取该section下第一个key名
	string GetFirstKey(const string& section = "Section", int MaxLength = 1024)const;

    //删除该section下的指定key
	void DeleteKey(const string& key, const string& section = "Section")const;

    //删除指定section
	void DeleteSection(const string& section = "Section")const;
};