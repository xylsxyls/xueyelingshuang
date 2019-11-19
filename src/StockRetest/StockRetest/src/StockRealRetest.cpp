#include "StockRealRetest.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "StockEveryTestTask.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockMysql/StockMysqlAPI.h"
#include <algorithm>
#include "StockTrade/StockTradeAPI.h"
#include "StockFund/StockFundAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include <math.h>

StockRealRetest::StockRealRetest():
m_strategyEnum(STRATEGY_INIT),
m_showStockLog(false),
m_beginTime(0, 0),
m_endTime(0, 0),
m_resultThreadId(0)
{
	
}

StockRealRetest::~StockRealRetest()
{
	int32_t index = -1;
	while (index++ != m_vecThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecThreadId[index]);
	}
	CTaskThreadManager::Instance().Uninit(m_resultThreadId);
}

void StockRealRetest::init(StrategyEnum strategyEnum,
	const std::vector<std::string>& allStock,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	bool showStockLog,
	int32_t threadCount)
{
	m_strategyEnum = strategyEnum;
	m_allStock = allStock;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_showStockLog = showStockLog;
	if (threadCount == 0)
	{
		threadCount = CSystem::GetCPUCoreCount();
	}
	while (threadCount-- != 0)
	{
		m_vecThreadId.push_back(CTaskThreadManager::Instance().Init());
	}
	m_resultThreadId = CTaskThreadManager::Instance().Init();
}

void StockRealRetest::run()
{
	if (m_beginTime > m_endTime)
	{
		return;
	}

	//std::vector<std::string> vecStock = m_allStock.empty() ? StockMysql::instance().readFilterStockFromRedis() : m_allStock;
	//std::sort(vecStock.begin(), vecStock.end());

	BigNumber money = "20000000000";
	StockFund fund;
	fund.add(money);

	CStopWatch stopWatch;
	StockTrade stockTrade;
	auto vecStrategyStock = StockStrategy::instance().strategyAllStock(m_strategyEnum, m_beginTime, m_endTime);
	stockTrade.init(m_beginTime, m_endTime, vecStrategyStock, m_strategyEnum);
	stockTrade.load();

	std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>> buyInfo;
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>> allBuyInfo;
	
	IntDateTime currentTime = m_beginTime;
	do
	{
		RCSend(currentTime.dateToString().c_str());

		std::vector<std::string> vecOwnStock = fund.ownedStock();
		BigNumber price;
		BigNumber percent;
		int32_t index = -1;
		while (index++ != vecOwnStock.size() - 1)
		{
			const std::string& stock = vecOwnStock[index];
			fund.buyInfo(stock, buyInfo);
			if (!stockTrade.sell(stock, currentTime, price, percent, buyInfo))
			{
				continue;
			}
			std::shared_ptr<StockMarket> spMarket = stockTrade.market(stock);
			if (!spMarket->setDate(currentTime))
			{
				continue;
			}
			fund.sellStock(price, percent / "100.0", spMarket->day());
		}
		
		if (currentTime == m_endTime)
		{
			break;
		}
		std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> buyStock;
		stopWatch.Run();
		fund.allBuyInfo(allBuyInfo);
		stockTrade.buy(buyStock, currentTime, allBuyInfo);
		stopWatch.Stop();

		if (buyStock.size() < 4)
		{
			currentTime = currentTime + 86400;
			continue;
		}
		change(buyStock);
		index = -1;
		while (index++ != buyStock.size() - 1)
		{
			const std::string& stock = buyStock[index].first;
			const BigNumber& price = buyStock[index].second.first;
			const BigNumber& rate = buyStock[index].second.second;
			std::shared_ptr<StockMarket> spMarket = stockTrade.market(stock);
			if (!spMarket->setDate(currentTime))
			{
				continue;
			}
			fund.buyStock(price, rate, spMarket->day());
		}

		currentTime = currentTime + 86400;
	} while (currentTime <= m_endTime);
	
	std::vector<std::string> vecOwnStock = fund.ownedStock();
	std::map<std::string, std::shared_ptr<StockDay>> dayDate;
	int32_t index = -1;
	while (index++ != vecOwnStock.size() - 1)
	{
		const std::string& stock = vecOwnStock[index];
		std::shared_ptr<StockMarket> spMarket = stockTrade.market(stock);
		spMarket->setEndDate();
		dayDate[stock] = spMarket->day();
	}
	BigNumber allFund = fund.allFund(dayDate);
	index = -1;
	std::vector<std::string> stockLog = fund.stockLog();
	while (index++ != stockLog.size() - 1)
	{
		RCSend("%s", stockLog[index].c_str());
	}
	RCSend("allFund = %s, time = %d", allFund.toString().c_str(), stopWatch.GetWatchTime());
}

void StockRealRetest::change(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock)
{
	BigNumber allScore = 0;
	int32_t index = -1;
	while (index++ != (std::min)((int32_t)buyStock.size(), 5) - 1)
	{
		allScore = allScore + buyStock[index].second.second;
	}
	index = -1;
	while (index++ != buyStock.size() - 1)
	{
		BigNumber score = buyStock[index].second.second;
		buyStock[index].second.second = score / BigNumber((std::max)(atoi(allScore.toString().c_str()), 100)).toPrec(2).zero();
		allScore = allScore - score;
	}
}
