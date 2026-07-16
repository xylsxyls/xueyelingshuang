#include "CollectTask.h"
#include "CustomStrategyManager.h"
#include "RedisManager.h"
#ifndef _WIN64
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#endif
#include "Point/PointAPI.h"
#include <set>

CollectTask::CollectTask():
m_count(0),
m_mode(CollectMode::CURRENT_STOCK),
m_exit(false)
{

}

void CollectTask::DoTask()
{
	if (m_mode == CollectMode::CURRENT_STOCK)
	{
		collectStock("", m_count);
		return;
	}

	if (m_strategyName.empty())
	{
		RCSend("collect strategy name empty, mode = %d", (int32_t)m_mode);
		return;
	}

	CustomStrategyManager::StrategyStockMap strategyStockMap =
		CustomStrategyManager::instance().getStrategyStockMap(m_strategyName, 0, 99999999);
	if (strategyStockMap.empty())
	{
		RCSend("custom strategy stock empty, strategy = %s", m_strategyName.c_str());
		return;
	}

	if (m_mode == CollectMode::STRATEGY_ALL)
	{
		collectStocks(strategyStockCollectCount(strategyStockMap, m_count));
		return;
	}

	if (m_mode == CollectMode::STRATEGY_MISSING)
	{
		collectStocks(missingStockCollectCount(strategyStockMap, m_count));
		return;
	}

	RCSend("collect mode error, mode = %d", (int32_t)m_mode);
}

void CollectTask::collectStock(const std::string& stock, int32_t count)
{
	std::vector<xyls::Point> vec = { { 1046, 742 }, { 1076, 990 }, { 1078, 649 }, { 1072, 601 } };
	if (count <= 0)
	{
		RCSend("collect stock skip, stock = %s, count = %d", stock.c_str(), count);
		return;
	}

	RCSend("collect stock = %s, count = %d", stock.c_str(), count);
	while (!m_exit && (count-- > 0))
	{
#ifndef _WIN64
		CKeyboard::KeyPress(CKeyboard::PageUp, 0);
#endif
		Sleep(800);
		size_t index = -1;
		while (!m_exit && (index++ != vec.size() - 1))
		{
#ifndef _WIN64
			CMouse::MoveClick(vec[index], 0);
#endif
			Sleep(800);
		}
	}
}

void CollectTask::collectStocks(const std::map<std::string, int32_t>& stockCollectCount)
{
	if (stockCollectCount.empty())
	{
		RCSend("collect stock empty, strategy = %s", m_strategyName.c_str());
		return;
	}

	std::string currentStock;
	for (auto itStock = stockCollectCount.begin(); itStock != stockCollectCount.end(); ++itStock)
	{
		if (m_exit)
		{
			return;
		}
		if (itStock->first.empty() || itStock->second <= 0)
		{
			continue;
		}
		switchStock(currentStock, itStock->first);
		if (m_exit)
		{
			return;
		}
		collectStock(itStock->first, itStock->second);
		currentStock = itStock->first;
	}
}

std::map<std::string, int32_t> CollectTask::strategyStockCollectCount(
	const std::map<int32_t, std::vector<std::string>>& strategyStockMap, int32_t count)
{
	std::map<std::string, int32_t> stockCollectCount;
	if (count <= 0)
	{
		return stockCollectCount;
	}

	for (auto itDate = strategyStockMap.begin(); itDate != strategyStockMap.end(); ++itDate)
	{
		const std::vector<std::string>& vecStock = itDate->second;
		for (uint32_t stockIndex = 0; stockIndex < vecStock.size(); ++stockIndex)
		{
			if (!vecStock[stockIndex].empty())
			{
				stockCollectCount[vecStock[stockIndex]] = count;
			}
		}
	}
	return stockCollectCount;
}

std::map<std::string, int32_t> CollectTask::missingStockCollectCount(
	const std::map<int32_t, std::vector<std::string>>& strategyStockMap, int32_t count)
{
	std::map<std::string, std::set<int32_t>> stockDateMap;
	for (auto itDate = strategyStockMap.begin(); itDate != strategyStockMap.end(); ++itDate)
	{
		const std::vector<std::string>& vecStock = itDate->second;
		for (uint32_t stockIndex = 0; stockIndex < vecStock.size(); ++stockIndex)
		{
			if (!vecStock[stockIndex].empty())
			{
				stockDateMap[vecStock[stockIndex]].insert(itDate->first);
			}
		}
	}

	std::map<std::string, int32_t> stockCollectCount;
	if (count <= 0)
	{
		return stockCollectCount;
	}

	for (auto itStock = stockDateMap.begin(); itStock != stockDateMap.end(); ++itStock)
	{
		int32_t missingCount = 0;
		int32_t checkCount = 0;
		const std::set<int32_t>& dateSet = itStock->second;
		for (auto itDate = dateSet.rbegin(); itDate != dateSet.rend() && checkCount < count; ++itDate)
		{
			++checkCount;
			if (hasStockDateData(itStock->first, *itDate))
			{
				break;
			}
			++missingCount;
		}
		RCSend("collect stock check, stock = %s, all = %d, check = %d, missing = %d",
			itStock->first.c_str(), (int32_t)dateSet.size(), checkCount, missingCount);
		if (missingCount > 0)
		{
			stockCollectCount[itStock->first] = missingCount;
		}
	}
	return stockCollectCount;
}

bool CollectTask::hasStockDateData(const std::string& stock, int32_t date)
{
	if (stock.empty() || date <= 0)
	{
		return false;
	}
	std::string groupName = stock + ":overall_0";
	return RedisManager::instance().m_redis.getOrderGroupCountByScore(groupName, date, date) > 0;
}

void CollectTask::switchStock(const std::string& fromStock, const std::string& toStock)
{
	// TODO: switch collect context from fromStock to toStock.
#ifndef _WIN64
	CMouse::MoveAbsolute(xyls::Point(1280, 296));
	CMouse::LeftClick(0);
	Sleep(100);
	for (size_t index = 0; index < toStock.size(); ++index)
	{
		CKeyboard::KeyPress(toStock[index], 10);
	}
	CKeyboard::KeyPress(CKeyboard::ENTER);
	Sleep(100);
	CKeyboard::KeyPress(CKeyboard::End);
	CKeyboard::KeyPress(CKeyboard::ENTER);
#else
	fromStock;
	toStock;
#endif
}

void CollectTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void CollectTask::setMode(CollectMode mode)
{
	m_mode = mode;
}

void CollectTask::setParam(int32_t count, const std::string& strategyName)
{
	m_count = count;
	m_strategyName = strategyName;
}

void CollectTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}
