#include <SDKDDKVer.h>
#include "CAttri.h"

CAttri::CAttri()
{
	m_type = 0;
	m_length = 0;
	m_hasDefault = 0;
	m_bPrimaryKey = 0;
	m_bUniqueKey = 0;
	m_bNotNull = 0;
	m_bBlob = 0;
	m_bAutoIncrement = 0;
	m_bZeroFill = 0;
}

CAttri::CAttri(const CAttri& attri)
{
	m_type = attri.m_type;
	m_strFieldName = attri.m_strFieldName;
	m_length = attri.m_length;
	m_hasDefault = attri.m_hasDefault;
	m_vDefault = attri.m_vDefault;
	m_bPrimaryKey = attri.m_bPrimaryKey;
	m_bUniqueKey = attri.m_bUniqueKey;
	m_bNotNull = attri.m_bNotNull;
	m_bBlob = attri.m_bBlob;
	m_bAutoIncrement = attri.m_bAutoIncrement;
	m_bZeroFill = attri.m_bZeroFill;
}

CAttri CAttri::operator = (const CAttri& attri)
{
	m_type = attri.m_type;
	m_strFieldName = attri.m_strFieldName;
	m_length = attri.m_length;
	m_hasDefault = attri.m_hasDefault;
	m_vDefault = attri.m_vDefault;
	m_bPrimaryKey = attri.m_bPrimaryKey;
	m_bUniqueKey = attri.m_bUniqueKey;
	m_bNotNull = attri.m_bNotNull;
	m_bBlob = attri.m_bBlob;
	m_bAutoIncrement = attri.m_bAutoIncrement;
	m_bZeroFill = attri.m_bZeroFill;
	return *this;
}

//CAttri& CAttri::operator->()
//{
//	return *this;
//}