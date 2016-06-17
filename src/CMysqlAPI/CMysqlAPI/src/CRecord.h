#pragma once
#include <SDKDDKVer.h>
#include <afx.h>
#include "CField.h"
#include "CFieldValue.h"
#include <vector>
using namespace std;
#include <stdarg.h>

class CField;
class CFieldValue;
//多个字段值组成一条，每个字段值都有自己的字段属性

class __declspec(dllexport) CRecord{
public:
	//用来记录各个进来的字段值
	vector<CFieldValue> vecFieldValue;
	//当CRecord作为记录条件时，中间的连接符
	vector<CString> vecCondition;

public:
	CRecord();
	CRecord(CField      x);
	CRecord(CFieldValue x);

public:
	CRecord operator = (CField      x);
	CRecord operator = (CFieldValue x);

	friend CRecord operator + (CRecord     x,CFieldValue y);
	friend CRecord operator + (CFieldValue x,CRecord     y);
	friend CRecord operator + (CRecord     x,CRecord     y);
	//这里没有办法完全屏蔽掉错误情况，需要函数来进行判断
	friend CRecord operator + (CRecord     x,CString     y);
	friend CRecord operator + (CFieldValue x,CString     y);
	friend CRecord operator + (CFieldValue x,CFieldValue y);

	friend CRecord operator + (CRecord     x,CField      y);
	friend CRecord operator + (CField      x,CRecord     y);
	friend CRecord operator + (CField      x,CField      y);

public:
	//从CTable中一次性获得表里的所有字段属性，然后一一对应输入对应的值
	CRecord* SetValue(vector<CField> vecField,...);
	//是否是条件记录
	BOOL IsConditionRecord();
	//是否是设置记录，只有在添加记录时才会用到
	BOOL IsSetRecord();

public:
	//输出字段名，逗号连接
	CString OutPutFieldName();
	//将记录转化为字符串
	CString RecordToCString();
	//强转为设置记录，不改变本身，返回一个新建的强转过的类
	CRecord ConditionToSet();
};