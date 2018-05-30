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
	//选择显示行数从0开始，看中间的间隔，0和1的间隔表示第一行，选择前4行应该是0到4
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