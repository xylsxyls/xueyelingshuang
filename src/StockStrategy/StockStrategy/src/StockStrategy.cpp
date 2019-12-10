#include "StockStrategy.h"
#include "SarRiseBack.h"
#include "SarRiseBackCount.h"
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

void StockStrategy::strategyStock(const IntDateTime& date, std::vector<std::string>& filterStock)
{
	StockMysql::instance().readFilterStockFromRedis(date, filterStock);
	for (auto itFilterStock = filterStock.begin(); itFilterStock != filterStock.end();)
	{
		if (CStringManager::Left(*itFilterStock, 3) == "688")
		{
			itFilterStock = filterStock.erase(itFilterStock);
			continue;
		}
		++itFilterStock;
	}
}

std::vector<std::string> StockStrategy::strategyAllStock(StrategyType strategyEnum, const IntDateTime& beginTime, const IntDateTime& endTime)
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
		std::vector<std::string> filterStock;
		strategyStock(currentTime, filterStock);
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

std::shared_ptr<Strategy> StockStrategy::strategy(StrategyType strategyEnum)
{
	std::shared_ptr<Strategy> strategy;
	switch (strategyEnum)
	{
	case SAR_RISE_BACK:
	{
		strategy.reset(new SarRiseBack);
	}
	break;
	case SAR_RISE_BACK_COUNT:
	{
		strategy.reset(new SarRiseBackCount);
	}
	break;
	default:
		break;
	}
	return strategy;
}

std::set<std::string> StockStrategy::strategyNeedLoad(StrategyType strategyEnum)
{
	std::set<std::string> result;
	switch (strategyEnum)
	{
	case SAR_RISE_BACK:
	case SAR_RISE_BACK_COUNT:
	{
		result.insert("market");
		result.insert("sar");
		result.insert("boll");
	}
	break;
	default:
		break;
	}
	return result;
}