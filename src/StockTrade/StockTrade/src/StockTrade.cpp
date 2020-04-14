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
	const std::vector<ChooseParam>& vecChooseParam)
{
	std::set<std::string> allNeedLoad;
	int32_t index = -1;
	while (index++ != vecChooseParam.size() - 1)
	{
		std::set<std::string> needLoad = StockStrategy::instance().strategyNeedLoad(vecChooseParam[index].m_useType);
		for (auto itNeedLoad = needLoad.begin(); itNeedLoad != needLoad.end(); ++itNeedLoad)
		{
			allNeedLoad.insert(*itNeedLoad);
		}
		needLoad = StockStrategy::instance().strategyNeedLoad(vecChooseParam[index].m_useCountType);
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
	while (index++ != vecChooseParam.size() - 1)
	{
		const StrategyType& useType = vecChooseParam[index].m_useType;
		m_strategyMap[useType] = StockStrategy::instance().strategy(useType);

		int32_t stockIndex = -1;
		while (stockIndex++ != m_allStock.size() - 1)
		{
			const std::string& stock = m_allStock[stockIndex];
			m_strategyInfoMap[stock][useType] = StockStrategy::instance().strategyInfo(useType,
				nullptr,
				m_spMarketMap.find(stock)->second,
				m_spIndicatorMap.find(stock)->second);
		}

		const StrategyType& useCountType = vecChooseParam[index].m_useCountType;
		if (useCountType == useType)
		{
			continue;
		}
		m_strategyMap[useCountType] = StockStrategy::instance().strategy(useCountType);

		stockIndex = -1;
		while (stockIndex++ != m_allStock.size() - 1)
		{
			const std::string& stock = m_allStock[stockIndex];
			m_strategyInfoMap[stock][useCountType] = StockStrategy::instance().strategyInfo(useCountType,
				nullptr,
				m_spMarketMap.find(stock)->second,
				m_spIndicatorMap.find(stock)->second);
		}
	}
	
	index = -1;
	while (index++ != vecSolutionType.size() - 1)
	{
		std::shared_ptr<Solution> spSolution = StockSolution::instance().solution(vecSolutionType[index]);
		const SolutionType& solutionType = vecSolutionType[index];
		switch (solutionType)
		{
		case AVG_FUND_HIGH_SCORE:
		{
			std::shared_ptr<AvgFundHighScore> spAvgFundHighScore = std::dynamic_pointer_cast<AvgFundHighScore>(spSolution);
			spAvgFundHighScore->setSolutionInfo(makeSolutionInfo(solutionType, vecChooseParam));
			spAvgFundHighScore->setChooseParam(vecChooseParam[0]);
		}
		break;
		case INTEGRATED_STRATEGY:
		{
			std::shared_ptr<IntegratedStrategy> spIntegratedStrategy = std::dynamic_pointer_cast<IntegratedStrategy>(spSolution);
			spIntegratedStrategy->init(vecChooseParam);
			spIntegratedStrategy->setSolutionInfo(makeSolutionInfo(solutionType, vecChooseParam));
		}
		break;
		case OBSERVE_STRATEGY:
		{
			std::shared_ptr<ObserveStrategy> spObserveStrategy = std::dynamic_pointer_cast<ObserveStrategy>(spSolution);
			spObserveStrategy->init(4, 2);
			spObserveStrategy->setSolutionInfo(makeSolutionInfo(solutionType, vecChooseParam));
		}
		break;
		case DISPOSABLE_STRATEGY:
		{
			std::shared_ptr<DisposableStrategy> spDisposableStrategy = std::dynamic_pointer_cast<DisposableStrategy>(spSolution);
			spDisposableStrategy->setSolutionInfo(makeSolutionInfo(solutionType, vecChooseParam));
		}
		break;
		default:
			break;
		}
		m_solutionMap[vecSolutionType[index]] = spSolution;
	}
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

void StockTrade::setTradeParam(SolutionType solutionType, const TradeParam& tradeParam)
{
	auto itSolution = m_solutionMap.find(solutionType);
	if (itSolution == m_solutionMap.end())
	{
		return;
	}
	const std::shared_ptr<Solution>& spSolution = itSolution->second;
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

	auto itSolution = m_solutionMap.find(solutionType);
	if (itSolution == m_solutionMap.end())
	{
		return false;
	}
	const std::shared_ptr<Solution>& spSolution = itSolution->second;
	spSolution->setFilterStock(&(m_filterStock.find(date)->second));
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
	auto itSolution = m_solutionMap.find(solutionType);
	if (itSolution == m_solutionMap.end())
	{
		return false;
	}
	const std::shared_ptr<Solution>& spSolution = itSolution->second;
	spSolution->setFilterStock(&(m_filterStock.find(date)->second));
	return spSolution->sell(sellStock, date);
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

std::shared_ptr<SolutionInfo> StockTrade::makeSolutionInfo(SolutionType solutionType,
	const std::vector<ChooseParam>& vecChooseParam)
{
	std::shared_ptr<SolutionInfo> spSolutionInfo = StockSolution::instance().solutionInfo(solutionType);
	//spSolutionInfo->m_filterStock = &(m_filterStock.find(date)->second);
	//spSolutionInfo->m_fund = stockFund;

	if (solutionType == DISPOSABLE_STRATEGY)
	{
		const std::shared_ptr<DisposableStrategyInfo>& disposableStrategyInfo = std::dynamic_pointer_cast<DisposableStrategyInfo>(spSolutionInfo);
		//disposableStrategyInfo->m_onceDate = onceDate;
		disposableStrategyInfo->m_chooseParam.m_useType = vecChooseParam[0].m_useType;
	}
	if (solutionType == AVG_FUND_HIGH_SCORE)
	{
		spSolutionInfo->m_chooseParam.m_useType = vecChooseParam[0].m_useType;
	}
	if (solutionType == OBSERVE_STRATEGY)
	{
		spSolutionInfo->m_chooseParam.m_isObserve = true;
	}

	int32_t index = -1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[index];
		int32_t strategyIndex = -1;
		while (strategyIndex++ != vecChooseParam.size() - 1)
		{
			const StrategyType& strategyTypeFirst = vecChooseParam[strategyIndex].m_useType;
			spSolutionInfo->m_allStrategy[strategyTypeFirst] = m_strategyMap.find(strategyTypeFirst)->second;
			spSolutionInfo->m_allStrategyInfo[stock][strategyTypeFirst] = m_strategyInfoMap.find(stock)->second.find(strategyTypeFirst)->second;

			const StrategyType& strategyTypeSecond = vecChooseParam[strategyIndex].m_useCountType;
			if (strategyTypeSecond == strategyTypeFirst)
			{
				continue;
			}
			spSolutionInfo->m_allStrategy[strategyTypeSecond] = m_strategyMap.find(strategyTypeSecond)->second;
			spSolutionInfo->m_allStrategyInfo[stock][strategyTypeSecond] = m_strategyInfoMap.find(stock)->second.find(strategyTypeSecond)->second;
		}
	}
	return spSolutionInfo;
}