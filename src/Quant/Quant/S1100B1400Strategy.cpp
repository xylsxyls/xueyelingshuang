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
	ObserveTime sellObserveTime = (ObserveTime)m_strategyParam[0]; // 卖出时间点 对应 10:40,10:50,11:00
	ObserveTime forceBuyObserveTime = (ObserveTime)m_strategyParam[1]; // 买入时间点 对应 13:40,13:50,14:00
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

	int32_t sellPrice = getDirectSellPrice(dayInfo, sellObserveTime);
	int32_t forceBuyPrice = getDirectBuyPrice(dayInfo, forceBuyObserveTime);
	int32_t tBuyPrice = sellPrice - discountParam;
	int32_t chaseBuyPrice = sellPrice + chaseParam;

	// 检查当前是否持有该股票
	bool hasPosition = (m_spFund->getPosition(stock) != nullptr);
	if (!hasPosition)
	{
		RCSend("hasPosition false");
		return false;
	}

	m_spFund->sellAllForT(stock, sellPrice, date, sellObserveTime);

	bool isBuy = false;
	for (int32_t endTimeIndex = (int32_t)sellObserveTime + 1;
		endTimeIndex <= (int32_t)forceBuyObserveTime; ++endTimeIndex)
	{
		ObserveTime endObserveTime = (ObserveTime)endTimeIndex;
		int32_t bestBuyPrice = getMinPrice(dayInfo, sellObserveTime, endObserveTime);
		if (bestBuyPrice <= tBuyPrice)
		{
			m_spFund->buyAll(stock, tBuyPrice, date, endObserveTime);
			isBuy = true;
			break;
		}
		int32_t bestSellPrice = getMaxPrice(dayInfo, sellObserveTime, endObserveTime);
		if (bestSellPrice >= chaseBuyPrice)
		{
			m_spFund->buyAll(stock, chaseBuyPrice, date, endObserveTime);
			isBuy = true;
			break;
		}
	}

	if (!isBuy)
	{
		m_spFund->buyAll(stock, forceBuyPrice, date, forceBuyObserveTime);
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