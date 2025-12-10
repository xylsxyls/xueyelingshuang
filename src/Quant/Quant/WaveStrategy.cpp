#include "WaveStrategy.h"
#include <algorithm>
#include <iostream>
#include "Util.h"

enum
{
	OPERATE,
	HOLD,
	GIVE_UP
};

WaveStrategy::WaveStrategy() :
m_operate(0),
m_hasFirstBuy(false)
{
	m_mode = StrategyMode::WAVE;
	m_modeName = "Wave";
}

WaveStrategy::~WaveStrategy()
{

}

bool WaveStrategy::onTradingDay(uint32_t date)
{
	if (!isStrategyParamValid())
	{
		RCSend("isStrategyParamValid");
		return false;
	}
	
	// 只处理第一只股票（可根据需要扩展为多股票）
	const std::string& stock = m_vecStock[0];
	const std::vector<int32_t>& dayInfo = m_spMarket->getStockData(stock, date);
	if (dayInfo.empty())
	{
		RCSend("dayInfo empty");
		return false;
	}
	

	// 解析策略参数
	ObserveTime hangSellObserveTime = (ObserveTime)m_strategyParam[0]; // 挂卖价时间点
	ObserveTime directSellObserveTime = (ObserveTime)m_strategyParam[1]; // 直卖价时间点
	ObserveTime hangBuyObserveTime = (ObserveTime)m_strategyParam[2]; // 挂买价时间点
	ObserveTime directBuyObserveTime = (ObserveTime)m_strategyParam[3]; // 直买价时间点
	ObserveTime giveUpHangSellObserveTime = hangBuyObserveTime; // 放弃挂卖价时间点
	ObserveTime giveUpDirectSellObserveTime = directBuyObserveTime; // 放弃直卖价时间点
	ObserveTime aObserveTime = (ObserveTime)m_strategyParam[4]; // 观察价A时间点
	ObserveTime bObserveTime = (ObserveTime)m_strategyParam[5]; // 观察价B时间点
	int32_t holdCent = m_strategyParam[6];    // 观察价B-A大于等于多少时当天直买，第二天早上不卖
	int32_t giveUpCent = m_strategyParam[7];    // 观察价B-A小于等于多少时当天不买，剩余的部分，当天直买，第二天早上卖
	int32_t chaseCent = m_strategyParam[8];    // 当天早上卖出后反追差价

	if ((int32_t)directSellObserveTime < (int32_t)hangSellObserveTime ||
		(int32_t)directBuyObserveTime < (int32_t)hangBuyObserveTime ||
		(int32_t)hangBuyObserveTime <= ((int32_t)directSellObserveTime + 1) ||
		(int32_t)bObserveTime <= (int32_t)hangSellObserveTime ||
		giveUpCent >= holdCent)
	{
		return false;
	}
	
	// 需要的价格指标
	int32_t hangSellPrice = getCurrentPrice(dayInfo, hangSellObserveTime); // 挂卖价时间点
	int32_t directSellPrice = getDirectSellPrice(dayInfo, directSellObserveTime); // 直卖价时间点
	int32_t hangBuyPrice = getCurrentPrice(dayInfo, hangBuyObserveTime) - 1; // 挂买价时间点
	int32_t directBuyPrice = getDirectBuyPrice(dayInfo, directBuyObserveTime); // 直买价时间点
	int32_t giveUpHangSellPrice = getCurrentPrice(dayInfo, hangBuyObserveTime); // 放弃挂卖价时间点
	int32_t giveUpDirectSellPrice = getDirectSellPrice(dayInfo, directBuyObserveTime); // 放弃直卖价时间点
	int32_t aPrice = getCurrentPrice(dayInfo, aObserveTime); // 观察价A时间点
	int32_t bPrice = getCurrentPrice(dayInfo, bObserveTime); // 观察价B时间点

	if (!m_hasFirstBuy)
	{
		updateOperate(directSellPrice, directBuyPrice);
		m_spFund->buyAll(stock, dayInfo[(int32_t)Overall::CLOSE], date, ObserveTime::COUNT);
		m_hasFirstBuy = true;
		return true;
	}

	// 检查当前是否持有该股票
	std::shared_ptr<Position> spPosition = m_spFund->getPosition(stock);
	if (spPosition == nullptr)
	{
		RCSend("hasPosition false");
		return false;
	}
	
	bool isBuy = false;
	bool hasSell = false;
	int32_t realSellPrice = 0;
	ObserveTime realSellObserveTime = ObserveTime::COUNT;

	// 早上卖出逻辑
	if (m_operate == OPERATE || (m_operate == GIVE_UP && spPosition->m_shares != 1))
	{
		if (spPosition->m_shares == 1)
		{
			RCSend("shares error");
		}

		if (hangSellObserveTime == directSellObserveTime)
		{
			realSellPrice = directSellPrice;
			realSellObserveTime = directSellObserveTime;
			m_spFund->sellAllForT(stock, directSellPrice, date, directSellObserveTime);
			hasSell = true;
		}
		else
		{
			int32_t bestSellPrice = getMaxPrice(dayInfo, hangSellObserveTime, directSellObserveTime);
			if (bestSellPrice < hangSellPrice)
			{
				realSellPrice = directSellPrice;
				realSellObserveTime = directSellObserveTime;
				m_spFund->sellAllForT(stock, directSellPrice, date, directSellObserveTime);
				hasSell = true;
			}
			else if (bestSellPrice >= hangSellPrice && bestSellPrice < hangSellPrice + chaseCent)
			{
				realSellPrice = hangSellPrice;
				realSellObserveTime = hangSellObserveTime;
				m_spFund->sellAllForT(stock, hangSellPrice, date, hangSellObserveTime);
				hasSell = true;
			}
			else
			{
				realSellPrice = hangSellPrice;
				realSellObserveTime = hangSellObserveTime;
				m_spFund->sellAllForT(stock, hangSellPrice, date, hangSellObserveTime);
				hasSell = true;
				m_spFund->buyAll(stock, hangSellPrice + chaseCent, date, directSellObserveTime);
				isBuy = true;
				m_operate = HOLD;
			}
		}
	}

	if (isBuy)
	{
		return true;
	}

	if (hasSell)
	{
		int32_t midBestSellPrice = getMaxPrice(dayInfo, realSellObserveTime, hangBuyObserveTime);
		if (midBestSellPrice >= realSellPrice + chaseCent)
		{
			m_spFund->buyAll(stock, realSellPrice + chaseCent, date, hangBuyObserveTime);
			isBuy = true;
			m_operate = HOLD;
		}
	}

	if (isBuy)
	{
		return true;
	}

	updateOperate(aPrice, bPrice);

	if (!hasSell)
	{
		if (m_operate == GIVE_UP)
		{
			if (giveUpHangSellObserveTime == giveUpDirectSellObserveTime)
			{
				realSellPrice = giveUpDirectSellPrice;
				realSellObserveTime = giveUpDirectSellObserveTime;
				m_spFund->sellAllForT(stock, giveUpDirectSellPrice, date, giveUpDirectSellObserveTime);
				hasSell = true;
			}
			else
			{
				int32_t bestSellPrice = getMaxPrice(dayInfo, giveUpHangSellObserveTime, giveUpDirectSellObserveTime);
				if (bestSellPrice < giveUpHangSellPrice)
				{
					realSellPrice = giveUpDirectSellPrice;
					realSellObserveTime = giveUpDirectSellObserveTime;
					m_spFund->sellAllForT(stock, giveUpDirectSellPrice, date, giveUpDirectSellObserveTime);
					hasSell = true;
				}
				else if (bestSellPrice >= giveUpHangSellPrice && bestSellPrice < giveUpHangSellPrice + chaseCent)
				{
					realSellPrice = giveUpHangSellPrice;
					realSellObserveTime = giveUpHangSellObserveTime;
					m_spFund->sellAllForT(stock, giveUpHangSellPrice, date, giveUpHangSellObserveTime);
					hasSell = true;
				}
				else
				{
					realSellPrice = giveUpHangSellPrice;
					realSellObserveTime = giveUpHangSellObserveTime;
					m_spFund->sellAllForT(stock, giveUpHangSellPrice, date, giveUpHangSellObserveTime);
					hasSell = true;
					m_spFund->buyAll(stock, giveUpHangSellPrice + chaseCent, date, giveUpDirectSellObserveTime);
					isBuy = true;
					m_operate = HOLD;
				}
			}
			
			if (isBuy)
			{
				return true;
			}

			int32_t endBestSellPrice = getMaxPrice(dayInfo, giveUpDirectSellObserveTime, ObserveTime::COUNT);
			if (endBestSellPrice >= realSellPrice + chaseCent)
			{
				m_spFund->buyAll(stock, realSellPrice + chaseCent, date, ObserveTime::COUNT);
				isBuy = true;
				m_operate = HOLD;
			}
		}
	}
	else
	{
		if (m_operate == OPERATE || m_operate == HOLD)
		{
			if (hangBuyObserveTime == directBuyObserveTime)
			{
				m_spFund->buyAll(stock, directBuyPrice, date, directBuyObserveTime);
				isBuy = true;
				if (directBuyPrice >= realSellPrice + chaseCent)
				{
					m_operate = HOLD;
				}
			}
			else
			{
				int32_t bestBuyPrice = getMinPrice(dayInfo, hangBuyObserveTime, directBuyObserveTime);
				if (bestBuyPrice <= hangBuyPrice)
				{
					m_spFund->buyAll(stock, hangBuyPrice, date, hangBuyObserveTime);
					isBuy = true;
				}
				else
				{
					int32_t bestSellPrice = getMaxPrice(dayInfo, hangBuyObserveTime, directBuyObserveTime);
					if (bestSellPrice >= realSellPrice + chaseCent)
					{
						m_spFund->buyAll(stock, realSellPrice + chaseCent, date, hangBuyObserveTime);
						isBuy = true;
						m_operate = HOLD;
					}
					else
					{
						m_spFund->buyAll(stock, directBuyPrice, date, directBuyObserveTime);
						isBuy = true;
						if (directBuyPrice >= realSellPrice + chaseCent)
						{
							m_operate = HOLD;
						}
					}
				}
			}
			
		}
	}
	
	return true;
}

std::string WaveStrategy::describeParam(const std::vector<int32_t>& params)
{
	std::string describe;
	for (size_t index = 0; index < params.size(); ++index)
	{
		describe += (index <= 5 ?
			(Util::observeTimeToWatchString((ObserveTime)params[index]) + ", ") :
			std::to_string(params[index]) + ", ");
	}
	if (!describe.empty())
	{
		describe.pop_back();
		describe.pop_back();
	}
	return describe;
}

bool WaveStrategy::isStrategyParamValid() const
{
	// 检查参数有效性
	if (m_strategyParam.size() != 9)
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

void WaveStrategy::updateOperate(int32_t sellPrice, int32_t buyPrice)
{
	int32_t holdCent = m_strategyParam[6];    // 观察价B-A大于等于多少时当天直买，第二天早上不卖
	int32_t giveUpCent = m_strategyParam[7];    // 观察价B-A小于等于多少时当天不买，剩余的部分，当天直买，第二天早上卖
	int32_t tPrice = sellPrice - buyPrice;
	if (tPrice >= holdCent)
	{
		m_operate = HOLD;
	}
	else if (tPrice <= giveUpCent)
	{
		m_operate = GIVE_UP;
	}
	else
	{
		m_operate = OPERATE;
	}
}
