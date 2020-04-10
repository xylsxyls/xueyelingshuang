#include "IntegratedStrategy.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "IntegratedStrategyInfo.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockSolution.h"
#include "AvgFundHighScore.h"

IntegratedStrategy::IntegratedStrategy()
{
	m_solutionType = INTEGRATED_STRATEGY;
	m_avgSolution = std::dynamic_pointer_cast<AvgFundHighScore>(StockSolution::instance().solution(AVG_FUND_HIGH_SCORE));
}

void IntegratedStrategy::init(const std::vector<ChooseParam>& vecChooseParam)
{
	m_vecChooseParam = vecChooseParam;
}

void IntegratedStrategy::setSolutionInfo(const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_solutionInfo = solutionInfo;
	m_avgSolution->setSolutionInfo(solutionInfo);
}

bool IntegratedStrategy::buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date)
{
	int32_t index = -1;
	while (index++ != m_vecChooseParam.size() - 1)
	{
		const ChooseParam& chooseParam = m_vecChooseParam[index];
		m_avgSolution->setChooseParam(chooseParam);
		if (m_avgSolution->buy(buyStock, date))
		{
			return true;
		}
	}
	return false;
}

bool IntegratedStrategy::sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date)
{
	return m_avgSolution->sell(sellStock, date);
}