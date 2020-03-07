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

void IntegratedStrategy::init(int32_t stockNum)
{
	m_avgSolution->init(stockNum);
}

bool IntegratedStrategy::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	if (solutionInfo->m_strategyAllInfo.empty())
	{
		return false;
	}

	std::vector<std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>> vecStrategy = m_vecStrategy;
	
	while (true)
	{
		auto& strategyMap = solutionInfo->m_strategyAllInfo.begin()->second;
		if (strategyMap.empty())
		{
			return false;
		}
		m_avgSolution->Solution::init(vecStrategy);
		if (m_avgSolution->buy(buyStock, date, solutionInfo))
		{
			return true;
		}
		for (auto itStrategyInfo = solutionInfo->m_strategyAllInfo.begin(); itStrategyInfo != solutionInfo->m_strategyAllInfo.end(); ++itStrategyInfo)
		{
			auto& strategyInfoMap = itStrategyInfo->second;
			strategyInfoMap.erase(strategyInfoMap.begin());
		}
		vecStrategy.erase(vecStrategy.begin());
	}
	return false;
}

bool IntegratedStrategy::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_avgSolution->Solution::init(m_vecStrategy);
	return m_avgSolution->sell(sellStock, date, solutionInfo);
}