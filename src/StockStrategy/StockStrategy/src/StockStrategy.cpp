#include "StockStrategy.h"
#include "SarRiseBack.h"
#include "StockMysql/StockMysqlAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMysql/StockMysqlAPI.h"

StockStrategy::StockStrategy()
{

}

StockStrategy& StockStrategy::instance()
{
	static StockStrategy s_stockStrategy;
	return s_stockStrategy;
}

std::vector<std::string> StockStrategy::strategyStock(StrategyEnum strategyEnum, const IntDateTime& date)
{
	switch (strategyEnum)
	{
	case SAR_RISE_BACK:
	{
		std::vector<std::string> filterStock = StockMysql::instance().readFilterStockFromRedis(date);
		for (auto itFilterStock = filterStock.begin(); itFilterStock != filterStock.end();)
		{
			if (CStringManager::Left(*itFilterStock, 3) == "688")
			{
				itFilterStock = filterStock.erase(itFilterStock);
				continue;
			}
			++itFilterStock;
		}
		return filterStock;
	}
	break;
	default:
		break;
	}
	return std::vector<std::string>();
}

std::vector<std::string> StockStrategy::strategyAllStock(StrategyEnum strategyEnum, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	if (beginTime > endTime)
	{
		return std::vector<std::string>();
	}
	std::vector<std::string> result;
	std::map<std::string, int32_t> allFilterStock;
	IntDateTime currentTime = beginTime;
	do 
	{
		std::vector<std::string> filterStock = strategyStock(strategyEnum, currentTime);
		int32_t index = -1;
		while (index++ != filterStock.size() - 1)
		{
			allFilterStock[filterStock[index]] = 0;
		}
		currentTime = currentTime + 86400;
	} while (currentTime <= endTime);

	for (auto itAllFilterStock = allFilterStock.begin(); itAllFilterStock != allFilterStock.end(); ++itAllFilterStock)
	{
		result.push_back(itAllFilterStock->first);
	}
	return result;
}

std::shared_ptr<Strategy> StockStrategy::strategy(const std::string& stock,
	const std::shared_ptr<StockMarket>& spMarket,
	StrategyEnum strategyEnum)
{
	std::shared_ptr<Strategy> strategy;
	switch (strategyEnum)
	{
	case SAR_RISE_BACK:
	{
		strategy.reset(new SarRiseBack);
	}
	break;
	default:
		break;
	}
	strategy->init(spMarket);
	return strategy;
}