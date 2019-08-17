#include "EveryTestTask.h"
#include "StockFund/StockFundAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMarket/StockMarketAPI.h"

void EveryTestTask::DoTask()
{
	RCSend("%d", ::GetTickCount());
	std::string stock = m_stock;
	
	CStopWatch marketWatch;
	m_spMarket->load();
	m_spStockWrIndicator->load();
	m_spStockRsiIndicator->load();
	int32_t marketWatchTime = marketWatch.GetWatchTime();

	StockFund fund;
	fund.add(100000);
	CStopWatch indexWatch;
	do
	{
		IntDateTime time = m_spMarket->date();
		if (time.empty())
		{
			break;
		}
	
		BigNumber close = m_spMarket->day()->close();
		if (close < 1)
		{
			continue;
		}
	
		BigNumber wr10 = m_spStockWrIndicator->day(time)->wr10();
		BigNumber rsi6 = m_spStockRsiIndicator->day(time)->rsi6();
		BigNumber rsi24 = m_spStockRsiIndicator->day(time)->rsi24();

		BigNumber chg = 0;
		fund.stockChg(m_spMarket->stock(), m_spMarket->day(), chg);
	
		//BigNumber wr10 = StockIndex::wr(10, time, market);
		//BigNumber rsi6 = StockIndex::rsi(6, time, market);
		//BigNumber rsi24 = StockIndex::rsi(24, time, market);
	
		if (fund.stockNum() == 0 && wr10 >= 90 && rsi24 <= 45)
		{
			fund.buyStock(close, 0.5, m_spMarket->day());
		}
		else if (fund.stockNum() == 1 && wr10 >= 90 && rsi24 <= 45)
		{
			fund.buyStock(close, 1, m_spMarket->day());
		}
		else if ((fund.stockNum() == 1 && wr10 < 10 && chg > 10))
		{
			fund.sellStock(close, 1, m_spMarket->day());
		}
	} while (m_spMarket->next());
	int32_t indexWatchTime = indexWatch.GetWatchTime();
	BigNumber allFund = fund.allFund(m_spMarket->day());
	RCSend("stock = %s, marketWatchTime = %d, indexWatchTime = %d, allFund = %s", stock.c_str(), marketWatchTime, indexWatchTime, allFund.toString().c_str());
	std::vector<std::string> vec = fund.stockLog();
	static BigNumber allStockFund = 0;
	static int x = 0;
	static std::mutex mu;
	mu.lock();
	allStockFund = allStockFund + allFund;
	++x;
	if (x == 3507)
	{
		RCSend("avg = %s", (allStockFund / 3507.0).toPrec(2).toString().c_str());
	}
	mu.unlock();
}

void EveryTestTask::setParam(const std::string& stock,
	const std::shared_ptr<StockMarket>& spMarket,
	const std::shared_ptr<StockWrIndicator>& spStockWrIndicator,
	const std::shared_ptr<StockRsiIndicator>& spStockRsiIndicator)
{
	m_stock = stock;
	m_spMarket = spMarket;
	m_spStockWrIndicator = spStockWrIndicator;
	m_spStockRsiIndicator = spStockRsiIndicator;
}

void EveryTestTask::setParam(const std::string& stock)
{
	m_stock = stock;
}
