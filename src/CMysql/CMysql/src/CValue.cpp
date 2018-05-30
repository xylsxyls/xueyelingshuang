#include <SDKDDKVer.h>
#include "CValue.h"
#include "CAttri.h"
#include "CTable.h"
#include "CStringManager/CStringManagerAPI.h"

//CTable* CValue::pTable = NULL;

CValue::CValue()
{
	m_type = 0;
	m_nValue = 0;
	m_dValue = 0;
	m_strValue = "";
}

CValue::CValue(const std::string& strValue)
{
	m_type = 1;
	m_nValue = 0;
	m_dValue = 0;
	m_strValue = "'" + strValue + "'";
}

CValue::CValue(char* szValue)
{
	m_type = 1;
	m_nValue = 0;
	m_dValue = 0;
	m_strValue = m_strValue + "'" + szValue + "'";
}

CValue::CValue(int32_t nValue)
{
	m_type = 2;
	m_nValue = nValue;
	m_dValue = 0;
	m_strValue = CStringManager::Format("%d", nValue);
}

CValue::CValue(double dValue)
{
	m_type = 3;
	m_nValue = 0;
	m_dValue = dValue;
	m_strValue = CStringManager::Format("%lf", dValue);
}

CValue::CValue(int32_t Type, const std::string& strValue, int32_t nValue, double dValue)
{
	m_type = -1;
	m_nValue = -1;
	m_dValue = -1;
	m_strValue = "-1";
}

CValue::CValue(const CValue& value)
{
	m_type = value.m_type;
	m_nValue = value.m_nValue;
	m_dValue = value.m_dValue;
	m_strValue = value.m_strValue;
	m_strCurrentField = value.m_strCurrentField;
}

CValue CValue::operator = (const CValue& value)
{
	m_type = value.m_type;
	m_nValue = value.m_nValue;
	m_dValue = value.m_dValue;
	m_strValue = value.m_strValue;
	m_strCurrentField = value.m_strCurrentField;
	return *this;
}

bool CValue::operator== (const CValue& value)
{
	return m_type == value.m_type &&
		m_nValue == value.m_nValue &&
		m_dValue == value.m_dValue &&
		m_strValue == value.m_strValue;
}

bool CValue::operator!= (const CValue& value)
{
	return m_type != value.m_type &&
		m_nValue != value.m_nValue &&
		m_dValue != value.m_dValue &&
		m_strValue != value.m_strValue;
}

CValue::operator int()
{
	return m_nValue;
}

CValue::operator std::string()
{
	if (m_strValue == "-1")
	{
		return "-1";
	}
	return CStringManager::Mid(m_strValue, 1, m_strValue.length() - 2);
}

CValue::operator double()
{
	return m_dValue;
}

//void CValue::init(CTable* pTable)
//{
//	pTable = pTable;
//	return;
//}

//CAttri* CValue::operator->()
//{
//	return &(pTable->mapAttri[strCurrentField]);
//}