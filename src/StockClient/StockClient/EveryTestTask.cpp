#include "EveryTestTask.h"
#include "StockFund/StockFundAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMarket/StockMarketAPI.h"

void EveryTestTask::DoTask()
{
	std::string stock = m_stock;
	IntDateTime beginTime = "2014-02-27";
	IntDateTime endTime = "2019-02-27";
	
	CStopWatch marketWatch;
	StockMarket market;
	market.load(stock, beginTime, endTime);
	int32_t marketWatchTime = marketWatch.GetWatchTime();
	StockFund fund;
	fund.add(100000);
	std::shared_ptr<StockWrIndicator> spStockWrIndicator = StockIndicator::instance().wr(stock, beginTime, endTime);
	std::shared_ptr<StockRsiIndicator> spStockRsiIndicator = StockIndicator::instance().rsi(stock, beginTime, endTime);
	CStopWatch indexWatch;
	do
	{
		IntDateTime time = market.date();
	
		BigNumber wr10 = spStockWrIndicator->day(time)->wr10();
		BigNumber rsi6 = spStockRsiIndicator->day(time)->rsi6();
		BigNumber rsi24 = spStockRsiIndicator->day(time)->rsi24();
	
		//BigNumber wr10 = StockIndex::wr(10, time, market);
		//BigNumber rsi6 = StockIndex::rsi(6, time, market);
		//BigNumber rsi24 = StockIndex::rsi(24, time, market);
	
		if (fund.stockNum() == 0 && wr10 >= 95 && rsi24 <= 45)
		{
			fund.buyStock(market.day()->close(), 0.5, market.day());
		}
		else if (fund.stockNum() == 1 && wr10 >= 95 && rsi24 <= 45)
		{
			fund.buyStock(market.day()->close(), 1, market.day());
		}
		else if (fund.stockNum() == 1 && wr10 < 50)
		{
			BigNumber chg = 0;
			fund.stockChg(market.stock(), time, chg);
			fund.sellStock(market.day()->close(), 1, market.day());
		}
	} while (market.next());
	int32_t indexWatchTime = indexWatch.GetWatchTime();
	BigNumber allFund = fund.allFund(endTime);
	RCSend("marketWatchTime = %d, indexWatchTime = %d, allFund = %s", marketWatchTime, indexWatchTime, allFund.toString().c_str());
	std::vector<std::string> vec = fund.stockLog();
}

void EveryTestTask::setParam(const std::string& stock)
{
	m_stock = stock;
}