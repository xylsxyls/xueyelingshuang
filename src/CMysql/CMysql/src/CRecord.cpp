#include <SDKDDKVer.h>
#include "CRecord.h"
#include "CTable.h"
#include "CStringManager/CStringManagerAPI.h"

CRecord::CRecord(CTable* pTable)
{
	m_pTable = pTable;
}

CRecord::CRecord(const CRecord& record)
{
	m_mapValue = record.m_mapValue;
	m_strCurrentField = record.m_strCurrentField;
	m_pTable = record.m_pTable;
	m_listReviseAttri = record.m_listReviseAttri;
}

CRecord CRecord::operator=(const CRecord& record)
{
	m_mapValue = record.m_mapValue;
	m_strCurrentField = record.m_strCurrentField;
	m_pTable = record.m_pTable;
	m_listReviseAttri = record.m_listReviseAttri;
	return *this;
}

CRecord& CRecord::operator[](const std::string& Field)
{
	m_strCurrentField = Field;
	return *this;
}

CRecord CRecord::operator=(const CValue& value)
{
	m_mapValue[m_strCurrentField] = value;
	return *this;
}

CAttri* CRecord::operator->()
{
	m_listReviseAttri.push_back(m_strCurrentField);
	return &(m_pTable->m_mapAttri[m_strCurrentField]);
}

CValue CRecord::toValue()
{
	//不能直接return，这样会返回一个空CValue，需要返回一个错误CValue，如果是直接定义CRecord则会添加一个错误字段
	//return mapValue[strCurrentField];

	//如果有才返回
	auto it = m_mapValue.find(m_strCurrentField);
	if (it != m_mapValue.end())
	{
		return it->second;
	}
	else return CValue(-1, "-1", -1, -1);
}

std::string CRecord::toString()
{
	std::string strResult;
	for (auto it = m_mapValue.begin(); it != m_mapValue.end(); it++)
	{
		strResult = strResult + it->first + "=" + it->second.m_strValue + ",";
	}
	CStringManager::Delete(strResult, strResult.length() - 1, 1);
	return strResult;
}