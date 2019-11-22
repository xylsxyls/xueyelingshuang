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
	const BigNumber initialFund,
	bool showStockLog,
	int32_t threadCount)
{
	m_strategyEnum = strategyEnum;
	m_allStock = allStock;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_initialFund = initialFund;
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

	StockFund fund;
	fund.add(m_initialFund);

	StockTrade stockTrade;
	std::vector<std::string> vecStrategyStock = StockStrategy::instance().strategyAllStock(m_strategyEnum, m_beginTime, m_endTime);
	stockTrade.init(m_beginTime, m_endTime, vecStrategyStock, m_strategyEnum);
	stockTrade.load();

	std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>> buyInfo;
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>> allBuyInfo;
	
	IntDateTime currentTime = m_beginTime;
	do
	{
		std::map<std::string, std::shared_ptr<StockDay>> dayData;
		stockTrade.stockDayData(fund.ownedStock(), currentTime, dayData);
		BigNumber allFund = fund.allFund(dayData);
		RCSend("date = %s, allFund = %s, profit = %s%%", currentTime.dateToString().c_str(),
			allFund.toString().c_str(),
			(((allFund - m_initialFund) / m_initialFund.toPrec(16).zero() * 100).toPrec(2)).toString().c_str());

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
		fund.allBuyInfo(allBuyInfo);
		stockTrade.buy(buyStock, currentTime, allBuyInfo);

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
	
	std::map<std::string, std::shared_ptr<StockDay>> dayData;
	stockTrade.stockDayData(fund.ownedStock(), m_endTime, dayData);
	BigNumber allFund = fund.allFund(dayData);
	if (m_showStockLog)
	{
		fund.printStockLog();
	}
	RCSend("allFund = %s, profit = %s%%", allFund.toString().c_str(),
		(((allFund - m_initialFund) / m_initialFund.toPrec(16).zero() * 100).toPrec(2)).toString().c_str());
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
