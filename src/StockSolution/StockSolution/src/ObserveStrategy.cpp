#include "ObserveStrategy.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <algorithm>
#include "ObserveStrategyInfo.h"
#include "StockFund/StockFundAPI.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockSolution.h"
#include "AvgFundHighScore.h"

ObserveStrategy::ObserveStrategy():
m_stockNum(4),
m_calcDays(0)
{
	m_solutionType = OBSERVE_STRATEGY;
	m_avgSolution = std::dynamic_pointer_cast<AvgFundHighScore>(StockSolution::instance().solution(AVG_FUND_HIGH_SCORE));
}

void ObserveStrategy::init(StrategyType strategyType, int32_t calcDays)
{
	m_strategyType = strategyType;
	m_calcDays = calcDays;
}

bool ObserveStrategy::buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_avgSolution->Solution::init(m_mapStrategy);

	m_avgSolution->setParam(m_strategyType);
	solutionInfo->setParam(m_strategyType);

	std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>> beforeDayBuyStock;
	if (!m_avgSolution->buy(beforeDayBuyStock, getBeforeDay("000001", date, m_calcDays, solutionInfo), solutionInfo))
	{
		return false;
	}

	std::map<BigNumber, std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>> beforeDayBuyStockMap;

	int32_t index = -1;
	while (index++ != beforeDayBuyStock.size() - 1)
	{
		bool isSkip = false;
		const std::string& stock = beforeDayBuyStock[index].first;
		std::shared_ptr<StockMarket> spMarket = solutionInfo->m_strategyAllInfo.find(stock)->second.begin()->second.first->m_spMarket;
		BigNumber avgChgValue = 0;
		int32_t calcDays = m_calcDays;
		while (calcDays-- != 0)
		{
			if (!spMarket->setDate(getBeforeDay(stock, date, calcDays, solutionInfo)))
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
			std::shared_ptr<StockMarket> spMarket = solutionInfo->m_strategyAllInfo.find(stock)->second.begin()->second.first->m_spMarket;
			spMarket->setDate(date);
			vecStock[index].second.first = spMarket->day()->close();
			vecStock[index].second.second = 1;
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
	if (buyStock.empty())
	{
		return false;
	}

	index = -1;
	while (index++ != buyStock.size() - 1)
	{
		buyStock[index].second.second = (BigNumber(1) / BigNumber((int32_t)buyStock.size() - index).toPrec(6)).toPrec(6);
	}

	if (buyStock.size() < 2)
	{
		buyStock.clear();
		return false;
	}
	auto buyPair = buyStock[1];
	buyPair.second.second = 1;
	const std::string& stockBuy = buyPair.first;
	std::shared_ptr<StockMarket> spStockBuyMarket = solutionInfo->m_strategyAllInfo.find(stockBuy)->second.begin()->second.first->m_spMarket;
	spStockBuyMarket->setDate(getBeforeDay(stockBuy, date, 1, solutionInfo));
	BigNumber firstAvg = spStockBuyMarket->day()->fourAvgChgValue();
	BigNumber firstChg = spStockBuyMarket->day()->chgValue();
	BigNumber firstOpenClose = spStockBuyMarket->day()->open() - spStockBuyMarket->day()->close();
	spStockBuyMarket->next();
	BigNumber secondAvg = spStockBuyMarket->day()->fourAvgChgValue();
	BigNumber secondChg = spStockBuyMarket->day()->chgValue();
	BigNumber secondOpenClose = spStockBuyMarket->day()->open() - spStockBuyMarket->day()->close();

	auto buyPairFirst = buyStock[0];
	buyPairFirst.second.second = "0.5";
	buyStock.clear();
	//if (firstAvg + secondAvg < 0)
	//{
	//	return false;
	//}
	if (firstChg + secondChg < 0)
	{
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

	//buyStock.push_back(buyPairFirst);
	buyStock.push_back(buyPair);
	return true;
}

bool ObserveStrategy::sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
	const IntDateTime& date,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	m_avgSolution->Solution::init(m_mapStrategy);
	return m_avgSolution->sell(sellStock, date, solutionInfo);
}

IntDateTime ObserveStrategy::getBeforeDay(const std::string& stock,
	const IntDateTime& date,
	int32_t days,
	const std::shared_ptr<SolutionInfo>& solutionInfo)
{
	if (solutionInfo->m_strategyAllInfo.find(stock) == solutionInfo->m_strategyAllInfo.end())
	{
		return IntDateTime(0, 0);
	}
	std::shared_ptr<StockMarket> spMarket = solutionInfo->m_strategyAllInfo.find(stock)->second.begin()->second.first->m_spMarket;
	if (!spMarket->setDate(date))
	{
		return IntDateTime(0, 0);
	}
	return spMarket->getDateBefore(days);
}