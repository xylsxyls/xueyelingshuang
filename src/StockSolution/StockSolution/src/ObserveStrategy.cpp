#include "ObserveStrategy.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "ObserveStrategyInfo.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockSolution.h"
#include "AvgFundHighScore.h"

ObserveStrategy::ObserveStrategy()
{
	m_solutionType = OBSERVE_STRATEGY;
	m_avgSolution = std::dynamic_pointer_cast<AvgFundHighScore>(StockSolution::instance().solution(AVG_FUND_HIGH_SCORE));
}

void ObserveStrategy::init(int32_t stockNum, StrategyType strategyType)
{
	m_avgSolution->init(stockNum);
	m_strategyType = strategyType;
}

bool ObserveStrategy::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_avgSolution->Solution::init(m_mapStrategy);

	m_avgSolution->setParam(m_strategyType);
	solutionInfo->setParam(m_strategyType);

	std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> beforeDayBuyStock;
	if (!m_avgSolution->buy(beforeDayBuyStock, getBeforeDay("000001", date, 2, solutionInfo), solutionInfo))
	{
		return false;
	}

	std::map<BigNumber, std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>> beforeDayBuyStockMap;

	int32_t index = -1;
	while (index++ != beforeDayBuyStock.size() - 1)
	{
		const std::string& stock = beforeDayBuyStock[index].first;
		std::shared_ptr<StockMarket> spMarket = solutionInfo->m_strategyAllInfo.find(stock)->second.begin()->second.first->m_spMarket;
		BigNumber avgChgValue = 0;
		if (!spMarket->setDate(getBeforeDay(stock, date, 2, solutionInfo)))
		{
			continue;
		}
		avgChgValue = avgChgValue + (spMarket->day()->highChgValue() + spMarket->day()->chgValue()) / 2;
		if (!spMarket->setDate(getBeforeDay(stock, date, 1, solutionInfo)))
		{
			continue;
		}
		avgChgValue = avgChgValue + (spMarket->day()->highChgValue() + spMarket->day()->chgValue()) / 2;
		beforeDayBuyStockMap[avgChgValue].push_back(beforeDayBuyStock[index]);
	}

	for (auto itStock = beforeDayBuyStockMap.rbegin(); itStock != beforeDayBuyStockMap.rend(); ++itStock)
	{
		auto& vecStock = itStock->second;
		int32_t index = -1;
		while (index++ != vecStock.size() - 1)
		{
			const std::string& stock = vecStock[index].first;
			std::shared_ptr<StockMarket> spMarket = solutionInfo->m_strategyAllInfo.find(stock)->second.begin()->second.first->m_spMarket;
			spMarket->setDate(date);
			vecStock[index].second.first = spMarket->day()->close();
			vecStock[index].second.second = 1;
			buyStock.push_back(vecStock[index]);
			if (buyStock.size() >= 4)
			{
				break;
				
			}
		}
		if (buyStock.size() >= 4)
		{
			break;
		}
	}
	if (buyStock.empty())
	{
		return false;
	}
	else if (buyStock.size() == 2)
	{
		buyStock[0].second.second = "0.5";
		return true;
	}
	else if (buyStock.size() == 3)
	{
		buyStock[0].second.second = "0.333333";
		buyStock[1].second.second = "0.5";
		return true;
	}
	else if (buyStock.size() == 4)
	{
		buyStock[0].second.second = "0.25";
		buyStock[1].second.second = "0.333333";
		buyStock[2].second.second = "0.5";
		return true;
	}
	return true;
}

bool ObserveStrategy::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_avgSolution->Solution::init(m_mapStrategy);
	return m_avgSolution->sell(sellStock, date, solutionInfo);
}

IntDateTime ObserveStrategy::getBeforeDay(const std::string& stock,
	const IntDateTime& date,
	int32_t days,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	if (solutionInfo->m_strategyAllInfo.find(stock) == solutionInfo->m_strategyAllInfo.end())
	{
		return IntDateTime(0, 0);
	}
	std::shared_ptr<StockMarket> spMarket = solutionInfo->m_strategyAllInfo.find(stock)->second.begin()->second.first->m_spMarket;
	if (!spMarket->setDate(date))
	{
		return IntDateTime(0, 0);
	}
	return spMarket->getDateBefore(days);
}