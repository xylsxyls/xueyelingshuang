#include "DisposableStrategy.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "DisposableStrategyInfo.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"

static bool sortFun(const std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>& stock1,
	const std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>& stock2)
{
	if (stock1.second.second.first > stock2.second.second.first)
	{
		return true;
	}
	else if (stock1.second.second.first < stock2.second.second.first)
	{
		return false;
	}
	return stock1.second.second.second > stock2.second.second.second;
}

DisposableStrategy::DisposableStrategy():
m_minPollSize(0)
{
	m_solutionType = DISPOSABLE_STRATEGY;
}

bool DisposableStrategy::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	buyStock.clear();

	const std::shared_ptr<DisposableStrategyInfo>& disposableStrategyInfo = std::dynamic_pointer_cast<DisposableStrategyInfo>(solutionInfo);
	if (date != disposableStrategyInfo->m_onceDate)
	{
		return false;
	}

	std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>> strategyBuyStock;
	if (!strategyBuy(strategyBuyStock, date, solutionInfo))
	{
		return false;
	}

	if (strategyBuyCount(date, solutionInfo) < minPollSize(solutionInfo))
	{
		return false;
	}

	int32_t firstSize = strategyBuyStock.size();
	int32_t popBuySize = popSize(firstSize, solutionInfo);
	//int32_t popSize = firstSize <= 11 ? (firstSize - firstSize / 2) : (firstSize - 5);
	//int32_t popSize = firstSize <= 5 ? 0 : (firstSize - 5);
	while (popBuySize-- != 0)
	{
		strategyBuyStock.pop_back();
	}
	if (strategyBuyStock.size() == 0)
	{
		return false;
	}

	BigNumber allPercent = (int32_t)strategyBuyStock.size() * 100;
	int32_t index = -1;
	while (index++ != strategyBuyStock.size() - 1)
	{
		buyStock.push_back(std::pair<std::string, std::pair<BigNumber, BigNumber>>());
		std::pair<std::string, std::pair<BigNumber, BigNumber>>& backStock = buyStock.back();
		backStock.first = strategyBuyStock[index].first;
		backStock.second.first = strategyBuyStock[index].second.first;
		backStock.second.second = strategyBuyStock[index].second.second.first / allPercent.toPrec(2).zero();
		allPercent = allPercent - 100;
	}
	return true;
}

bool DisposableStrategy::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	sellStock.clear();
	std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>> strategySellStock;
	if (!strategySell(strategySellStock, date, solutionInfo))
	{
		return false;
	}

	int32_t index = -1;
	for (auto itSellStock = strategySellStock.begin(); itSellStock != strategySellStock.end(); ++itSellStock)
	{
		if (itSellStock->second.second.second == 100)
		{
			sellStock.push_back(std::pair<std::string, std::pair<BigNumber, BigNumber>>());
			std::pair<std::string, std::pair<BigNumber, BigNumber>>& backStock = sellStock.back();
			backStock.first = itSellStock->first;
			backStock.second.first = itSellStock->second.first;
			backStock.second.second = itSellStock->second.second.first / "100.0";
			continue;
		}
	}
	return !sellStock.empty();
}

bool DisposableStrategy::strategyBuy(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	bool result = false;
	buyStock.clear();

	const std::shared_ptr<DisposableStrategyInfo>& disposableStrategyInfo = std::dynamic_pointer_cast<DisposableStrategyInfo>(solutionInfo);

	const std::vector<std::string>& filterStock = *disposableStrategyInfo->m_filterStock;
	int32_t index = -1;
	while (index++ != filterStock.size() - 1)
	{
		const std::string& stock = filterStock[index];

		std::shared_ptr<Strategy> spStrategy = strategy(m_useType);
		std::shared_ptr<StrategyInfo> spStrategyInfo = disposableStrategyInfo->strategyInfo(stock, disposableStrategyInfo->m_useType);
		if (spStrategy == nullptr || spStrategyInfo == nullptr)
		{
			RCSend("未发现使用类型3");
			continue;
		}

		BigNumber price;
		BigNumber percent;
		BigNumber score;
		if (spStrategy->buy(date, price, percent, score, spStrategyInfo))
		{
			std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>> choose;
			choose.first = stock;
			choose.second.first = price;
			choose.second.second.first = percent;
			choose.second.second.second = score;
			buyStock.push_back(choose);
			result = true;
		}
	}
	std::sort(buyStock.begin(), buyStock.end(), sortFun);
	return result;
}

int32_t DisposableStrategy::strategyBuyCount(const IntDateTime& date, const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	const std::shared_ptr<DisposableStrategyInfo>& disposableStrategyInfo = std::dynamic_pointer_cast<DisposableStrategyInfo>(solutionInfo);

	int32_t count = 0;
	const std::vector<std::string>& filterStock = *disposableStrategyInfo->m_filterStock;
	int32_t index = -1;
	while (index++ != filterStock.size() - 1)
	{
		const std::string& stock = filterStock[index];

		std::shared_ptr<Strategy> spStrategyCount = strategyCount(m_useType);
		std::shared_ptr<StrategyInfo> spStrategyInfoCount = disposableStrategyInfo->strategyInfoCount(stock, disposableStrategyInfo->m_useType);
		if (spStrategyCount == nullptr || spStrategyInfoCount == nullptr)
		{
			RCSend("未发现使用类型4");
			continue;
		}

		BigNumber price;
		BigNumber percent;
		BigNumber score;
		count += (int32_t)spStrategyCount->buy(date, price, percent, score, spStrategyInfoCount);
	}
	return count;
}

bool DisposableStrategy::strategySell(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	bool result = false;
	sellStock.clear();
	std::vector<std::string> vecOwnedStock = solutionInfo->m_fund->ownedStock();

	int32_t index = -1;
	while (index++ != vecOwnedStock.size() - 1)
	{
		const std::string& stock = vecOwnedStock[index];

		std::shared_ptr<Strategy> spStrategy = strategy(m_useType);
		std::shared_ptr<StrategyInfo> spStrategyInfo = solutionInfo->strategyInfo(stock, solutionInfo->m_useType);
		if (spStrategy == nullptr || spStrategyInfo == nullptr)
		{
			RCSend("未发现使用类型，stock = %s", stock.c_str());
			continue;
		}

		BigNumber price;
		BigNumber percent;
		BigNumber score;
		if (spStrategy->sell(date, price, percent, score, spStrategyInfo))
		{
			std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>> sellChoose;
			sellChoose.first = stock;
			sellChoose.second.first = price;
			sellChoose.second.second.first = percent;
			sellChoose.second.second.second = score;
			sellStock.push_back(sellChoose);
			result = true;
		}
	}
	std::sort(sellStock.begin(), sellStock.end(), sortFun);
	return result;
}