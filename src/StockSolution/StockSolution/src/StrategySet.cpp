#include "StrategySet.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "StrategySetInfo.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"

static bool sortFun(const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock1,
	const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock2)
{
	if (stock1.second.second > stock2.second.second)
	{
		return true;
	}
	return false;
}

StrategySet::StrategySet()
{
	m_solutionType = STRATEGY_SET;
}

bool StrategySet::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	bool result = false;
	buyStock.clear();

	const std::shared_ptr<StrategySetInfo>& strategySetInfo = std::dynamic_pointer_cast<StrategySetInfo>(solutionInfo);

	const std::vector<std::string>& filterStock = *strategySetInfo->m_filterStock;
	int32_t index = -1;
	while (index++ != filterStock.size() - 1)
	{
		const std::string& stock = filterStock[index];

		std::shared_ptr<Strategy> spStrategy = strategy(m_useType);
		std::shared_ptr<StrategyInfo> spStrategyInfo = strategySetInfo->strategyInfo(stock, strategySetInfo->m_useType);
		if (spStrategy == nullptr || spStrategyInfo == nullptr)
		{
			RCSend("未发现使用类型5");
			continue;
		}

		BigNumber price;
		BigNumber percent;
		BigNumber score;
		if (spStrategy->buy(date, price, percent, score, spStrategyInfo))
		{
			std::pair<std::string, std::pair<BigNumber, BigNumber>> choose;
			choose.first = stock;
			choose.second.first = price;
			choose.second.second = score;
			buyStock.push_back(choose);
			result = true;
		}
	}
	std::sort(buyStock.begin(), buyStock.end(), sortFun);
	return result;
}