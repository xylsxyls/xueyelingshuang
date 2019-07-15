#include "EveryTestTask.h"
#include "StockFund/StockFundAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMarket/StockMarketAPI.h"

void EveryTestTask::DoTask()
{
	//RCSend("1");
	//return;
	RCSend("%d", ::GetTickCount());
	std::string stock = m_stock;
	//RCSend("%s", m_stock.c_str());
	IntDateTime beginTime = "2014-02-27";
	IntDateTime endTime = "2019-02-27";

	//std::shared_ptr<StockWrIndicator> spStockWrIndicatorTest(new StockWrIndicator);
	//spStockWrIndicatorTest->load(stock, beginTime, endTime);
	//std::shared_ptr<StockWrIndicator> spStockWrIndicator = StockIndicator::instance().wr(stock, beginTime, endTime);
	//std::shared_ptr<StockRsiIndicator> spStockRsiIndicator = StockIndicator::instance().rsi(stock, beginTime, endTime);
	//RCSend("1");
	//return;
	
	CStopWatch marketWatch;
	std::shared_ptr<StockMarket> m_spMarket(new StockMarket);
	m_spMarket->load(stock, beginTime, endTime);
	int32_t marketWatchTime = marketWatch.GetWatchTime();
	StockFund fund;
	fund.add(100000);
	std::shared_ptr<StockWrIndicator> m_spStockWrIndicator = StockIndicator::instance().wr(stock, beginTime, endTime);
	std::shared_ptr<StockRsiIndicator> m_spStockRsiIndicator = StockIndicator::instance().rsi(stock, beginTime, endTime);
	//std::shared_ptr<StockWrIndicator> spStockWrIndicator(new StockWrIndicator);
	//spStockWrIndicator->load(stock, beginTime, endTime);
	//std::shared_ptr<StockRsiIndicator> spStockRsiIndicator(new StockRsiIndicator);
	//spStockRsiIndicator->load(stock, beginTime, endTime);
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
	
		//BigNumber wr10 = StockIndex::wr(10, time, market);
		//BigNumber rsi6 = StockIndex::rsi(6, time, market);
		//BigNumber rsi24 = StockIndex::rsi(24, time, market);
	
		if (fund.stockNum() == 0 && wr10 >= 95 && rsi24 <= 45)
		{
			fund.buyStock(close, 0.5, m_spMarket->day());
		}
		else if (fund.stockNum() == 1 && wr10 >= 95 && rsi24 <= 45)
		{
			fund.buyStock(close, 1, m_spMarket->day());
		}
		else if (fund.stockNum() == 1 && wr10 < 50)
		{
			BigNumber chg = 0;
			fund.stockChg(m_spMarket->stock(), m_spMarket->day(), chg);
			fund.sellStock(close, 1, m_spMarket->day());
		}
	} while (m_spMarket->next());
	int32_t indexWatchTime = indexWatch.GetWatchTime();
	BigNumber allFund = fund.allFund(m_spMarket->day());
	RCSend("marketWatchTime = %d, indexWatchTime = %d, allFund = %s", marketWatchTime, indexWatchTime, allFund.toString().c_str());
	std::vector<std::string> vec = fund.stockLog();
	//RCSend("%s end", m_stock.c_str());
	//RCSend("%d end", ::GetTickCount());
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
