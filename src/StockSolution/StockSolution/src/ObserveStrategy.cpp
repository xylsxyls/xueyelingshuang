#include "ObserveStrategy.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockSolution.h"
#include "AvgFundHighScore.h"
#include "SolutionInfo.h"

ObserveStrategy::ObserveStrategy():
m_stockNum(4),
m_calcDays(2),
m_storage(nullptr)
{
	m_solutionType = OBSERVE_STRATEGY;
}

void ObserveStrategy::init(int32_t stockNum, int32_t calcDays, StockStorageBase* storage)
{
	m_stockNum = stockNum;
	m_calcDays = calcDays;
	m_storage = storage;
}

void ObserveStrategy::setSolution(const std::shared_ptr<Solution>& spSolution)
{
	m_useSolution = spSolution;
}

bool ObserveStrategy::buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date)
{
	buyStock.clear();
	std::vector<std::pair<std::string, StockInfo>> beforeDayBuyStock;
	if (!m_useSolution->buy(beforeDayBuyStock, m_storage->moveDay(date, m_calcDays)))
	{
		return false;
	}

	std::map<BigNumber, std::vector<std::pair<std::string, StockInfo>>> beforeDayBuyStockMap;

	int32_t index = -1;
	while (index++ != beforeDayBuyStock.size() - 1)
	{
		bool isSkip = false;
		const std::string& stock = beforeDayBuyStock[index].first;
		std::shared_ptr<StockMarket> spMarket = getMarket(stock);
		BigNumber avgChgValue = 0;
		int32_t calcDays = m_calcDays;
		while (calcDays-- != 0)
		{
			if (!spMarket->setDate(m_storage->moveDay(date, calcDays)))
			{
				//ËµÃ÷tingpaiÁË
				RCSend("tingpai, stock = %s, date = %s", stock.c_str(), date.dateToString().c_str());
				isSkip = true;
				break;
			}
			avgChgValue = avgChgValue + spMarket->day()->fourAvgChgValue();
		}
		if (isSkip)
		{
			continue;
		}
		beforeDayBuyStockMap[avgChgValue].push_back(beforeDayBuyStock[index]);
	}

	for (auto itStock = beforeDayBuyStockMap.rbegin(); itStock != beforeDayBuyStockMap.rend(); ++itStock)
	{
		auto& vecStock = itStock->second;
		int32_t index = -1;
		while (index++ != vecStock.size() - 1)
		{
			const std::string& stock = vecStock[index].first;
			std::shared_ptr<StockMarket> spMarket = getMarket(stock);
			spMarket->setDate(date);
			vecStock[index].second.m_price = spMarket->day()->close();
			buyStock.push_back(vecStock[index]);
			if ((int32_t)buyStock.size() >= m_stockNum)
			{
				break;
			}
		}
		if ((int32_t)buyStock.size() >= m_stockNum)
		{
			break;
		}
	}

	if (buyStock.size() < 2)
	{
		return false;
	}
	const std::string& stockBuy = buyStock[1].first;
	std::shared_ptr<StockMarket> spStockBuyMarket = getMarket(stockBuy);
	spStockBuyMarket->setDate(m_storage->moveDay(date, 1));
	BigNumber firstAvg = spStockBuyMarket->day()->fourAvgChgValue();
	BigNumber firstChg = spStockBuyMarket->day()->chgValue();
	BigNumber firstOpenClose = spStockBuyMarket->day()->open() - spStockBuyMarket->day()->close();
	spStockBuyMarket->next();
	BigNumber secondAvg = spStockBuyMarket->day()->fourAvgChgValue();
	BigNumber secondChg = spStockBuyMarket->day()->chgValue();
	BigNumber secondOpenClose = spStockBuyMarket->day()->open() - spStockBuyMarket->day()->close();

	index = -1;
	while (index++ != buyStock.size() - 1)
	{
		buyStock[index].second.m_rate = (index == 1 ? "1.00" : "0.00");
	}

	//auto buyPairFirst = buyStock[0];
	//buyPairFirst.second.second = "0.5";
	//buyStock.clear();
	//if (firstAvg + secondAvg < 0)
	//{
	//	return false;
	//}
	if (firstChg + secondChg < 0)
	{
		buyStock[1].second.m_rate = "0.00";
		return false;
	}
	//if (firstAvg < -3 || secondAvg < -3)
	//{
	//	return false;
	//}
	//if (firstAvg < 1.5 && secondAvg < 1.5)
	//{
	//	return false;
	//}
	//if (firstOpenClose > 0 && secondOpenClose > 0)
	//{
	//	return false;
	//}
	return true;
}

bool ObserveStrategy::sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date)
{
	return m_useSolution->sell(sellStock, date);
}

int32_t ObserveStrategy::calcDays()
{
	return m_calcDays;
}

std::shared_ptr<StockMarket> ObserveStrategy::getMarket(const std::string& stock)
{
	return m_solutionInfo->m_allStrategyInfo.find(stock)->second.find(m_solutionInfo->m_chooseParam.m_useType)->second->m_spMarket;
}