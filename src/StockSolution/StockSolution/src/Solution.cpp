#include "Solution.h"
#include "SolutionInfo.h"

void Solution::setSolutionInfo(const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_solutionInfo = solutionInfo;
}

void Solution::setChooseParam(const ChooseParam& chooseParam)
{
	m_solutionInfo->m_chooseParam = chooseParam;
}

ChooseParam Solution::chooseParam()
{
	return m_solutionInfo->m_chooseParam;
}

void Solution::setFilterStock(std::vector<std::string>* filterStock)
{
	m_solutionInfo->m_filterStock = filterStock;
}

void Solution::setStockFund(StockFund* stockFund)
{
	m_solutionInfo->m_fund = stockFund;
	for (auto itStrategyInfoMap = m_solutionInfo->m_allStrategyInfo.begin(); itStrategyInfoMap != m_solutionInfo->m_allStrategyInfo.end(); ++itStrategyInfoMap)
	{
		for (auto itStrategyInfo = itStrategyInfoMap->second.begin(); itStrategyInfo != itStrategyInfoMap->second.end(); ++itStrategyInfo)
		{
			itStrategyInfo->second->m_fund = stockFund;
		}
	}
}

bool Solution::buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date)
{
	return false;
}

bool Solution::sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date)
{
	return false;
}

SolutionType Solution::type()
{
	return m_solutionType;
}