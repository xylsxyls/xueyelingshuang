#pragma once
#include <list>
#include <map>
#include "CValue.h"
#include "CMysqlMacro.h"

class CTable;
class CMysqlAPI CRecord
{
public:
	CRecord(CTable* pTable);
	CRecord(const CRecord& record);
	CRecord operator=(const CRecord& record);

public:
	//查询了错误的字段名不会导致记录里的字段增加
	CRecord& operator[](const std::string& Field);
	//给记录赋值字段值，不存在则添加
	CRecord operator=(const CValue& value);

public:
	//一旦设置属性则添加属性，即便字段名不存在
	CAttri* operator->();
	CValue toValue();

public:
	std::string toString();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//不使用table中定义字段名的模式，因为无法添加新字段
	std::map<std::string, CValue> m_mapValue;
	//存储修改的字段名，在Add执行之后清空
	std::list<std::string> m_listReviseAttri;
	std::string m_strCurrentField;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	CTable* m_pTable;
};