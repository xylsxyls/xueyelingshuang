#pragma once
#include <SDKDDKVer.h>
#include <afx.h>
#include "API.h"

class API CField{
	//字段管具体信息
public:
	CString FieldName;
	CString Type;
	int Length;
	CString strLength;
	BOOL Default;
	int nDefault;
	CString strDefault;
	BOOL ifPrimaryKey;

public:
	CField();
	//用于查找字段属性时只输入名字即可
	CField(CString FieldName);
	//int类型的不带长度，写在数据库里的长度是虚的，只有字符带长度，如果长度为1，存"1234"，进去的是"1"
	CField(CString FieldName,CString Type,int Length = 255,BOOL ifPrimaryKey = 0);
	//在数据库中type是用数字表示的
	CField(CString FieldName,int nType,int Length = 255,BOOL ifPrimaryKey = 0);

public:
	//设置字段属性，int型type
	void SetAttributes(CString FieldName,int    nType,int Length = 255,BOOL Default = 0,CString strDefault = "",BOOL ifPrimaryKey = 0);
	//设置字段属性
	void SetAttributes(CString FieldName,CString Type,int Length = 255,BOOL Default = 0,CString strDefault = "",BOOL ifPrimaryKey = 0);

};

