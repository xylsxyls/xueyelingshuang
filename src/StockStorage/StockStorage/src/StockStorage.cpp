#include "StockStorage.h"

StockStorage::StockStorage():
m_moveDay(0)
{

}

StockStorage& StockStorage::instance()
{
	static StockStorage s_stockStorage;
	return s_stockStorage;
}

void StockStorage::init(const std::vector<std::string>& allStock,
	int32_t moveDay,
	const IntDateTime& beginTime,
	const IntDateTime& endTime)
{
	m_allStock = allStock;
	m_moveDay = moveDay;
	m_beginTime = beginTime;
	m_endTime = endTime;
}

void StockStorage::loadMarket()
{
	m_spMarketMap.clear();
	int32_t index = -1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[index];
		std::shared_ptr<StockMarket> spMarket(new StockMarket);
		spMarket->loadFromRedis(stock, m_beginTime - m_moveDay * 86400, m_endTime);
		m_spMarketMap[stock] = spMarket;
	}
}

void StockStorage::loadIndicator(const std::set<std::string>& allNeedLoad)
{
	m_spIndicatorMap.clear();
	int32_t index = -1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[index];
		std::map<std::string, std::shared_ptr<IndicatorManagerBase>>& stockIndicatorMap = m_spIndicatorMap[stock];

		StockIndicator::instance().loadIndicatorFromRedis(stock, m_beginTime - m_moveDay * 86400, m_endTime);
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
}

void StockStorage::loadFilterStock()
{
	m_filterStock.clear();
	IntDateTime currentTime = m_beginTime;
	while (true)
	{
		StockStrategy::instance().strategyStock(currentTime, m_filterStock[currentTime]);
		currentTime = currentTime + 86400;
		if (currentTime > m_endTime)
		{
			break;
		}
	}
}

void StockStorage::loadSolution(const std::set<SolutionType>& allSolutionType, const std::vector<ChooseParam>& vecChooseParam)
{
	for (auto itSolutionType = allSolutionType.begin(); itSolutionType != allSolutionType.end(); ++itSolutionType)
	{
		const SolutionType& solutionType = *itSolutionType;
		std::shared_ptr<Solution> spSolution = StockSolution::instance().solution(solutionType);
		switch (solutionType)
		{
		case INTEGRATED_STRATEGY:
		{
			std::shared_ptr<IntegratedStrategy> spIntegratedStrategy = std::dynamic_pointer_cast<IntegratedStrategy>(spSolution);
			spIntegratedStrategy->init(vecChooseParam, &m_solutionMap);
		}
		break;
		case OBSERVE_STRATEGY:
		{
			std::shared_ptr<ObserveStrategy> spObserveStrategy = std::dynamic_pointer_cast<ObserveStrategy>(spSolution);
			spObserveStrategy->init(4, 2);
		}
		break;
		default:
			break;
		}
		spSolution->setSolutionInfo(m_solutionInfoMap.find(solutionType)->second);
		spSolution->setChooseParam(vecChooseParam[0]);
		m_solutionMap[solutionType] = spSolution;
	}
}

std::vector<std::string>* StockStorage::filterStock(const IntDateTime& date)
{
	auto itFilterStock = m_filterStock.find(date);
	if (itFilterStock == m_filterStock.end())
	{
		return nullptr;
	}
	return &(itFilterStock->second);
}

void StockStorage::loadStrategy(const std::set<StrategyType>& allStrategyType)
{
	m_strategyMap.clear();
	for (auto itStrategyType = allStrategyType.begin(); itStrategyType != allStrategyType.end(); ++itStrategyType)
	{
		const StrategyType& strategyType = *itStrategyType;
		m_strategyMap[strategyType] = StockStrategy::instance().strategy(strategyType);
	}
}

void StockStorage::loadSolutionInfo(const std::set<SolutionType>& allSolutionType, const std::set<StrategyType>& allStrategyType)
{
	for (auto itSolutionType = allSolutionType.begin(); itSolutionType != allSolutionType.end(); ++itSolutionType)
	{
		const SolutionType& solutionType = *itSolutionType;
		std::shared_ptr<SolutionInfo> spSolutionInfo = StockSolution::instance().solutionInfo(solutionType);

		int32_t index = -1;
		while (index++ != m_allStock.size() - 1)
		{
			const std::string& stock = m_allStock[index];
			for (auto itStrategyType = allStrategyType.begin(); itStrategyType != allStrategyType.end(); ++itStrategyType)
			{
				const StrategyType& strategyType = *itStrategyType;
				spSolutionInfo->m_allStrategy[strategyType] = m_strategyMap.find(strategyType)->second;
				spSolutionInfo->m_allStrategyInfo[stock][strategyType] = m_strategyInfoMap.find(stock)->second.find(strategyType)->second;
			}
		}
		m_solutionInfoMap[solutionType] = spSolutionInfo;
	}
}

void StockStorage::loadStrategyInfo(const std::set<StrategyType>& allStrategyType)
{
	m_strategyInfoMap.clear();
	int32_t stockIndex = -1;
	while (stockIndex++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[stockIndex];
		for (auto itStrategyType = allStrategyType.begin(); itStrategyType != allStrategyType.end(); ++itStrategyType)
		{
			const StrategyType& strategyType = *itStrategyType;
			m_strategyInfoMap[stock][strategyType] = StockStrategy::instance().strategyInfo(strategyType,
				nullptr,
				m_spMarketMap.find(stock)->second,
				m_spIndicatorMap.find(stock)->second);
		}
	}
}

std::shared_ptr<Solution> StockStorage::solution(SolutionType solutionType)
{
	auto itSolution = m_solutionMap.find(solutionType);
	if (itSolution == m_solutionMap.end())
	{
		return nullptr;
	}
	return itSolution->second;
}

std::shared_ptr<Strategy> StockStorage::strategy(StrategyType strategyType)
{
	auto itStrategy = m_strategyMap.find(strategyType);
	if (itStrategy == m_strategyMap.end())
	{
		return nullptr;
	}
	return itStrategy->second;
}

std::shared_ptr<SolutionInfo> StockStorage::solutionInfo(SolutionType solutionType)
{
	auto itSolutionInfo = m_solutionInfoMap.find(solutionType);
	if (itSolutionInfo == m_solutionInfoMap.end())
	{
		return nullptr;
	}
	return itSolutionInfo->second;
}

std::shared_ptr<StrategyInfo> StockStorage::strategyInfo(StrategyType strategyType, const std::string& stock)
{
	auto itStrategyInfoMap = m_strategyInfoMap.find(stock);
	if (itStrategyInfoMap == m_strategyInfoMap.end())
	{
		return nullptr;
	}
	auto itStrategyInfo = itStrategyInfoMap->second.find(strategyType);
	if (itStrategyInfo == itStrategyInfoMap->second.end())
	{
		return nullptr;
	}
	return itStrategyInfo->second;
}

std::shared_ptr<StockMarket> StockStorage::market(const std::string& stock)
{
	auto itMarket = m_spMarketMap.find(stock);
	if (itMarket == m_spMarketMap.end())
	{
		return nullptr;
	}
	return itMarket->second;
}

void StockStorage::load()
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