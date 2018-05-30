#include <SDKDDKVer.h>
#include "CSelect.h"
#include <map>
#include "CStringManager/CStringManagerAPI.h"

CSelect& CSelect::operator()(const std::string& Table)
{
	m_tf.m_table = Table;
	return *this;
}

CSelect& CSelect::operator[](const std::string& Field)
{
	m_tf.m_field = Field;
	m_listTf.push_back(m_tf);
	return *this;
}

CSelect::CSelect()
{

}

CSelect::CSelect(const CSelect& sel)
{
	m_tf = sel.m_tf;
	m_listTf = sel.m_listTf;
}

CSelect CSelect::operator=(const CSelect& sel)
{
	m_tf = sel.m_tf;
	m_listTf = sel.m_listTf;
	return *this;
}

std::string CSelect::toString()
{
	std::string strResult;
	for (auto it = m_listTf.begin(); it != m_listTf.end(); it++)
{
		strResult = strResult + it->m_table + "." + it->m_field + ",";
	}
	CStringManager::Delete(strResult, strResult.length() - 1, 1);
	return strResult;
}

std::string CSelect::toTableString()
{
	//因为要参与去重，所以把所有的表名存入map中再遍历
	std::string strSelectTable;
	std::map<std::string, int> mapTable;
	for (auto itList = m_listTf.begin(); itList != m_listTf.end(); itList++)
{
		mapTable[itList->m_table] = 1;
	}
	for (auto itMap = mapTable.begin(); itMap != mapTable.end(); itMap++)
{
		strSelectTable = strSelectTable + itMap->first + ",";
	}
	CStringManager::Delete(strSelectTable, strSelectTable.length() - 1, 1);
	return strSelectTable;
}