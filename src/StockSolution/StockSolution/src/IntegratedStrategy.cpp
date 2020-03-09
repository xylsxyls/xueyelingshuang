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

void IntegratedStrategy::init(int32_t stockNum, const std::vector<StrategyType>& vecStrategyType)
{
	m_avgSolution->init(stockNum);
	m_vecStrategyType = vecStrategyType;
}

bool IntegratedStrategy::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_avgSolution->Solution::init(m_mapStrategy);

	int32_t index = -1;
	while (index++ != m_vecStrategyType.size() - 1)
	{
		const StrategyType& useType = m_vecStrategyType[index];
		m_avgSolution->setParam(useType);
		solutionInfo->setParam(useType);
		if (m_avgSolution->buy(buyStock, date, solutionInfo))
		{
			return true;
		}
	}
	return false;
}

bool IntegratedStrategy::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_avgSolution->Solution::init(m_mapStrategy);
	return m_avgSolution->sell(sellStock, date, solutionInfo);
}