#include "StockTrade.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>

void StockTrade::init(const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const std::vector<std::string>& allStock,
	StrategyEnum strategyEnum)
{
	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		const std::string& stock = allStock[index];
		std::shared_ptr<StockMarket> spMarket(new StockMarket);
		spMarket->loadFromRedis(stock, beginTime, endTime);
		StockIndicator::instance().loadIndicatorFromRedis(stock, beginTime, endTime);
		m_stockStrategyMap[stock] = StockStrategy::instance().strategy(stock,
			spMarket,
			StockIndicator::instance().wr(),
			StockIndicator::instance().rsi(),
			StockIndicator::instance().sar(),
			StockIndicator::instance().boll(),
			strategyEnum);
	}
}

void StockTrade::load()
{
	for (auto itData = m_stockStrategyMap.begin(); itData != m_stockStrategyMap.end(); ++itData)
	{
		auto& stockStrategy = itData->second;
		stockStrategy->load();
	}
}

bool sortFun(const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock1,
	const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock2)
{
	return stock1.second.second > stock2.second.second;
}

bool StockTrade::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock, const IntDateTime& date)
{
	bool result = false;
	buyStock.clear();
	for (auto itData = m_stockStrategyMap.begin(); itData != m_stockStrategyMap.end(); ++itData)
	{
		auto& stockStrategy = itData->second;
		BigNumber price;
		BigNumber percent;
		if (stockStrategy->buy(date, price, percent))
		{
			std::pair<std::string, std::pair<BigNumber, BigNumber>> choose;
			choose.first = itData->first;
			choose.second.first = price;
			choose.second.second = percent;
			buyStock.push_back(choose);
			result = true;
		}
	}
	std::sort(buyStock.begin(), buyStock.end(), sortFun);
	return result;
}

bool StockTrade::sell(const std::string& stock, const IntDateTime& date, BigNumber& price, BigNumber& percent)
{
	auto itStrategy = m_stockStrategyMap.find(stock);
	if (itStrategy == m_stockStrategyMap.end())
	{
		return false;
	}
	return itStrategy->second->sell(date, price, percent);
}