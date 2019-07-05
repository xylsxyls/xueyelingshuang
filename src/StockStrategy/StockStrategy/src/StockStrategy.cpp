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
}

void StockStrategy::times(const std::vector<std::string>& vecStock, const IntDateTime& beginTime, const IntDateTime& endTime)
{

}

void StockStrategy::profit(const std::vector<std::string>& vecStock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	std::map<BigNumber, std::vector<std::string>> profitMap;
	BigNumber allProfit = 0;
	int32_t index = -1;
	while (index++ != vecStock.size() - 1)
	{
		IntDateTime date = beginTime;
		const std::string& stock = vecStock[index];
		std::shared_ptr<StockMarket> spStockMarket = StockAllMarket::instance().history(stock);
		IntDateTime stockBeginDate = spStockMarket->beginDate();
		if (stockBeginDate.empty())
		{
			continue;
		}
		if (date < stockBeginDate)
		{
			date = stockBeginDate;
		}
		std::vector<std::string> vecRunStock;
		vecRunStock.push_back(stock);
		run(vecRunStock, date, endTime);
		std::vector<std::pair<std::vector<BigNumber>, std::vector<IntDateTime>>> dataLog = m_stockFund->dataLog();
		m_stockFund->clear();
		m_stockFund->add(100000);
		int32_t logIndex = -1;
		while (logIndex++ != dataLog.size() - 1)
		{
			std::string every;
			every.append(dataLog[logIndex].first[0].toString() + ",");
			every.append(dataLog[logIndex].first[2].toString() + ",");
			every.append(dataLog[logIndex].first[3].toString() + ",");
			int32_t timeIndex = -1;
			while (timeIndex++ != dataLog[logIndex].second.size() - 1)
			{
				every.append(dataLog[logIndex].second[timeIndex].dateToString() + ",");
			}
			profitMap[dataLog[logIndex].first[1]].push_back(every);
			allProfit = allProfit + (dataLog[logIndex].first[1] / 100) * (dataLog[logIndex].first[2] * 100000);
		}
	}
	
	int32_t times = 0;
	for (auto itProfit = profitMap.begin(); itProfit != profitMap.end(); ++itProfit)
	{
		int32_t profitIndex = -1;
		while (profitIndex++ != itProfit->second.size() - 1)
		{
			RCSend((itProfit->first.toString() + "," + itProfit->second[profitIndex]).c_str());
			++times;
		}
	}
	RCSend("平均一次收益：%s%", (allProfit / times / 100000 * 100).toPrec(2).toString().c_str());
}

bool StockStrategy::check()
{
	return m_strategy != nullptr && m_stockFund != nullptr;
}