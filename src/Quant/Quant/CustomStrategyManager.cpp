#include "CustomStrategyManager.h"
#include "Cini/CiniAPI.h"
#include "Config.h"
#include "CStringManager/CStringManagerAPI.h"
#include "RedisManager.h"
#include <algorithm>
#include <mutex>
#include <set>

CustomStrategyManager::CustomStrategyManager()
{

}

CustomStrategyManager& CustomStrategyManager::instance()
{
	static CustomStrategyManager s_customStrategyManager;
	return s_customStrategyManager;
}

std::vector<std::string> CustomStrategyManager::getStrategyStocks(const std::string& strategyName)
{
	return getStrategyStocks(strategyName, 0, 99999999);
}

std::vector<std::string> CustomStrategyManager::getStrategyStocks(const std::string& strategyName, int32_t beginDate,
	int32_t endDate)
{
	StrategyStockMap strategyStockMap = getStrategyStockMap(strategyName, beginDate, endDate);
	std::set<std::string> stockSet;
	for (auto itDate = strategyStockMap.begin(); itDate != strategyStockMap.end(); ++itDate)
	{
		const std::vector<std::string>& vecStock = itDate->second;
		for (uint32_t stockIndex = 0; stockIndex < vecStock.size(); ++stockIndex)
		{
			if (!vecStock[stockIndex].empty())
			{
				stockSet.insert(vecStock[stockIndex]);
			}
		}
	}
	return std::vector<std::string>(stockSet.begin(), stockSet.end());
}

CustomStrategyManager::StrategyStockMap CustomStrategyManager::getStrategyStockMap(const std::string& strategyName,
	int32_t beginDate, int32_t endDate)
{
	StrategyStockMap strategyStockMap;
	if (strategyName.empty())
	{
		return strategyStockMap;
	}

	std::string beginDateString = std::to_string(beginDate);
	std::string endDateString = std::to_string(endDate);
	HiRedis& redis = RedisManager::instance().m_redis;
	std::shared_ptr<HiRedisResultSet> spKeySet = redis.command("keys " + strategyDateKeyPattern(strategyName));
	std::vector<std::string> vecKey = (spKeySet == nullptr) ? std::vector<std::string>() : spKeySet->toKeys();
	std::sort(vecKey.begin(), vecKey.end());
	for (uint32_t keyIndex = 0; keyIndex < vecKey.size(); ++keyIndex)
	{
		const std::string& key = vecKey[keyIndex];
		std::string dateString = dateFromStrategyDateKey(strategyName, key);
		if (dateString.empty() || dateString < beginDateString || dateString > endDateString)
		{
			continue;
		}

		std::shared_ptr<HiRedisResultSet> spStockSet = redis.getGroup(key);
		std::vector<std::string> vecStock = (spStockSet == nullptr) ?
			std::vector<std::string>() : spStockSet->toGroup();
		std::sort(vecStock.begin(), vecStock.end());
		vecStock.erase(std::unique(vecStock.begin(), vecStock.end()), vecStock.end());
		strategyStockMap[std::atoi(dateString.c_str())] = vecStock;
	}
	return strategyStockMap;
}

void CustomStrategyManager::saveStrategyStocksToRedis(const std::string& strategyName)
{
	if (strategyName.empty())
	{
		return;
	}
	saveStrategyStocksToRedis(strategyName, readStrategyStockFile(strategyName));
}

CustomStrategyManager::StrategyStockMap CustomStrategyManager::readStrategyStockFile(const std::string& strategyName)
{
	StrategyStockMap strategyStockMap;
	Cini ini(strategyConfigPath(strategyName), true);
	std::vector<std::string> vecSection = ini.getAllSection();
	for (uint32_t sectionIndex = 0; sectionIndex < vecSection.size(); ++sectionIndex)
	{
		const std::string& section = vecSection[sectionIndex];
		if (section.size() <= 4 || section.substr(0, 4) != "date")
		{
			continue;
		}
		int32_t date = std::atoi(section.substr(4).c_str());
		std::vector<std::string> vecStock = CStringManager::split(ini.readIni("stock", section), ",");
		for (uint32_t stockIndex = 0; stockIndex < vecStock.size(); ++stockIndex)
		{
			if (!vecStock[stockIndex].empty())
			{
				strategyStockMap[date].push_back(vecStock[stockIndex]);
			}
		}
	}
	return strategyStockMap;
}

void CustomStrategyManager::saveStrategyStocksToRedis(const std::string& strategyName, const StrategyStockMap& strategyStockMap)
{
	static std::mutex redisMutex;
	std::lock_guard<std::mutex> lock(redisMutex);
	HiRedis& redis = RedisManager::instance().m_redis;
	std::shared_ptr<HiRedisResultSet> spOldKeySet = redis.command("keys " + strategyDateKeyPattern(strategyName));
	std::vector<std::string> oldKeys = (spOldKeySet == nullptr) ? std::vector<std::string>() : spOldKeySet->toKeys();
	if (!oldKeys.empty())
	{
		redis.deleteKeys(oldKeys);
	}

	for (auto itDate = strategyStockMap.begin(); itDate != strategyStockMap.end(); ++itDate)
	{
		std::string date = std::to_string(itDate->first);
		std::set<std::string> dateStocks(itDate->second.begin(), itDate->second.end());
		for (auto itStock = dateStocks.begin(); itStock != dateStocks.end(); ++itStock)
		{
			if (!itStock->empty())
			{
				redis.setGroup(strategyDateKey(strategyName, date), *itStock);
			}
		}
	}
}

std::string CustomStrategyManager::strategyConfigPath(const std::string& strategyName) const
{
	return g_config.m_currentExePath + strategyName + ".ini";
}

std::string CustomStrategyManager::strategyDateKey(const std::string& strategyName, const std::string& date) const
{
	return strategyName + "_" + date;
}

std::string CustomStrategyManager::strategyDateKeyPattern(const std::string& strategyName) const
{
	return strategyName + "_*";
}

std::string CustomStrategyManager::dateFromStrategyDateKey(const std::string& strategyName, const std::string& key) const
{
	std::string prefix = strategyName + "_";
	if (key.size() <= prefix.size() || key.substr(0, prefix.size()) != prefix)
	{
		return "";
	}
	std::vector<std::string> vecKey = CStringManager::split(key, "_");
	if (vecKey.empty())
	{
		return "";
	}
	return vecKey.back();
}
