#pragma once
#include "CMysqlMacro.h"
#include <string>

class CValue;
class CCondition;
class CMysqlAPI CTableField
{
public:
	//tf("user")["ID"];
	CTableField& operator()(const std::string& Table);
	CTableField& operator[](const std::string& Field);
public:
	CTableField();
	CTableField(const CTableField& tf);
	CTableField operator=(const CTableField& tf);

public:
	CTableField(const std::string& Table, const std::string& Field);

public:
	CCondition operator==(const CValue& value);
	CCondition operator!=(const CValue& value);
	CCondition operator<(const CValue& value);
	CCondition operator>(const CValue& value);
	CCondition operator<=(const CValue& value);
	CCondition operator>=(const CValue& value);
	//内联
	CCondition operator&&(const CTableField& tf);
	//左联合，左边的数据量大于等于右边的数据量
	CCondition operator<=(const CTableField& tf);
	//右联合，左边的数据量小于等于右边的数据量
	CCondition operator>=(const CTableField& tf);
	//全联
	CCondition operator||(const CTableField& tf);

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_table;
	std::string m_field;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};