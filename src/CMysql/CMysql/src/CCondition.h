#pragma once
#include "CMysqlMacro.h"
#include <string>
#include <stdint.h>

class CMysqlAPI CCondition
{
public:
	CCondition();
	CCondition& operator&&(CCondition con);
	CCondition& operator||(CCondition con);
	CCondition& operator!();
	CCondition& operator()(int32_t nLineBegin, int32_t nLineEnd);
	CCondition& operator[](const std::string& strOrderField);
	const CCondition& operator++(int);
	const CCondition& operator--(int);

	std::string toString();
	void Clear();

public:
	//ѡ����ʾ������0��ʼ�����м�ļ����0��1�ļ����ʾ��һ�У�ѡ��ǰ4��Ӧ����0��4
	int32_t m_lineBegin;
	int32_t m_lineEnd;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_strOrderField;
	std::string m_strAscDesc;
	std::string m_strSQL;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};