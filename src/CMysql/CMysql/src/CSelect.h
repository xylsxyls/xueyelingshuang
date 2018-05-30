#pragma once
#include <list>
#include "CTableField.h"
#include "CMysqlMacro.h"

class CMysqlAPI CSelect
{
public:
	//sel("User")["ID"];
	CSelect& operator()(const std::string& Table);
	CSelect& operator[](const std::string& Field);

public:
	CSelect();
	CSelect(const CSelect& sel);
	CSelect operator=(const CSelect& sel);

public:
	std::string toString();
	std::string toTableString();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//先传入tf然后传入list
	std::list<CTableField> m_listTf;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	CTableField m_tf;
};