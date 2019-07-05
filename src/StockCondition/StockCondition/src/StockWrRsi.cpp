#include "StockWrRsi.h"

void StockWrRsi::strategy(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock, std::vector<StockHandle>& vecStockHandle)
{
	/** 买卖条件，最多两只票
	买：
	1.WR>95,RSI<45,RSI>30，
	2.前一日WR比今日低，
	3.今日最低低于昨日最低，
	4.下影线小于实体的4分之1，
	5.今日跌幅大于2%小于9%，或昨日跌幅大于3%，
	6.收盘价>10
	7.非跌停
	8.rsi24-rsi6最大优先
	下仓一半
	补买：有冻结资金的话考虑补买，逻辑只考虑前3，第三条改为昨日最低上浮一个点
	卖：
	1.第二日下跌不卖，
	2.如果吞掉昨天3分之2实体则解冻资金，否则卖出，
	3.后续如果出现涨幅超2%或实体超3%，并且当前是有收益的则标记卖出，
	4.一旦标记之后出现上影线超过实体2分之1卖出，下跌小于2%且有收益卖出，开盘价大于收盘价且上涨卖出，
	5.如果下跌至亏损则取消标记，
	6.WR小于10且有收益且非涨停卖出
	*/

	vecStockHandle.clear();
	if (m_stockFund->stockNum() != 0)
	{
		const std::vector<std::string>& vecOwnedStock = m_stockFund->ownedStock();
		int32_t index = -1;
		while (index++ != vecOwnedStock.size() - 1)
		{
			const std::string& ownedStock = vecOwnedStock[index];
			auto itValidStock = validStock.find(ownedStock);
			if (itValidStock != validStock.end())
			{
				if (needSell(date, itValidStock->second.first, itValidStock->second.second))
				{
					m_stockHandle.m_isBuy = false;
					m_stockHandle.m_stock = ownedStock;
					m_stockHandle.m_freezeOrFree = 1;
					m_stockHandle.m_price = itValidStock->second.first->close(date);
					m_stockHandle.m_rate = 1;
					m_stockHandle.m_freeBeforeBuy = 0;
					vecStockHandle.push_back(m_stockHandle);
				}
			}
		}
	}
	if (m_stockFund->hasFreezeFund())
	{
		const std::vector<std::string>& vecOwnedStock = m_stockFund->ownedStock();
		int32_t index = -1;
		while (index++ != vecOwnedStock.size() - 1)
		{
			const std::string& ownedStock = vecOwnedStock[index];
			auto itValidStock = validStock.find(ownedStock);
			if (itValidStock != validStock.end())
			{
				if (addBuy(date, itValidStock->second.first, itValidStock->second.second))
				{
					m_stockHandle.m_isBuy = true;
					m_stockHandle.m_stock = ownedStock;
					m_stockHandle.m_freezeOrFree = 0;
					m_stockHandle.m_price = itValidStock->second.first->close(date);
					m_stockHandle.m_rate = 1;
					m_stockHandle.m_freeBeforeBuy = 1;
					vecStockHandle.push_back(m_stockHandle);
				}
			}
		}
	}
	else if (m_stockFund->hasAvailableFund())
	{
		availBuyStock(date, validStock, m_vecChooseStock);
		if (!m_vecChooseStock.empty())
		{
			m_stockHandle.m_isBuy = true;
			m_stockHandle.m_stock = m_vecChooseStock[0].first;
			m_stockHandle.m_freezeOrFree = 1;
			m_stockHandle.m_price = m_vecChooseStock[0].second;
			m_stockHandle.m_rate = ((m_stockFund->stockNum() == 0) ? 0.5 : 1);
			m_stockHandle.m_freeBeforeBuy = 0;
			vecStockHandle.push_back(m_stockHandle);
		}
	}
}

void StockWrRsi::availBuyStock(const IntDateTime& date,
	const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock,
	std::vector<std::pair<std::string, BigNumber>>& vecAvailBuyStock)
{
	/**
	1.WR>95,RSI<45,RSI>30，
	2.前一日WR比今日低，
	3.今日最低低于昨日最低，
	4.下影线小于实体的4分之1，
	5.今日跌幅大于2%，或昨日跌幅大于3%，
	6.收盘价>10
	7.非跌停
	8.rsi24-rsi6最大优先
	下仓一半
	*/
	vecAvailBuyStock.clear();
	m_mapChooseStock.clear();
	for (auto itValidStock = validStock.begin(); itValidStock != validStock.end(); ++itValidStock)
	{
		const std::string& stock = itValidStock->first;
		const std::shared_ptr<StockMarket>& spStockMarket = validStock.find(stock)->second.first;
		const std::shared_ptr<StockIndex>& spStockIndex = validStock.find(stock)->second.second;
		if (!spStockMarket->dateExist(date))
		{
			continue;
		}
		IntDateTime preDate;
		if (!spStockMarket->getDatePre(date, preDate))
		{
			continue;
		}
		BigNumber close = spStockMarket->close(date);
		BigNumber wr10Pre = spStockIndex->wr10(preDate);
		BigNumber wr10 = spStockIndex->wr10(date);
		BigNumber rsi6 = spStockIndex->rsi6(date);
		BigNumber rsi6Pre = spStockIndex->rsi6(preDate);
		BigNumber rsi12 = spStockIndex->rsi12(date);
		BigNumber rsi24 = spStockIndex->rsi24(date);
		BigNumber downValue = spStockMarket->downValue(date);
		BigNumber entityValue = spStockMarket->entityValue(date);
		BigNumber chgValue = spStockMarket->chgValue(date);
		IntDateTime firstDate = spStockMarket->beginDate();
		if ((wr10 > 90 && rsi24 < 40 && rsi24 > 35/*&& rsi24 > 35*/) &&
			(wr10Pre < wr10) &&
			(spStockMarket->low(date) < spStockMarket->low(preDate)) &&
			((entityValue != 0) && (downValue < (1 / 4.0) * entityValue.toPrec(4))) &&
			((chgValue < -2 && chgValue > -7) || spStockMarket->chgValue(preDate) < -3) &&
			(close > 10) &&
			(!spStockMarket->isLimitDown(date)) &&
			(date - 86400 * 365 > firstDate))
		//if ((wr10 > 95 && rsi24 < 50 && rsi24 > 40 && close > 0) &&
		//	//wr10Pre < wr10 &&
		//	chgValue > -9)
		{
			m_chooseStock.first = stock;
			m_chooseStock.second = close;
			m_mapChooseStock[rsi24 - rsi6].push_back(m_chooseStock);
		}
	}
	for (auto itChooseStock = m_mapChooseStock.rbegin(); itChooseStock != m_mapChooseStock.rend(); ++itChooseStock)
	{
		const std::vector<std::pair<std::string, BigNumber>>& vecStock = itChooseStock->second;
		int32_t index = -1;
		while (index++ != vecStock.size() - 1)
		{
			vecAvailBuyStock.push_back(vecStock[index]);
		}
	}
}

bool StockWrRsi::addBuy(const IntDateTime& date, const std::shared_ptr<StockMarket>& spStockMarket, const std::shared_ptr<StockIndex>& spStockIndex)
{
	/**
	1.WR>90,RSI<45,RSI>30，
	2.前一日WR比今日低，
	3.今日最低低于昨日最低上浮一个点，
	4.非跌停
	*/
	if (!spStockMarket->dateExist(date))
	{
		return false;
	}
	IntDateTime preDate;
	if (!spStockMarket->getDatePre(date, preDate))
	{
		return false;
	}
	BigNumber wr10Pre = spStockIndex->wr10(preDate);
	BigNumber wr10 = spStockIndex->wr10(date);
	BigNumber rsi24 = spStockIndex->rsi24(date);
	if ((wr10 > 90 && rsi24 < 40 && rsi24 > 35/*rsi24 < 45 && rsi24 > 30*/) &&
		(wr10Pre < wr10) &&
		(spStockMarket->low(date) < spStockMarket->low(preDate) * 1.01) &&
		(!spStockMarket->isLimitDown(date)))
	{
		return true;
	}
	return false;
}

bool StockWrRsi::needSell(const IntDateTime& date, const std::shared_ptr<StockMarket>& spStockMarket, const std::shared_ptr<StockIndex>& spStockIndex)
{
	std::string stock = spStockMarket->stock();
	std::vector<IntDateTime> vecTime = m_stockFund->ownedTime(stock);
	if (vecTime.empty())
	{
		return false;
	}
	const IntDateTime& firstBuyTime = vecTime[0];
	BigNumber days = spStockMarket->getDays(vecTime[0], date);
	BigNumber chg = 0;
	bool res = m_stockFund->stockChg(stock, date, chg);
	bool firstBuy = (vecTime.size() == 1);
	if (firstBuy)
	{
		if (days == 2)
		{
			if (spStockMarket->chgValue(date) > 0 && spStockMarket->close(date) < ((spStockMarket->open(firstBuyTime) - spStockMarket->close(firstBuyTime)).toPrec(6) * 2 / 3 + spStockMarket->close(firstBuyTime)))
			{
				m_stockFund->allFund(date);
				return true;
			}
		}
		else
		{
			if (chg > 0 && spStockMarket->entityValue(date) < spStockMarket->upValue(date) * 2)
			{
				m_stockFund->allFund(date);
				return true;
			}
		}
	}
	bool result = spStockIndex->wr10(date) < 10 && spStockMarket->chgValue(date) < 9.8;
	if (result)
	{
		m_stockFund->allFund(date);
	}
	return result;

	//const BigNumber& stockNum = m_stockFund->stockNum();
	//if (stockNum != 0)
	//{
	//	std::vector<std::string> vecOwnedStock = m_stockFund->ownedStock();
	//	int32_t index = -1;
	//	while (index++ != vecOwnedStock.size() - 1)
	//	{
	//		std::string& ownedStock = vecOwnedStock[index];
	//		const IntDateTime& firstBuyDate = m_stockFund->trade(ownedStock)[0].first;
	//		IntDateTime sellDatePre;
	//		auto itValidStock = validStock.find(ownedStock);
	//		if (itValidStock == validStock.end())
	//		{
	//			continue;
	//		}
	//		const StockMarket& stockMarket = itValidStock->second.first;
	//		if (!stockMarket.getDatePre(date, sellDatePre))
	//		{
	//			continue;
	//		}
	//		BigNumber chg = 0;
	//		if (!m_stockFund->stockChg(ownedStock, date, chg))
	//		{
	//			continue;
	//		}
	//		const BigNumber& upValue = stockMarket.upValue(date);
	//		const BigNumber& downValue = stockMarket.downValue(date);
	//		const BigNumber& entityValue = stockMarket.entityValue(date);
	//		const BigNumber& wr10 = validStock.find(ownedStock)->second.second.wr10(date);
	//		if (sellDatePre == firstBuyDate)
	//		{
	//			if (!(stockMarket.entityValue(date) > stockMarket.entityValue(sellDatePre).toPrec(6) * 2 / 3))
	//			{
	//				vecStockHandle.push_back(StockHandle(false, ownedStock, 0, validStock.find(ownedStock)->second.first.close(date), 1));
	//				BigNumber chg = 0;
	//				m_stockFund->stockChg(ownedStock, date, chg);
	//			}
	//		}
	//		if ((!(sellDatePre == firstBuyDate && stockMarket.chgValue(date) < 0) &&
	//			(chg > 0 && upValue > entityValue / 2.0)) ||
	//			(wr10 < 10 && chg > 0 && stockMarket.chgValue(date) < 9.9))
	//		{
	//			vecStockHandle.push_back(StockHandle(false, ownedStock, 0, validStock.find(ownedStock)->second.first.close(date), 1));
	//			BigNumber chg = 0;
	//			m_stockFund->stockChg(ownedStock, date, chg);
	//		}
	//	}
	//}
	//
	//
	//if (m_stockFund->stockNum() == 1 && mapChooseStock.find(m_stockFund->ownedStock()[0]) != mapChooseStock.end())
	//{
	//	vecStockHandle.push_back(StockHandle(true, m_stockFund->ownedStock()[0], 0, validStock.find(m_stockFund->ownedStock()[0])->second.first.close(date), 1));
	//}
	//else if (m_stockFund->stockNum() == 0)
	//{
	//	std::string chooseStock;
	//	BigNumber rsiCompare = 100;
	//	for (auto itChooseStock = mapChooseStock.begin(); itChooseStock != mapChooseStock.end(); ++itChooseStock)
	//	{
	//		if (itChooseStock->second < rsiCompare)
	//		{
	//			rsiCompare = itChooseStock->second;
	//			chooseStock = itChooseStock->first;
	//		}
	//	}
	//	if (!chooseStock.empty())
	//	{
	//		vecStockHandle.push_back(StockHandle(true, chooseStock, 0, validStock.find(chooseStock)->second.first.close(date), 0.5));
	//	}
	//}
}
