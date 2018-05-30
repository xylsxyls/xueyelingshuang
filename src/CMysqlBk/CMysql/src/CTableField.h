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
	//����
	CCondition operator&&(const CTableField& tf);
	//�����ϣ���ߵ����������ڵ����ұߵ�������
	CCondition operator<=(const CTableField& tf);
	//�����ϣ���ߵ�������С�ڵ����ұߵ�������
	CCondition operator>=(const CTableField& tf);
	//ȫ��
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