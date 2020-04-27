#include "StockType.h"

ChooseParam::ChooseParam()
{
	m_useType = STRATEGY_INIT;
	m_useCountType = STRATEGY_INIT;
	m_isObserve = false;
	m_solutionType = SOLUTION_INIT;
}

bool ChooseParam::operator==(const ChooseParam& chooseParam)
{
	return m_useType == chooseParam.m_useType &&
		m_useCountType == chooseParam.m_useCountType &&
		m_isObserve == chooseParam.m_isObserve &&
		m_solutionType == chooseParam.m_solutionType;
}

bool ChooseParam::operator!=(const ChooseParam& chooseParam)
{
	return m_useType != chooseParam.m_useType ||
		m_useCountType != chooseParam.m_useCountType ||
		m_isObserve != chooseParam.m_isObserve ||
		m_solutionType != chooseParam.m_solutionType;
}