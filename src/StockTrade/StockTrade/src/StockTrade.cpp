#include "StockTrade.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "StockSolution/StockSolutionAPI.h"
#include "StockFund/StockFundAPI.h"

StockTrade::StockTrade()
{

}

void StockTrade::init(const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const std::vector<std::string>& allStock,
	const std::vector<SolutionType>& vecSolutionType,
	const std::vector<StrategyType>& vecStrategyType)
{
	std::set<std::string> allNeedLoad;
	int32_t index = -1;
	while (index++ != vecStrategyType.size() - 1)
	{
		std::set<std::string> needLoad = StockStrategy::instance().strategyNeedLoad(vecStrategyType[index]);
		for (auto itNeedLoad = needLoad.begin(); itNeedLoad != needLoad.end(); ++itNeedLoad)
		{
			allNeedLoad.insert(*itNeedLoad);
		}
	}
	
	m_allStock = allStock;

	int32_t moveDay = 90;
	index = -1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[index];
		std::map<std::string, std::shared_ptr<IndicatorManagerBase>>& stockIndicatorMap = m_spIndicatorMap[stock];
		
		std::shared_ptr<StockMarket> spMarket(new StockMarket);
		spMarket->loadFromRedis(stock, beginTime - moveDay * 86400, endTime);
		m_spMarketMap[stock] = spMarket;

		StockIndicator::instance().loadIndicatorFromRedis(stock, beginTime - moveDay * 86400, endTime);
		for (auto itAllNeedLoad = allNeedLoad.begin(); itAllNeedLoad != allNeedLoad.end(); ++itAllNeedLoad)
		{
			if (*itAllNeedLoad == "wr")
			{
				stockIndicatorMap["wr"] = StockIndicator::instance().wr();
			}
			else if (*itAllNeedLoad == "rsi")
			{
				stockIndicatorMap["rsi"] = StockIndicator::instance().rsi();
			}
			else if (*itAllNeedLoad == "sar")
			{
				stockIndicatorMap["sar"] = StockIndicator::instance().sar();
			}
			else if (*itAllNeedLoad == "boll")
			{
				stockIndicatorMap["boll"] = StockIndicator::instance().boll();
			}
		}
	}

	IntDateTime currentTime = beginTime;
	while (true)
	{
		StockStrategy::instance().strategyStock(currentTime, m_filterStock[currentTime]);
		currentTime = currentTime + 86400;
		if (currentTime > endTime)
		{
			break;
		}
	}
	
	index = -1;
	while (index++ != vecSolutionType.size() - 1)
	{
		std::shared_ptr<Solution> spSolution = StockSolution::instance().solution(vecSolutionType[index]);
		switch (vecSolutionType[index])
		{
		case AVG_FUND_HIGH_SCORE:
		{
			(std::dynamic_pointer_cast<AvgFundHighScore>(spSolution))->init(4);
			(std::dynamic_pointer_cast<AvgFundHighScore>(spSolution))->setParam(vecStrategyType[0]);
		}
		break;
		case INTEGRATED_STRATEGY:
		{
			(std::dynamic_pointer_cast<IntegratedStrategy>(spSolution))->init(4, vecStrategyType);
		}
		break;
		case OBSERVE_STRATEGY:
		{
			(std::dynamic_pointer_cast<ObserveStrategy>(spSolution))->init(4, vecStrategyType[0], 2);
		}
		break;
		default:
			break;
		}
		m_solutionMap[vecSolutionType[index]] = spSolution;
	}

	index = -1;
	while (index++ != vecStrategyType.size() - 1)
	{
		std::shared_ptr<Strategy> spStrategy = StockStrategy::instance().strategy(vecStrategyType[index]);
		m_strategyMap[vecStrategyType[index]] = spStrategy;
	}
}

void StockTrade::init(const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const std::vector<std::string>& allStock,
	SolutionType solutionType,
	const std::vector<StrategyType>& vecStrategyType)
{
	std::vector<SolutionType> vecSolutionEnum;
	vecSolutionEnum.push_back(solutionType);
	init(beginTime, endTime, allStock, vecSolutionEnum, vecStrategyType);
}

void StockTrade::load()
{
	for (auto itMarket = m_spMarketMap.begin(); itMarket != m_spMarketMap.end(); ++itMarket)
	{
		itMarket->second->load();
	}
	for (auto itStockIndicatorMap = m_spIndicatorMap.begin(); itStockIndicatorMap != m_spIndicatorMap.end(); ++itStockIndicatorMap)
	{
		const std::map<std::string, std::shared_ptr<IndicatorManagerBase>>& spStockIndicator = itStockIndicatorMap->second;
		for (auto itIndicator = spStockIndicator.begin(); itIndicator != spStockIndicator.end(); ++itIndicator)
		{
			itIndicator->second->load();
		}
	}
}

bool sortFun(const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock1,
	const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock2)
{
	return stock1.second.second > stock2.second.second;
}

bool StockTrade::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	StockFund* stockFund,
	SolutionType solutionType,
	const std::vector<std::pair<StrategyType, StrategyType>>& vecStrategyType,
	StrategyType& useStrategyType,
	const IntDateTime& onceDate)
{
	buyStock.clear();

	auto itSolution = m_solutionMap.find(solutionType);
	if (itSolution == m_solutionMap.end())
	{
		return false;
	}
	const std::shared_ptr<Solution>& spSolution = itSolution->second;

	std::map<StrategyType, std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>> mapStrategy;
	int32_t index = -1;
	while (index++ != vecStrategyType.size() - 1)
	{
		auto itStrategyFirst = m_strategyMap.find(vecStrategyType[index].first);
		if (itStrategyFirst == m_strategyMap.end())
		{
			return false;
		}
		auto itStrategySecond = m_strategyMap.find(vecStrategyType[index].second);
		if (itStrategySecond == m_strategyMap.end())
		{
			return false;
		}
		std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>& strategyPair = mapStrategy[vecStrategyType[index].first];
		strategyPair.first = itStrategyFirst->second;
		strategyPair.second = itStrategySecond->second;
	}
	spSolution->init(mapStrategy);
	spSolution->setParam(vecStrategyType[0].first);
	std::shared_ptr<SolutionInfo> buySolutionInfo = makeSolutionInfo(date, stockFund, solutionType, vecStrategyType, onceDate);
	if (!spSolution->buy(buyStock, date, buySolutionInfo))
	{
		return false;
	}
	useStrategyType = buySolutionInfo->m_useType;
	return true;
}

bool StockTrade::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
	const IntDateTime& date,
	StockFund* stockFund,
	SolutionType solutionType,
	const std::vector<std::pair<StrategyType, StrategyType>>& vecStrategyType)
{
	auto itSolution = m_solutionMap.find(solutionType);
	if (itSolution == m_solutionMap.end())
	{
		return false;
	}
	const std::shared_ptr<Solution>& spSolution = itSolution->second;

	std::map<StrategyType, std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>> mapStrategy;
	int32_t index = -1;
	while (index++ != vecStrategyType.size() - 1)
	{
		auto itStrategyFirst = m_strategyMap.find(vecStrategyType[index].first);
		if (itStrategyFirst == m_strategyMap.end())
		{
			return false;
		}
		auto itStrategySecond = m_strategyMap.find(vecStrategyType[index].second);
		if (itStrategySecond == m_strategyMap.end())
		{
			return false;
		}
		std::pair<std::shared_ptr<Strategy>, std::shared_ptr<Strategy>>& strategyPair = mapStrategy[vecStrategyType[index].first];
		strategyPair.first = itStrategyFirst->second;
		strategyPair.second = itStrategySecond->second;
	}
	spSolution->init(mapStrategy);
	spSolution->setParam(vecStrategyType[0].first);
	std::shared_ptr<SolutionInfo> solutionInfo = makeSolutionInfo(date,
		stockFund,
		solutionType,
		vecStrategyType,
		IntDateTime(0, 0));
	return spSolution->sell(sellStock, date, solutionInfo);
}

std::shared_ptr<StockMarket> StockTrade::market(const std::string& stock)
{
	auto itMarket = m_spMarketMap.find(stock);
	if (itMarket == m_spMarketMap.end())
	{
		return std::shared_ptr<StockMarket>();
	}
	return itMarket->second;
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

std::shared_ptr<SolutionInfo> StockTrade::makeSolutionInfo(const IntDateTime& date,
	StockFund* stockFund,
	SolutionType solutionType,
	const std::vector<std::pair<StrategyType, StrategyType>>& vecStrategyType,
	const IntDateTime& onceDate)
{
	std::shared_ptr<SolutionInfo> spSolutionInfo = StockSolution::instance().solutionInfo(solutionType);
	spSolutionInfo->m_filterStock = &(m_filterStock.find(date)->second);
	spSolutionInfo->m_fund = stockFund;

	if (solutionType == DISPOSABLE_STRATEGY)
	{
		const std::shared_ptr<DisposableStrategyInfo>& disposableStrategyInfo = std::dynamic_pointer_cast<DisposableStrategyInfo>(spSolutionInfo);
		disposableStrategyInfo->m_onceDate = onceDate;
		disposableStrategyInfo->m_useType = vecStrategyType[0].first;
	}
	if (solutionType == AVG_FUND_HIGH_SCORE)
	{
		spSolutionInfo->m_useType = vecStrategyType[0].first;
	}

	int32_t index = -1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[index];
		int32_t strategyIndex = -1;
		while (strategyIndex++ != vecStrategyType.size() - 1)
		{
			const StrategyType& strategyTypeFirst = vecStrategyType[strategyIndex].first;
			const StrategyType& strategyTypeSecond = vecStrategyType[strategyIndex].second;
			auto& strategyInfoPair = spSolutionInfo->m_strategyAllInfo[stock][strategyTypeFirst];
			strategyInfoPair.first = StockStrategy::instance().strategyInfo(strategyTypeFirst,
				stockFund,
				m_spMarketMap.find(stock)->second,
				m_spIndicatorMap.find(stock)->second);
			strategyInfoPair.second = StockStrategy::instance().strategyInfo(strategyTypeSecond,
				stockFund,
				m_spMarketMap.find(stock)->second,
				m_spIndicatorMap.find(stock)->second);
		}
	}
	return spSolutionInfo;
}