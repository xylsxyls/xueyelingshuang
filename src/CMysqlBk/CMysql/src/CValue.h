#pragma once
#include <string>
#include "CMysqlMacro.h"
#include <stdint.h>

class CAttri;
class CTable;
class CMysqlAPI CValue
{
public:
	CValue();
	CValue(const std::string& strValue);
	CValue(char* szValue);
	CValue(int32_t nValue);
	CValue(double dValue);
	CValue(int32_t Type, const std::string& strValue, int32_t nValue, double dValue);

	CValue(const CValue& value);
	CValue operator= (const CValue& value);
	bool operator== (const CValue& value);
	bool operator!= (const CValue& value);

	operator int();
	operator std::string();
	operator double();

public:
	//如果要多线程操作的话需要先init才能使用CValue
	//void init(CTable* pTable);

public:
	//CAttri* operator->();

public:
	//1是字符串，2是整数，3是小数，-1是运算表达式或者出错
	int32_t m_type;
	int32_t m_nValue;
	double m_dValue;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_strValue;
	//转到表中的map字段
	//static CTable* pTable;
	std::string m_strCurrentField;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};