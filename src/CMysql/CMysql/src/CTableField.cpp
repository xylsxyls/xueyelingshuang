#include <SDKDDKVer.h>
#include "CTableField.h"
#include "CValue.h"
#include "CCondition.h"
#include "CStringManager/CStringManagerAPI.h"

CTableField& CTableField::operator()(const std::string& Table)
{
	m_table = Table;
	return *this;
}

CTableField& CTableField::operator[](const std::string& Field)
{
	m_field = Field;
	return *this;
}

CTableField::CTableField()
{

}

CTableField::CTableField(const CTableField& tf)
{
	m_table = tf.m_table;
	m_field = tf.m_field;
}

CTableField CTableField::operator=(const CTableField& tf)
{
	m_table = tf.m_table;
	m_field = tf.m_field;
	return *this;
}

CTableField::CTableField(const std::string& Table, const std::string& Field)
{
	m_table = Table;
	m_field = Field;
}

CCondition CTableField::operator==(const CValue& value)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s.%s=%s)", m_table, m_field, value.m_strValue);
	return con;
}

CCondition CTableField::operator!=(const CValue& value)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s.%s!=%s)", m_table, m_field, value.m_strValue);
	return con;
}

CCondition CTableField::operator<(const CValue& value)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s.%s<%s)", m_table, m_field, value.m_strValue);
	return con;
}

CCondition CTableField::operator>(const CValue& value)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s.%s>%s)", m_table, m_field, value.m_strValue);
	return con;
}

CCondition CTableField::operator<=(const CValue& value)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s.%s<=%s)", m_table, m_field, value.m_strValue);
	return con;
}

CCondition CTableField::operator>=(const CValue& value)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s.%s>=%s)", m_table, m_field, value.m_strValue);
	return con;
}

//联合相等
CCondition CTableField::operator&&(const CTableField& tf)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s inner join %s on %s.%s=%s.%s)", m_table, tf.m_table, m_table, m_field, tf.m_table, tf.m_field);
	return con;
}

//左联合，左边的数据量小于等于右边的数据量
CCondition CTableField::operator<=(const CTableField& tf)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s left outer join %s on %s.%s=%s.%s)", m_table, tf.m_table, m_table, m_field, tf.m_table, tf.m_field);
	return con;
}

//右联合，左边的数据量大于等于右边的数据量
CCondition CTableField::operator>=(const CTableField& tf)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s right outer join %s on %s.%s=%s.%s)", m_table, tf.m_table, m_table, m_field, tf.m_table, tf.m_field);
	return con;
}

CCondition CTableField::operator||(const CTableField& tf)
{
	CCondition con;
	con.m_strSQL = CStringManager::Format("(%s full outer join %s on %s.%s=%s.%s)", m_table, tf.m_table, m_table, m_field, tf.m_table, tf.m_field);
	return con;
}