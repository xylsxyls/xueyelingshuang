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
	while (index++ != vecSolutionType.size() - 1)
	{
		std::shared_ptr<Solution> spSolution = StockSolution::instance().solution(vecSolutionType[index]);
		switch (vecSolutionType[index])
		{
		case AVG_FUND_HIGH_SCORE:
		{
			std::dynamic_pointer_cast<AvgFundHighScore>(spSolution)->init(5, 5);
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
	StockFund* stockFund,
	SolutionType solutionType,
	const std::vector<StrategyType>& vecStrategyType)
{
	buyStock.clear();

	auto itSolution = m_solutionMap.find(solutionType);
	if (itSolution == m_solutionMap.end())
	{
		return false;
	}
	const std::shared_ptr<Solution>& spSolution = itSolution->second;

	std::vector<std::shared_ptr<Strategy>> vecStrategy;
	int32_t index = -1;
	while (index++ != vecStrategyType.size() - 1)
	{
		auto itStrategy = m_strategyMap.find(vecStrategyType[index]);
		if (itStrategy == m_strategyMap.end())
		{
			return false;
		}
		vecStrategy.push_back(itStrategy->second);
	}
	spSolution->init(vecStrategy);
	RCSend("size = %d", vecStrategyType.size());
	return spSolution->buy(buyStock, date, makeSolutionAllInfo(date, stockFund, solutionType, vecStrategyType));
}

bool StockTrade::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
	const IntDateTime& date,
	StockFund* stockFund,
	SolutionType solutionType,
	const std::vector<StrategyType>& vecStrategyType)
{
	auto itSolution = m_solutionMap.find(solutionType);
	if (itSolution == m_solutionMap.end())
	{
		return false;
	}
	const std::shared_ptr<Solution>& spSolution = itSolution->second;

	std::vector<std::shared_ptr<Strategy>> vecStrategy;
	int32_t index = -1;
	while (index++ != vecStrategyType.size() - 1)
	{
		auto itStrategy = m_strategyMap.find(vecStrategyType[index]);
		if (itStrategy == m_strategyMap.end())
		{
			return false;
		}
		vecStrategy.push_back(itStrategy->second);
	}
	spSolution->init(vecStrategy);

	std::shared_ptr<SolutionAllInfo> solutionAllInfo = makeSolutionAllInfo(date,
		stockFund,
		solutionType,
		vecStrategyType);
	return spSolution->sell(sellStock, date, solutionAllInfo);
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

std::shared_ptr<SolutionAllInfo> StockTrade::makeSolutionAllInfo(const IntDateTime& date,
	StockFund* stockFund,
	SolutionType solutionType,
	const std::vector<StrategyType>& vecStrategyType)
{
	std::shared_ptr<SolutionAllInfo> spSolutionAllInfo;
	switch (solutionType)
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
	spSolutionAllInfo->m_fund = stockFund;

	int32_t index = -1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[index];
		std::vector<std::shared_ptr<StrategyInfo>>& vecStrategyInfoInAllInfo = spSolutionAllInfo->m_strategyAllInfo[stock];
		int32_t strategyIndex = -1;
		while (strategyIndex++ != vecStrategyType.size() - 1)
		{
			std::shared_ptr<StrategyInfo> spStrategyInfo;
			switch (vecStrategyType[strategyIndex])
			{
			case SAR_RISE_BACK:
			{
				SarRiseBackInfo* sarRiseBackInfo = new SarRiseBackInfo;
				sarRiseBackInfo->m_fund = stockFund;
				sarRiseBackInfo->m_spMarket = m_spMarketMap.find(stock)->second;
				sarRiseBackInfo->m_spSarIndicator = m_spSarIndicatorMap.find(stock)->second;
				sarRiseBackInfo->m_spBollIndicator = m_spBollIndicatorMap.find(stock)->second;
				spStrategyInfo.reset(sarRiseBackInfo);
			}
			break;
			default:
				break;
			}
			vecStrategyInfoInAllInfo.push_back(spStrategyInfo);
		}
	}
	return spSolutionAllInfo;
}