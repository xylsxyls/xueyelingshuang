#include "StockStorage.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include "CStringManager/CStringManagerAPI.h"

StockStorage::StockStorage():
m_moveDay(0),
m_isCustomize(false)
{

}

StockStorage& StockStorage::instance()
{
	static StockStorage s_stockStorage;
	return s_stockStorage;
}

void StockStorage::init(const IntDateTime& beginTime,
	const IntDateTime& endTime,
	int32_t moveDay,
	const std::vector<std::string>& allStock)
{
	m_moveDay = moveDay;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_spRunMarket.reset(new StockMarket);
	m_spRunMarket->loadFromRedis("000001", m_beginTime - 2 * 365 * 86400, m_endTime);
	m_spRunMarket->load();
	m_spRunMarket->setNewDate(m_beginTime);
	m_moveBeginTime = m_spRunMarket->getDateBefore(m_moveDay);

	if (!allStock.empty())
	{
		m_allStock = allStock;
		m_isCustomize = true;
		return;
	}
	m_isCustomize = false;
	m_allStock.clear();
}

void StockStorage::loadMarket()
{
	m_spMarketMap.clear();
	int32_t index = -1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& stock = m_allStock[index];
		std::shared_ptr<StockMarket> spMarket(new StockMarket);
		spMarket->loadFromRedis(stock, m_moveBeginTime, m_endTime);
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

		StockIndicator::instance().loadIndicatorFromRedis(stock, m_moveBeginTime, m_endTime);
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

void StockStorage::loadFilterStock(StrategyType strategyType, const StockLoadInfo& stockLoadInfo)
{
	m_filterStock.clear();
	if (m_isCustomize)
	{
		return;
	}

	std::vector<std::string> allStock = StockMysql::instance().allStock();
	m_spRunMarket->setLastDate(m_moveBeginTime);
	std::map<IntDateTime, std::vector<std::string>>& filterMap = m_filterStock[strategyType];
	while (true)
	{
		IntDateTime date = m_spRunMarket->date();
		if (date > m_endTime)
		{
			break;
		}

		std::vector<std::string>& dayVec = filterMap[date];
		switch (stockLoadInfo.m_filterType)
		{
		case ALL_STOCK:
		{
			dayVec = allStock;
		}
		break;
		case RISE_UP:
		{
			StockMysql::instance().readRiseUpStockFromRedis(date, dayVec);
		}
		break;
		case FALL_DOWN:
		{
			dayVec = allStock;
			std::vector<std::string> vecRiseUp;
			StockMysql::instance().readRiseUpStockFromRedis(date, vecRiseUp);
			StockMysql::filterRemove(dayVec, vecRiseUp);
		}
		break;
		default:
			break;
		}
		
		if (stockLoadInfo.m_isDislodge688)
		{
			dislodge688(dayVec);
		}
		if (stockLoadInfo.m_isDislodgeLiftBan)
		{
			std::vector<std::string> liftBanStock;
			StockMysql::instance().readLiftBanStockFromRedis(date, liftBanStock);
			StockMysql::filterRemove(dayVec, liftBanStock);
		}

		if (!m_spRunMarket->next())
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
			spIntegratedStrategy->init(vecChooseParam, this);
		}
		break;
		case OBSERVE_STRATEGY:
		{
			std::shared_ptr<ObserveStrategy> spObserveStrategy = std::dynamic_pointer_cast<ObserveStrategy>(spSolution);
			spObserveStrategy->init(4, 2, this);
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

void StockStorage::clear()
{
	m_spMarketMap.clear();
	m_spIndicatorMap.clear();
	m_solutionMap.clear();
	m_solutionInfoMap.clear();
	m_strategyMap.clear();
	m_strategyInfoMap.clear();
	m_allStock.clear();
	m_moveDay = 0;
	m_beginTime.clear();
	m_endTime.clear();
	m_moveBeginTime.clear();
	m_filterStock.clear();
	m_spRunMarket = nullptr;
}

std::vector<std::string>* StockStorage::filterStock(StrategyType strategyType, const IntDateTime& date)
{
	if (m_isCustomize)
	{
		return &m_allStock;
	}
	auto itFilterMap = m_filterStock.find(strategyType);
	if (itFilterMap == m_filterStock.end())
	{
		return nullptr;
	}
	auto itFilterStock = itFilterMap->second.find(date);
	if (itFilterStock == itFilterMap->second.end())
	{
		return nullptr;
	}
	return &(itFilterStock->second);
}

void StockStorage::loadStrategy(const std::set<StrategyType>& allStrategyType)
{
	m_strategyMap.clear();
	m_filterStock.clear();
	for (auto itStrategyType = allStrategyType.begin(); itStrategyType != allStrategyType.end(); ++itStrategyType)
	{
		const StrategyType& strategyType = *itStrategyType;
		m_strategyMap[strategyType] = StockStrategy::instance().strategy(strategyType);
		loadFilterStock(strategyType, StockStrategy::instance().strategyStockLoadInfo(strategyType));
	}
	
	std::vector<std::vector<std::string>> allStock;
	allStock.push_back(std::vector<std::string>());
	for (auto itFilterMap = m_filterStock.begin(); itFilterMap != m_filterStock.end(); ++itFilterMap)
	{
		const std::map<IntDateTime, std::vector<std::string>>& filterMap = itFilterMap->second;
		for (auto itDayFilter = filterMap.begin(); itDayFilter != filterMap.end(); ++itDayFilter)
		{
			const std::vector<std::string>& filterStock = itDayFilter->second;
			int32_t index = -1;
			while (index++ != filterStock.size() - 1)
			{
				allStock.back().push_back(filterStock[index]);
			}
		}
	}
	StockMysql::filterMerge(m_allStock, allStock);
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

std::shared_ptr<StockMarket> StockStorage::runMarket()
{
	return m_spRunMarket;
}

IntDateTime StockStorage::moveDay(const IntDateTime& date, int32_t day, const std::shared_ptr<StockMarket>& runMarket)
{
	if (day < 0)
	{
		return IntDateTime(0, 0);
	}
	if (day == 0)
	{
		return date;
	}
	std::shared_ptr<StockMarket> spRunMarket = (runMarket != nullptr ? runMarket : m_spRunMarket);
	if (!spRunMarket->setDate(date))
	{
		if (!spRunMarket->setLastDate(date))
		{
			return IntDateTime(0, 0);
		}
		return spRunMarket->getDateBefore(day - 1);
	}
	return spRunMarket->getDateBefore(day);
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

void StockStorage::dislodge688(std::vector<std::string>& allStock)
{
	for (auto itStock = allStock.begin(); itStock != allStock.end();)
	{
		const std::string& stock = *itStock;
		if (CStringManager::Left(stock, 3) == "688")
		{
			itStock = allStock.erase(itStock);
			continue;
		}
		++itStock;
	}
}