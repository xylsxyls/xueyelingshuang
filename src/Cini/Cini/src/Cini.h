#pragma once
#include <afxwin.h>
#include "CiniMacro.h"

class CiniAPI Cini{
public:
	CString iniPath;

public:
	Cini(CString iniPath);

public:
	//取出的信息默认1024最大长度，如果key不存在，默认返回"-1"，路径传绝对路径，strSection默认为Section
	CString ReadIni(CString key,int MaxLength = 1024,CString strSection = "Section");

	//路径传绝对路径，strSection默认为Section，key不存在则创建，存在则修改，文件可以不存在路径必须存在
	void WriteIni(CString key,CString text,CString strSection = "Section");
};