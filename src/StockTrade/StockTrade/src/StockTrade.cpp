#include "StockTrade.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "StockSolution/StockSolutionAPI.h"

StockTrade::StockTrade()
{

}

void StockTrade::init(const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const std::vector<std::string>& allStock,
	const std::vector<SolutionEnum>& vecSolutionEnum,
	const std::vector<StrategyEnum>& vecStrategyEnum)
{
	std::set<std::string> allNeedLoad;
	int32_t index = -1;
	while (index++ != vecStrategyEnum.size() - 1)
	{
		std::set<std::string> needLoad = StockStrategy::instance().strategyNeedLoad(vecStrategyEnum[index]);
		for (auto itNeedLoad = needLoad.begin(); itNeedLoad != needLoad.end(); ++itNeedLoad)
		{
			allNeedLoad.insert(*itNeedLoad);
		}
	}
	
	m_allStock = allStock;
	index = -1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[index];
		StockIndicator::instance().loadIndicatorFromRedis(stock, beginTime, endTime);
		for (auto itAllNeedLoad = allNeedLoad.begin(); itAllNeedLoad != allNeedLoad.end(); ++itAllNeedLoad)
		{
			if (*itAllNeedLoad == "market")
			{
				std::shared_ptr<StockMarket> spMarket(new StockMarket);
				spMarket->loadFromRedis(stock, beginTime, endTime);
				m_spMarketMap[stock] = spMarket;
			}
			else if (*itAllNeedLoad == "wr")
			{
				m_spWrIndicatorMap[stock] = StockIndicator::instance().wr();
			}
			else if (*itAllNeedLoad == "rsi")
			{
				m_spRsiIndicatorMap[stock] = StockIndicator::instance().rsi();
			}
			else if (*itAllNeedLoad == "sar")
			{
				m_spSarIndicatorMap[stock] = StockIndicator::instance().sar();
			}
			else if (*itAllNeedLoad == "boll")
			{
				m_spBollIndicatorMap[stock] = StockIndicator::instance().boll();
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
	while (index++ != vecSolutionEnum.size() - 1)
	{
		std::shared_ptr<Solution> spSolution = StockSolution::instance().solution(vecSolutionEnum[index]);
		m_solutionMap[vecSolutionEnum[index]] = spSolution;
	}

	index = -1;
	while (index++ != vecStrategyEnum.size() - 1)
	{
		std::shared_ptr<Strategy> spStrategy = StockStrategy::instance().strategy(vecStrategyEnum[index]);
		m_strategyMap[vecStrategyEnum[index]] = spStrategy;
	}
}

void StockTrade::init(const IntDateTime& beginTime,
	const IntDateTime& endTime,
	const std::vector<std::string>& allStock,
	SolutionEnum solutionEnum,
	StrategyEnum strategyEnum)
{
	std::vector<SolutionEnum> vecSolutionEnum;
	vecSolutionEnum.push_back(solutionEnum);
	std::vector<StrategyEnum> vecStrategyEnum;
	vecStrategyEnum.push_back(strategyEnum);
	init(beginTime, endTime, allStock, vecSolutionEnum, vecStrategyEnum);
}

void StockTrade::load()
{
	for (auto itMarket = m_spMarketMap.begin(); itMarket != m_spMarketMap.end(); ++itMarket)
	{
		itMarket->second->load();
	}
	for (auto itWrIndicator = m_spWrIndicatorMap.begin(); itWrIndicator != m_spWrIndicatorMap.end(); ++itWrIndicator)
	{
		itWrIndicator->second->load();
	}
	for (auto itRsiIndicator = m_spRsiIndicatorMap.begin(); itRsiIndicator != m_spRsiIndicatorMap.end(); ++itRsiIndicator)
	{
		itRsiIndicator->second->load();
	}
	for (auto itSarIndicator = m_spSarIndicatorMap.begin(); itSarIndicator != m_spSarIndicatorMap.end(); ++itSarIndicator)
	{
		itSarIndicator->second->load();
	}
	for (auto itBollIndicator = m_spBollIndicatorMap.begin(); itBollIndicator != m_spBollIndicatorMap.end(); ++itBollIndicator)
	{
		itBollIndicator->second->load();
	}
}

bool sortFun(const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock1,
	const std::pair<std::string, std::pair<BigNumber, BigNumber>>& stock2)
{
	return stock1.second.second > stock2.second.second;
}

bool StockTrade::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>>* allBuyInfo,
	SolutionEnum solutionEnum,
	StrategyEnum strategyEnum)
{
	buyStock.clear();

	auto itSolution = m_solutionMap.find(solutionEnum);
	if (itSolution == m_solutionMap.end())
	{
		return false;
	}
	auto itStrategy = m_strategyMap.find(strategyEnum);
	if (itStrategy == m_strategyMap.end())
	{
		return false;
	}
	const std::shared_ptr<Solution>& spSolution = itSolution->second;
	const std::shared_ptr<Strategy>& spStrategy = itStrategy->second;
	spSolution->init(spStrategy);
	std::shared_ptr<SolutionAllInfo> spSolutionAllInfo;
	switch (solutionEnum)
	{
	case AVG_FUND_HIGH_SCORE:
	{
		spSolutionAllInfo.reset(new AvgFundHighScoreAllInfo);
	}
	break;
	default:
		break;
	}
	spSolutionAllInfo->m_filterStock = &(m_filterStock.find(date)->second);

	int32_t index = -1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[index];
		std::shared_ptr<StrategyInfo> spStrategyInfo;
		switch (strategyEnum)
		{
		case SAR_RISE_BACK:
		{
			SarRiseBackInfo* sarRiseBackInfo = new SarRiseBackInfo;
			sarRiseBackInfo->m_allBuyInfo = allBuyInfo;
			sarRiseBackInfo->m_spMarket = m_spMarketMap.find(stock)->second;
			sarRiseBackInfo->m_spSarIndicator = m_spSarIndicatorMap.find(stock)->second;
			sarRiseBackInfo->m_spBollIndicator = m_spBollIndicatorMap.find(stock)->second;
			spStrategyInfo.reset(sarRiseBackInfo);
		}
		break;
		default:
			break;
		}
		spSolutionAllInfo->m_strategyAllInfo[stock] = spStrategyInfo;
	}
	
	return spSolution->buy(buyStock, date, spSolutionAllInfo);
}

bool StockTrade::sell(const std::string& stock,
	const IntDateTime& date,
	BigNumber& price,
	BigNumber& rate,
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>>* allBuyInfo,
	SolutionEnum solutionEnum,
	StrategyEnum strategyEnum)
{
	auto itSolution = m_solutionMap.find(solutionEnum);
	if (itSolution == m_solutionMap.end())
	{
		return false;
	}
	auto itStrategy = m_strategyMap.find(strategyEnum);
	if (itStrategy == m_strategyMap.end())
	{
		return false;
	}
	const std::shared_ptr<Solution>& spSolution = itSolution->second;
	const std::shared_ptr<Strategy>& spStrategy = itStrategy->second;
	spSolution->init(spStrategy);

	std::shared_ptr<SolutionInfo> spSolutionInfo;
	switch (solutionEnum)
	{
	case AVG_FUND_HIGH_SCORE:
	{
		spSolutionInfo.reset(new AvgFundHighScoreInfo);
	}
	break;
	default:
		break;
	}

	std::shared_ptr<StrategyInfo> spStrategyInfo;
	switch (strategyEnum)
	{
	case SAR_RISE_BACK:
	{
		SarRiseBackInfo* sarRiseBackInfo = new SarRiseBackInfo;
		sarRiseBackInfo->m_allBuyInfo = allBuyInfo;
		sarRiseBackInfo->m_spMarket = m_spMarketMap.find(stock)->second;
		sarRiseBackInfo->m_spSarIndicator = m_spSarIndicatorMap.find(stock)->second;
		sarRiseBackInfo->m_spBollIndicator = m_spBollIndicatorMap.find(stock)->second;
		spStrategyInfo.reset(sarRiseBackInfo);
	}
	break;
	default:
		break;
	}

	spSolutionInfo->m_strategyInfo = spStrategyInfo;

	return spSolution->sell(date, price, rate, spSolutionInfo);
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