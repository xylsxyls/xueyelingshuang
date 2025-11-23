#include "S1100B1400Strategy.h"
#include <algorithm>
#include <iostream>
#include "Util.h"

S1100B1400Strategy::S1100B1400Strategy():
m_hasFirstBuy(false)
{
	m_mode = StrategyMode::S1100B1400;
	m_modeName = "S1100B1400Strategy";
}

S1100B1400Strategy::~S1100B1400Strategy()
{

}

bool S1100B1400Strategy::onTradingDay(uint32_t date)
{
	if (!isStrategyParamValid())
	{
		RCSend("isStrategyParamValid");
		return false;
	}

	// 只处理第一只股票（可根据需要扩展为多股票）
	const std::string& stock = m_vecStock[0];
	
	// 解析策略参数
	int32_t sellTimeIndex = m_strategyParam[0] + (int32_t)Overall::COUNT; // 0,1,2 对应 10:40,10:50,11:00
	int32_t forceBuyTimeIndex = m_strategyParam[1] + (int32_t)Overall::COUNT;  // 0,1,2 对应 13:40,13:50,14:00
	int32_t chaseParam = m_strategyParam[2];    // 7,8,9 分
	int32_t discountParam = m_strategyParam[3]; // 1,2,3 分

	const std::vector<int32_t>& dayInfo = m_spMarket->getStockData(stock, date);
	if (dayInfo.empty())
	{
		RCSend("dayInfo empty");
		return false;
	}

	if (!m_hasFirstBuy)
	{
		m_spFund->buyAll(stock, dayInfo[(int32_t)Overall::CLOSE], date, ObserveTime::COUNT);
		m_hasFirstBuy = true;
		return true;
	}

	int32_t sellPrice = dayInfo[sellTimeIndex];
	int32_t forceBuyPrice = dayInfo[forceBuyTimeIndex];
	int32_t buyPrice = sellPrice - discountParam;
	int32_t chaseBuyPrice = sellPrice + chaseParam;
	ObserveTime sellTime = Util::indexToTime(sellTimeIndex);
	ObserveTime forceBuyTime = Util::indexToTime(forceBuyTimeIndex);

	// 检查当前是否持有该股票
	bool hasPosition = (m_spFund->getPosition(stock) != nullptr);
	if (!hasPosition)
	{
		RCSend("hasPosition false");
		return false;
	}

	m_spFund->sellAllForT(stock, sellPrice, date, sellTime);

	bool isBuy = false;
	for (int32_t timeIndex = (int32_t)sellTime; timeIndex <= (int32_t)forceBuyTime; ++timeIndex)
	{
		int32_t bestSellIndex = Util::bestPrice((ObserveTime)timeIndex, RangeTime::RANGENEXT, TransType::BEST_SELL);
		int32_t bestSellPrice = dayInfo[bestSellIndex];
		if (bestSellPrice >= chaseBuyPrice)
		{
			m_spFund->buyAll(stock, chaseBuyPrice, date, (ObserveTime)timeIndex);
			isBuy = true;
			break;
		}
		int32_t bestBuyIndex = Util::bestPrice((ObserveTime)timeIndex, RangeTime::RANGENEXT, TransType::BEST_BUY);
		int32_t bestBuyPrice = dayInfo[bestBuyIndex];
		if (bestBuyPrice <= buyPrice)
		{
			m_spFund->buyAll(stock, buyPrice, date, (ObserveTime)timeIndex);
			isBuy = true;
			break;
		}
	}

	if (!isBuy)
	{
		m_spFund->buyAll(stock, forceBuyPrice, date, (ObserveTime)forceBuyTime);
	}

	return true;
}

bool S1100B1400Strategy::isStrategyParamValid() const
{
	// 检查参数有效性
	if (m_strategyParam.size() != 4)
	{
		RCSend("Error: Strategy:%s parameters size is not 4", m_modeName.c_str());
		return false;
	}
	// 检查市场数据和资金账户
	if (!m_spMarket || !m_spFund)
	{
		RCSend("Error: Strategy:%s Market or Fund is not set", m_modeName.c_str());
		return false;
	}
	// 检查股票列表
	if (m_vecStock.empty())
	{
		RCSend("Error: Strategy:%s No stocks in strategy", m_modeName.c_str());
		return false;
	}
	// 只处理第一只股票（可根据需要扩展为多股票）
	const std::string& stock = m_vecStock[0];
	// 检查股票是否存在市场数据中
	if (!m_spMarket->hasStock(stock))
	{
		RCSend("Error: Strategy:%s Stock %s not found in market data", m_modeName.c_str(), stock.c_str());
		return false;
	}
	return true;
}

int32_t S1100B1400Strategy::getBestSellPrice(const std::string& stock, uint32_t date, ObserveTime time) const
{
	if (!m_spMarket)
	{
		return 0;
	}

	const auto& dayData = m_spMarket->getStockData(stock, date);
	if (dayData.empty())
	{
		return 0;
	}

	// 计算索引：(Overall) + (ObserveTime) + 三维坐标
	size_t index = Util::getPriceMatrixIndex(time, RangeTime::RANGE0, TransType::BEST_SELL);
	if (index < dayData.size())
	{
		return dayData[index];
	}

	return 0;
}

int32_t S1100B1400Strategy::getBestBuyPrice(const std::string& stock, uint32_t date, ObserveTime time) const
{
	if (!m_spMarket)
	{
		return 0;
	}

	const auto& dayData = m_spMarket->getStockData(stock, date);
	if (dayData.empty())
	{
		return 0;
	}

	size_t index = Util::getPriceMatrixIndex(time, RangeTime::RANGE0, TransType::BEST_BUY);
	if (index < dayData.size())
	{
		return dayData[index];
	}

	return 0;
}

int32_t S1100B1400Strategy::getRangeNextBestBuyPrice(const std::string& stock, uint32_t date,
	ObserveTime startTime, ObserveTime endTime) const
{
	if (!m_spMarket)
	{
		return 0;
	}

	const auto& dayData = m_spMarket->getStockData(stock, date);
	if (dayData.empty())
	{
		return 0;
	}

	int32_t bestPrice = 0;
	ObserveTime currentTime = startTime;

	while (currentTime < endTime)
	{
		ObserveTime nextTime = getNextTime(currentTime);
		if (nextTime == ObserveTime::COUNT)
		{
			break;
		}

		// 获取RANGENEXT的最佳买价
		size_t index = Util::getPriceMatrixIndex(currentTime, RangeTime::RANGENEXT, TransType::BEST_BUY);
		if (index < dayData.size() && dayData[index] > 0)
		{
			if (bestPrice == 0 || dayData[index] < bestPrice)
			{
				bestPrice = dayData[index];
			}
		}

		currentTime = nextTime;
	}

	return bestPrice;
}

int32_t S1100B1400Strategy::getRangeNextBestSellPrice(const std::string& stock, uint32_t date,
	ObserveTime startTime, ObserveTime endTime) const
{
	if (!m_spMarket)
	{
		return 0;
	}

	const auto& dayData = m_spMarket->getStockData(stock, date);
	if (dayData.empty())
	{
		return 0;
	}

	int32_t bestPrice = 0;
	ObserveTime currentTime = startTime;

	while (currentTime < endTime)
	{
		ObserveTime nextTime = getNextTime(currentTime);
		if (nextTime == ObserveTime::COUNT)
		{
			break;
		}

		// 获取RANGENEXT的最佳卖价
		size_t index = Util::getPriceMatrixIndex(currentTime, RangeTime::RANGENEXT, TransType::BEST_SELL);
		if (index < dayData.size() && dayData[index] > 0)
		{
			if (dayData[index] > bestPrice)
			{
				bestPrice = dayData[index];
			}
		}

		currentTime = nextTime;
	}

	return bestPrice;
}

bool S1100B1400Strategy::shouldChaseBuy(const std::string& stock, uint32_t date, int32_t sellPrice,
	int32_t chaseParam, ObserveTime startTime, ObserveTime endTime) const
{
	int32_t bestSellPrice = getRangeNextBestSellPrice(stock, date, startTime, endTime);
	return (bestSellPrice >= sellPrice + chaseParam);
}

bool S1100B1400Strategy::shouldConditionBuy(const std::string& stock, uint32_t date, int32_t sellPrice,
	int32_t discountParam, ObserveTime startTime, ObserveTime endTime) const
{
	int32_t bestBuyPrice = getRangeNextBestBuyPrice(stock, date, startTime, endTime);
	return (bestBuyPrice <= sellPrice - discountParam && bestBuyPrice > 0);
}

ObserveTime S1100B1400Strategy::getNextTime(ObserveTime currentTime) const
{
	switch (currentTime)
	{
	case ObserveTime::TIME0930: return ObserveTime::TIME1040;
	case ObserveTime::TIME1040: return ObserveTime::TIME1050;
	case ObserveTime::TIME1050: return ObserveTime::TIME1100;
	case ObserveTime::TIME1100: return ObserveTime::TIME1110;
	case ObserveTime::TIME1110: return ObserveTime::TIME1340;
	case ObserveTime::TIME1340: return ObserveTime::TIME1350;
	case ObserveTime::TIME1350: return ObserveTime::TIME1400;
	case ObserveTime::TIME1400: return ObserveTime::TIME1410;
	case ObserveTime::TIME1410: return ObserveTime::COUNT;
	default: return ObserveTime::COUNT;
	}
}

bool S1100B1400Strategy::executeSell(const std::string& stock, uint32_t date, ObserveTime sellTime)
{
	int32_t price = getBestSellPrice(stock, date, sellTime);
	if (price > 0)
	{
		return m_spFund->sellAll(stock, price, date, sellTime);
	}
	return false;
}

bool S1100B1400Strategy::executeBuy(const std::string& stock, uint32_t date, ObserveTime buyTime, int32_t price)
{
	if (price > 0)
	{
		return m_spFund->buyAll(stock, price, date, buyTime);
	}
	return false;
}

size_t S1100B1400Strategy::getObserveTimeOffset(ObserveTime time) const
{
	return static_cast<size_t>(time);
}

size_t S1100B1400Strategy::getRangeTimeOffset(RangeTime range) const
{
	switch (range)
	{
	case RangeTime::RANGE0: return 0;
	case RangeTime::RANGE10: return 1;
	case RangeTime::RANGE20: return 2;
	case RangeTime::RANGE30: return 3;
	case RangeTime::RANGENEXT: return 4;
	default: return 0;
	}
}

size_t S1100B1400Strategy::getTransTypeOffset(TransType transType) const
{
	return static_cast<size_t>(transType);
}