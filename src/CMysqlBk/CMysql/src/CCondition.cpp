#include <SDKDDKVer.h>
#include "CCondition.h"
#include "CStringManager/CStringManagerAPI.h"

CCondition::CCondition()
{
	m_lineBegin = -1;
	m_lineEnd = -1;
}

CCondition& CCondition::operator&&(CCondition con)
{
	//第一次
	if (m_strSQL == "")
	{
		m_strSQL = con.m_strSQL;
		return *this;
	}
	m_strSQL = "(" + m_strSQL + " and " + con.m_strSQL + ")";
	return *this;
}

CCondition& CCondition::operator||(CCondition con)
{
	//第一次
	if (m_strSQL == "")
	{
		m_strSQL = con.m_strSQL;
		return *this;
	}
	m_strSQL = "(" + m_strSQL + " or " + con.m_strSQL + ")";
	return *this;
}

CCondition& CCondition::operator!()
{
	//第一次
	if (m_strSQL == "")
	{
		return *this;
	}
	m_strSQL = "( not " + m_strSQL + ")";;
	return *this;
}

CCondition& CCondition::operator()(int32_t nLineBegin, int32_t nLineEnd)
{
	m_lineBegin = nLineBegin;
	m_lineEnd = nLineEnd;
	return *this;
}

CCondition& CCondition::operator[](const std::string& strOrderField)
{
	m_strOrderField = strOrderField;
	return *this;
}

const CCondition& CCondition::operator++(int)
{
	m_strAscDesc = "asc";
	return *this;
}

const CCondition& CCondition::operator--(int)
{
	m_strAscDesc = "desc";
	return *this;
}

std::string CCondition::toString()
{
	std::string strResult = m_strSQL;
	//排序
	if (m_strOrderField != "" && m_strAscDesc != "")
	{
		strResult = strResult + " order by " + m_strOrderField + " " + m_strAscDesc;
	}
	//行数
	if (m_lineBegin >= 0 && m_lineEnd >= 0)
	{
		std::string strBeginEndLine;
		strBeginEndLine = CStringManager::Format(" limit %d,%d", m_lineBegin, m_lineEnd);
		strResult = strResult + strBeginEndLine;
	}
	return strResult;
}

void CCondition::Clear()
{
	m_strSQL = "";
	return;
}