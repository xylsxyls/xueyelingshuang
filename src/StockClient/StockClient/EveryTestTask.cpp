#include "EveryTestTask.h"
#include "StockFund/StockFundAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockClient.h"

EveryTestTask::EveryTestTask():
m_stockClient(nullptr)
{

}

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
	
		if (fund.stockNum() == 0 && wr10 >= 90 && rsi24 >= 35 && rsi24 <= 40)
		{
			fund.buyStock(close, 1, m_spMarket->day());
		}
		//else if (fund.stockNum() == 1 && wr10 >= 90 && rsi24 <= 45)
		//{
		//	fund.buyStock(close, 1, m_spMarket->day());
		//}
		//else if ((fund.stockNum() == 1 && chg < -20))
		//{
		//	fund.sellStock(close, 1, m_spMarket->day());
		//}
		else if ((fund.stockNum() == 1 && wr10 < 50 && chg > 100))
		{
			fund.sellStock(close, 1, m_spMarket->day());
		}
	} while (m_spMarket->next());
	BigNumber allFund = fund.allFund(m_spMarket->day());
	RCSend("stock = %s, allFund = %s", stock.c_str(), allFund.toString().c_str());
	static BigNumber allStockFund = 0;
	static int x = 0;
	static std::mutex mu;
	mu.lock();
	allStockFund = allStockFund + allFund;
	++x;
	if (x == m_stockClient->m_stockCount)
	{
		RCSend("avg = %s", (allStockFund / m_stockClient->m_stockCount).toPrec(2).toString().c_str());
	}
	mu.unlock();
}

void EveryTestTask::setParam(const std::string& stock,
	const std::shared_ptr<StockMarket>& spMarket,
	const std::shared_ptr<StockWrIndicator>& spStockWrIndicator,
	const std::shared_ptr<StockRsiIndicator>& spStockRsiIndicator,
	StockClient* stockClient)
{
	m_stock = stock;
	m_spMarket = spMarket;
	m_spStockWrIndicator = spStockWrIndicator;
	m_spStockRsiIndicator = spStockRsiIndicator;
	m_stockClient = stockClient;
}