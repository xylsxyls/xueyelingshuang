#include "StockStrategy.h"
#include "SarRiseBack.h"
#include "SarRiseBackCount.h"
#include "StockMysql/StockMysqlAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include "CatchUp.h"
#include "SarRiseBackThirtyLine.h"
#include "LineBack.h"
#include "SarRiseBackInfo.h"
#include "CatchUpInfo.h"
#include "LineBackInfo.h"
#include "SarRiseBackThirtyLineInfo.h"
#include "T0.h"
#include "T0Info.h"

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

std::vector<std::string> StockStrategy::strategyAllStock(const IntDateTime& beginTime, const IntDateTime& endTime)
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
	case CATCH_UP:
	{
		strategy.reset(new CatchUp);
	}
	break;
	case SAR_RISE_BACK_THIRTY_LINE:
	{
		strategy.reset(new SarRiseBackThirtyLine);
	}
	break;
	case LINE_BACK:
	{
		strategy.reset(new LineBack);
	}
	break;
	case T_ADD_0:
	{
		strategy.reset(new T0);
	}
	break;
	default:
		break;
	}
	return strategy;
}

std::shared_ptr<StrategyInfo> StockStrategy::strategyInfo(StrategyType strategyEnum,
	StockFund* stockFund,
	const std::shared_ptr<StockMarket>& spMarket,
	const std::map<std::string, std::shared_ptr<IndicatorManagerBase>>& spIndicator)
{
	std::shared_ptr<StrategyInfo> spStrategyInfo = strategyInfoNew(strategyEnum);

	spStrategyInfo->m_fund = stockFund;
	spStrategyInfo->m_spMarket = spMarket;
	std::set<std::string> needLoadIndicator = spStrategyInfo->needIndicator();
	for (auto itNeedLoadIndicator = needLoadIndicator.begin(); itNeedLoadIndicator != needLoadIndicator.end(); ++itNeedLoadIndicator)
	{
		const std::string& indicatorType = *itNeedLoadIndicator;
		spStrategyInfo->m_spIndicator[indicatorType] = spIndicator.find(indicatorType)->second;
	}

	return spStrategyInfo;
}

std::set<std::string> StockStrategy::strategyNeedLoad(StrategyType strategyEnum)
{
	return strategyInfoNew(strategyEnum)->needIndicator();
}

int32_t StockStrategy::strategyNeedMoveDay(StrategyType strategyEnum)
{
	return strategyInfoNew(strategyEnum)->needMoveDay();
}

std::shared_ptr<StrategyInfo> StockStrategy::strategyInfoNew(StrategyType strategyEnum)
{
	std::shared_ptr<StrategyInfo> spStrategyInfo;
	switch (strategyEnum)
	{
	case SAR_RISE_BACK:
	case SAR_RISE_BACK_COUNT:
	{
		spStrategyInfo.reset(new SarRiseBackInfo);
	}
	break;
	case SAR_RISE_BACK_THIRTY_LINE:
	{
		spStrategyInfo.reset(new SarRiseBackThirtyLineInfo);
	}
	break;
	case CATCH_UP:
	{
		spStrategyInfo.reset(new CatchUpInfo);
	}
	break;
	case LINE_BACK:
	{
		spStrategyInfo.reset(new LineBackInfo);
	}
	break;
	case T_ADD_0:
	{
		spStrategyInfo.reset(new T0Info);
	}
	break;
	default:
		break;
	}
	return spStrategyInfo;
}