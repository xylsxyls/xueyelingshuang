#include "StockWrRsi.h"

void StockWrRsi::strategy(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock, std::vector<StockHandle>& vecStockHandle)
{
	/** ���������������ֻƱ
	��
	1.WR>95,RSI<45,RSI>30��
	2.ǰһ��WR�Ƚ��յͣ�
	3.������͵���������ͣ�
	4.��Ӱ��С��ʵ���4��֮1��
	5.���յ�������2%С��9%�������յ�������3%��
	6.���̼�>10
	7.�ǵ�ͣ
	8.rsi24-rsi6�������
	�²�һ��
	�����ж����ʽ�Ļ����ǲ����߼�ֻ����ǰ3����������Ϊ��������ϸ�һ����
	����
	1.�ڶ����µ�������
	2.����̵�����3��֮2ʵ����ⶳ�ʽ𣬷���������
	3.������������Ƿ���2%��ʵ�峬3%�����ҵ�ǰ�������������������
	4.һ�����֮�������Ӱ�߳���ʵ��2��֮1�������µ�С��2%�����������������̼۴������̼�������������
	5.����µ���������ȡ����ǣ�
	6.WRС��10���������ҷ���ͣ����
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
	1.WR>95,RSI<45,RSI>30��
	2.ǰһ��WR�Ƚ��յͣ�
	3.������͵���������ͣ�
	4.��Ӱ��С��ʵ���4��֮1��
	5.���յ�������2%�������յ�������3%��
	6.���̼�>10
	7.�ǵ�ͣ
	8.rsi24-rsi6�������
	�²�һ��
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
	1.WR>90,RSI<45,RSI>30��
	2.ǰһ��WR�Ƚ��յͣ�
	3.������͵�����������ϸ�һ���㣬
	4.�ǵ�ͣ
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
