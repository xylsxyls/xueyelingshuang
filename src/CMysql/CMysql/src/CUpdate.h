#pragma once
#include <map>
#include "CValue.h"
#include "CMysqlMacro.h"

class CMysqlAPI CUpdate
{
public:
	CUpdate();
	CUpdate(const CUpdate& upd);
	CUpdate operator=(const CUpdate& upd);
public:
	CUpdate operator=(const CValue& value);
	CUpdate& operator[](const std::string& Field);

public:
	std::string toString();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//一条记录中字段名对应字段值
	std::map<std::string, CValue> mapValue;
	std::string strCurrentField;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};