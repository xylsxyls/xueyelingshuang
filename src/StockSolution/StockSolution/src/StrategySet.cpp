#include "StrategySet.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "StrategySetInfo.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"

static bool sortFun(const std::pair<std::string, StockInfo>& stock1,
	const std::pair<std::string, StockInfo>& stock2)
{
	if (stock1.second.m_percent > stock2.second.m_percent)
	{
		return true;
	}
	else if (stock1.second.m_percent < stock2.second.m_percent)
	{
		return false;
	}
	return stock1.second.m_score > stock2.second.m_score;
}

StrategySet::StrategySet()
{
	m_solutionType = STRATEGY_SET;
}

bool StrategySet::buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date)
{
	bool result = false;
	buyStock.clear();

	const std::vector<std::string>& filterStock = *(m_solutionInfo->m_filterStock);
	int32_t index = -1;
	while (index++ != filterStock.size() - 1)
	{
		const std::string& stock = filterStock[index];

		std::shared_ptr<Strategy> spStrategy = m_solutionInfo->strategy();
		std::shared_ptr<StrategyInfo> spStrategyInfo = m_solutionInfo->strategyInfo(m_solutionInfo->m_chooseParam.m_useType, stock);
		if (spStrategy == nullptr || spStrategyInfo == nullptr)
		{
			RCSend("未发现使用类型5");
			continue;
		}

		StockInfo stockInfo;
		spStrategy->setStrategyInfo(spStrategyInfo);
		if (spStrategy->buy(date, stockInfo))
		{
			std::pair<std::string, StockInfo> choose;
			choose.first = stock;
			choose.second = stockInfo;
			buyStock.push_back(choose);
			result = true;
		}
	}
	std::sort(buyStock.begin(), buyStock.end(), sortFun);
	return result;
}