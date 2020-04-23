#include "StockTrade.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "StockSolution/StockSolutionAPI.h"
#include "StockFund/StockFundAPI.h"
#include "StockStorage/StockStorageAPI.h"

StockTrade::StockTrade()
{

}

void StockTrade::init(const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const std::vector<std::string>& allStock,
	const std::vector<SolutionType>& vecSolutionType,
	const std::vector<ChooseParam>& vecChooseParam)
{
	std::set<SolutionType> solutionTypeSet;
	std::set<StrategyType> strategyTypeSet;

	int32_t index = -1;
	while (index++ != vecChooseParam.size() - 1)
	{
		solutionTypeSet.insert(vecChooseParam[index].m_solutionType);
		if (vecChooseParam[index].m_isObserve)
		{
			solutionTypeSet.insert(OBSERVE_STRATEGY);
		}
		const StrategyType& useType = vecChooseParam[index].m_useType;
		strategyTypeSet.insert(useType);

		const StrategyType& useCountType = vecChooseParam[index].m_useCountType;
		strategyTypeSet.insert(useType);
	}

	index = -1;
	while (index++ != vecSolutionType.size() - 1)
	{
		solutionTypeSet.insert(vecSolutionType[index]);
	}

	int32_t allNeedMoveDay = 0;
	std::set<std::string> allNeedLoad;
	for (auto itStrategyType = strategyTypeSet.begin(); itStrategyType != strategyTypeSet.end(); ++itStrategyType)
	{
		const StrategyType& strategyType = *itStrategyType;
		std::set<std::string> needLoad = StockStrategy::instance().strategyNeedLoad(strategyType);
		for (auto itNeedLoad = needLoad.begin(); itNeedLoad != needLoad.end(); ++itNeedLoad)
		{
			allNeedLoad.insert(*itNeedLoad);
		}
		int32_t needMoveDay = StockStrategy::instance().strategyNeedMoveDay(strategyType);
		if (allNeedMoveDay < needMoveDay)
		{
			allNeedMoveDay = needMoveDay;
		}
	}

	StockStorage::instance().init(allStock, allNeedMoveDay, beginTime, endTime);
	StockStorage::instance().loadMarket();
	StockStorage::instance().loadIndicator(allNeedLoad);
	StockStorage::instance().loadFilterStock();
	StockStorage::instance().loadStrategy(strategyTypeSet);
	StockStorage::instance().loadStrategyInfo(strategyTypeSet);
	StockStorage::instance().loadSolutionInfo(solutionTypeSet, strategyTypeSet);
	StockStorage::instance().loadSolution(solutionTypeSet, vecChooseParam);
}

void StockTrade::init(const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const std::vector<std::string>& allStock,
	SolutionType solutionType,
	const std::vector<ChooseParam>& vecChooseParam)
{
	std::vector<SolutionType> vecSolutionEnum;
	vecSolutionEnum.push_back(solutionType);
	init(beginTime, endTime, allStock, vecSolutionEnum, vecChooseParam);
}

void StockTrade::load()
{
	StockStorage::instance().load();
}

void StockTrade::setTradeParam(SolutionType solutionType, const TradeParam& tradeParam)
{
	std::shared_ptr<Solution> spSolution = StockStorage::instance().solution(solutionType);
	if (spSolution == nullptr)
	{
		return;
	}
	spSolution->setStockFund(tradeParam.m_stockFund);
	if (spSolution->type() == DISPOSABLE_STRATEGY)
	{
		const std::shared_ptr<DisposableStrategy>& disposableStrategy = std::dynamic_pointer_cast<DisposableStrategy>(spSolution);
		disposableStrategy->setOnceDate(tradeParam.m_onceDate);
	}
}

bool StockTrade::buy(std::vector<std::pair<std::string, StockInfo>>& buyStock,
	const IntDateTime& date,
	SolutionType solutionType,
	std::shared_ptr<ChooseParam>& useChooseParam)
{
	buyStock.clear();
	std::shared_ptr<Solution> spSolution = StockStorage::instance().solution(solutionType);
	if (spSolution == nullptr)
	{
		return false;
	}
	spSolution->setFilterStock(StockStorage::instance().filterStock(date));
	if (!spSolution->buy(buyStock, date))
	{
		return false;
	}

	useChooseParam.reset(new ChooseParam(spSolution->chooseParam()));
	return true;
}

bool StockTrade::sell(std::vector<std::pair<std::string, StockInfo>>& sellStock,
	const IntDateTime& date,
	SolutionType solutionType)
{
	std::shared_ptr<Solution> spSolution = StockStorage::instance().solution(solutionType);
	if (spSolution == nullptr)
	{
		return false;
	}
	spSolution->setFilterStock(StockStorage::instance().filterStock(date));
	return spSolution->sell(sellStock, date);
}

std::shared_ptr<StockMarket> StockTrade::market(const std::string& stock)
{
	return StockStorage::instance().market(stock);
}

std::shared_ptr<StockMarket> StockTrade::runMarket()
{
	return StockStorage::instance().runMarket();
}

bool StockTrade::stockDayData(const std::vector<std::string>& vecStock,
	const IntDateTime& date,
	std::map<std::string, std::shared_ptr<StockDay>>& dayData)
{
	dayData.clear();
	int32_t index = -1;
	while (index++ != vecStock.size() - 1)
	{
		const std::string& stock = vecStock[index];
		std::shared_ptr<StockMarket> spMarket = market(stock);
		if (spMarket == nullptr)
		{
			RCSend("获取了不存在的gupiao");
			dayData.clear();
			return false;
		}
		spMarket->setLastDate(date);
		dayData[stock] = spMarket->day();
	}
	return true;
}