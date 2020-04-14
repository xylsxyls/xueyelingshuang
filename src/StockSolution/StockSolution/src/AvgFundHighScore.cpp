#include "AvgFundHighScore.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "SolutionInfo.h"

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

AvgFundHighScore::AvgFundHighScore()
{
	m_solutionType = AVG_FUND_HIGH_SCORE;
}

bool AvgFundHighScore::buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date)
{
	buyStock.clear();
	//std::vector<std::pair<std::string, StockInfo>> strategyBuyStock;
	if (!strategyBuy(buyStock, date))
	{
		return false;
	}

	//RCSend("date = %s, stocksize = %d", date.dateToString().c_str(), strategyBuyStock.size());
	if (strategyBuyCount(date) < m_solutionInfo->strategyCountInfo()->minPollSize())
	{
		return false;
	}

	int32_t firstSize = buyStock.size();
	int32_t popBuySize = m_solutionInfo->strategyInfo()->popSize(firstSize);
	//int32_t popSize = firstSize <= 11 ? (firstSize - firstSize / 2) : (firstSize - 5);
	//int32_t popSize = firstSize <= 5 ? 0 : (firstSize - 5);
	while (popBuySize-- != 0)
	{
		buyStock.pop_back();
	}
	if (buyStock.size() == 0)
	{
		return false;
	}

	int32_t stockNum = m_solutionInfo->strategyInfo()->stockNum();
	int32_t hasBuyStockNum = m_solutionInfo->m_fund->allBuyInfo()->size();
	if (stockNum <= hasBuyStockNum)
	{
		return false;
	}
	BigNumber allPercent = min((stockNum - hasBuyStockNum), (int32_t)buyStock.size()) * 100;
	
	int32_t index = -1;
	while (index++ != buyStock.size() - 1)
	{
		std::pair<std::string, StockInfo>& stockInfoPair = buyStock[index];
		stockInfoPair.second.m_rate = stockInfoPair.second.m_percent / allPercent.toPrec(2);
		allPercent = allPercent - 100;
		if (allPercent == 0)
		{
			break;
		}
	}
	return true;
}

bool AvgFundHighScore::sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date)
{
	sellStock.clear();
	if (!strategySell(sellStock, date))
	{
		return false;
	}

	int32_t index = -1;
	for (auto itSellStock = sellStock.begin(); itSellStock != sellStock.end(); ++itSellStock)
	{
		if (itSellStock->second.m_percent != 100)
		{
			continue;
		}
		itSellStock->second.m_rate = itSellStock->second.m_percent / "100.0";
	}
	return true;
}

bool AvgFundHighScore::strategyBuy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date)
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
			RCSend("未发现使用类型1");
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
			if (stockInfo.m_percent != 0)
			{
				result = true;
			}
		}
	}
	std::sort(buyStock.begin(), buyStock.end(), sortFun);
	return result;
}

int32_t AvgFundHighScore::strategyBuyCount(const IntDateTime& date)
{
	int32_t count = 0;
	const std::vector<std::string>& filterStock = *(m_solutionInfo->m_filterStock);
	int32_t index = -1;
	while (index++ != filterStock.size() - 1)
	{
		const std::string& stock = filterStock[index];
		std::shared_ptr<Strategy> spStrategyCount = m_solutionInfo->strategyCount();
		std::shared_ptr<StrategyInfo> spStrategyInfoCount = m_solutionInfo->strategyCountInfo(m_solutionInfo->m_chooseParam.m_useCountType, stock);
		if (spStrategyCount == nullptr || spStrategyInfoCount == nullptr)
		{
			RCSend("未发现使用类型2");
			continue;
		}
		spStrategyCount->setStrategyInfo(spStrategyInfoCount);
		StockInfo stockInfo;
		count += (int32_t)spStrategyCount->buy(date, stockInfo);
	}
	return count;
}

bool AvgFundHighScore::strategySell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date)
{
	bool result = false;
	sellStock.clear();
	std::vector<std::string> vecOwnedStock = m_solutionInfo->m_fund->ownedStock();

	int32_t index = -1;
	while (index++ != vecOwnedStock.size() - 1)
	{
		const std::string& stock = vecOwnedStock[index];
		std::shared_ptr<ChooseParam> spChooseParam = m_solutionInfo->m_fund->stockChooseParam(stock);
		std::shared_ptr<Strategy> spStrategy = m_solutionInfo->strategy(spChooseParam->m_useType);
		std::shared_ptr<StrategyInfo> spStrategyInfo = m_solutionInfo->strategyInfo(spChooseParam->m_useType, stock);
		if (spStrategy == nullptr || spStrategyInfo == nullptr)
		{
			RCSend("未发现使用类型6，stock = %s", stock.c_str());
			continue;
		}

		StockInfo stockInfo;
		spStrategy->setStrategyInfo(spStrategyInfo);
		bool hasSellStock = false;
		if (m_solutionInfo->m_chooseParam.m_isObserve)
		{
			hasSellStock = spStrategy->observeSell(date, stockInfo);
		}
		else
		{
			hasSellStock = spStrategy->sell(date, stockInfo);
		}
		if (hasSellStock)
		{
			std::pair<std::string, StockInfo> sellChoose;
			sellChoose.first = stock;
			sellChoose.second = stockInfo;
			sellStock.push_back(sellChoose);
			if (stockInfo.m_percent == 100)
			{
				result = true;
			}
		}
	}
	std::sort(sellStock.begin(), sellStock.end(), sortFun);
	return result;
}