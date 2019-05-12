#include "StockStrategy.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndex/StockIndexAPI.h"
#include "Strategy.h"
#include "StockFund/StockFundAPI.h"
#include "CStopWatch/CStopWatchAPI.h"

void StockStrategy::init(Strategy* strategy, StockFund* stockFund)
{
	m_stockFund = stockFund;
	m_strategy = strategy;
	m_strategy->init(m_stockFund);
}

void StockStrategy::run(const std::vector<std::string>& vecStock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	if (!check())
	{
		return;
	}
	if (beginTime >= endTime)
	{
		return;
	}
	CStopWatch watch;
	std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>> vecValidStock;
	std::vector<StockHandle> vecHandle;
	IntDateTime time = beginTime;
	do
	{
		vecValidStock.clear();
		int32_t index = -1;
		while (index++ != vecStock.size() - 1)
		{
			const std::string& stock = vecStock[index];
			const std::shared_ptr<StockMarket>& spStockMarket = StockAllMarket::instance().history(stock);
			if (!spStockMarket->dateExist(time))
			{
				continue;
			}
			vecValidStock[stock] = std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>(spStockMarket, StockAllIndex::instance().index(stock));
		}
		if (vecValidStock.empty())
		{
			continue;
		}
		vecHandle.clear();
		watch.Run();
		m_strategy->strategy(time, vecValidStock, vecHandle);
		watch.Stop();
		int32_t handleIndex = -1;
		while (handleIndex++ != vecHandle.size() - 1)
		{
			const StockHandle& handle = vecHandle[handleIndex];
			if (handle.m_isBuy)
			{
				if (handle.m_freeBeforeBuy != 0)
				{
					m_stockFund->free(handle.m_freeBeforeBuy);
				}
				if (m_stockFund->buyStock(time, handle.m_price, StockAllMarket::instance().history(handle.m_stock), handle.m_rate))
				{
					if (handle.m_freezeOrFree != 0)
					{
						m_stockFund->freeze(handle.m_freezeOrFree);
					}
				}
			}
			else
			{
				if (m_stockFund->sellStock(time, handle.m_price, handle.m_stock, handle.m_rate))
				{
					if (handle.m_freezeOrFree != 0)
					{
						m_stockFund->free(handle.m_freezeOrFree);
					}
				}
			}
		}
	} while ((time = time + 86400) <= endTime);
	RCSend("StockStrategy run watch = %d", watch.GetWatchTime());
}

bool StockStrategy::check()
{
	return m_strategy != nullptr && m_stockFund != nullptr;
}