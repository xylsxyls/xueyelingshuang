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
		std::shared_ptr<Strategy> spStrategy = StockStrategy::instance().strategy(stock, spMarket, strategyEnum);
		switch (strategyEnum)
		{
		case SAR_RISE_BACK:
		{
			std::shared_ptr<SarRiseBack> spSarRiseBack = std::dynamic_pointer_cast<SarRiseBack>(spStrategy);
			spSarRiseBack->init(StockIndicator::instance().sar(), StockIndicator::instance().boll());
		}
		break;
		default:
			break;
		}
		m_stockStrategyMap[stock] = spStrategy;
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

bool StockTrade::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>>& allBuyInfo)
{
	bool result = false;
	buyStock.clear();
	for (auto itData = m_stockStrategyMap.begin(); itData != m_stockStrategyMap.end(); ++itData)
	{
		const std::string& stock = itData->first;
		auto& stockStrategy = itData->second;
		auto itBuyInfo = allBuyInfo.find(stock);
		BigNumber price;
		BigNumber percent;
		if (stockStrategy->buy(date,
			price,
			percent,
			itBuyInfo == allBuyInfo.end() ? std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>() : itBuyInfo->second))
		{
			std::pair<std::string, std::pair<BigNumber, BigNumber>> choose;
			choose.first = stock;
			choose.second.first = price;
			choose.second.second = percent;
			buyStock.push_back(choose);
			result = true;
		}
	}
	std::sort(buyStock.begin(), buyStock.end(), sortFun);
	return result;
}

bool StockTrade::sell(const std::string& stock,
	const IntDateTime& date,
	BigNumber& price,
	BigNumber& percent,
	const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo)
{
	auto itStrategy = m_stockStrategyMap.find(stock);
	if (itStrategy == m_stockStrategyMap.end())
	{
		return false;
	}
	return itStrategy->second->sell(date, price, percent, buyInfo);
}

std::shared_ptr<StockMarket> StockTrade::market(const std::string& stock)
{
	auto itStrategy = m_stockStrategyMap.find(stock);
	if (itStrategy == m_stockStrategyMap.end())
	{
		return std::shared_ptr<StockMarket>();
	}
	std::shared_ptr<Strategy> strategy = itStrategy->second;
	return strategy->market();
}