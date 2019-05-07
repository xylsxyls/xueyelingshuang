#include "StockWrRsi.h"

std::vector<StockHandle> StockWrRsi::strategy(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock)
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

	std::vector<StockHandle> vecStockHandle;
	if (m_stockFund->stockNum() != 0)
	{
		std::vector<std::string> vecOwnedStock = m_stockFund->ownedStock();
		int32_t index = -1;
		while (index++ != vecOwnedStock.size() - 1)
		{
			const std::string& ownedStock = vecOwnedStock[index];
			auto itValidStock = validStock.find(ownedStock);
			if (itValidStock != validStock.end())
			{
				if (needSell(date, itValidStock->second.first, itValidStock->second.second))
				{
					vecStockHandle.push_back(StockHandle(false, ownedStock, 1, itValidStock->second.first->close(date), 1, 0));
				}
			}
		}
	}
	if (m_stockFund->hasFreezeFund())
	{
		std::vector<std::string> vecOwnedStock = m_stockFund->ownedStock();
		int32_t index = -1;
		while (index++ != vecOwnedStock.size() - 1)
		{
			const std::string& ownedStock = vecOwnedStock[index];
			auto itValidStock = validStock.find(ownedStock);
			if (itValidStock != validStock.end())
			{
				if (addBuy(date, itValidStock->second.first, itValidStock->second.second))
				{
					vecStockHandle.push_back(StockHandle(true, ownedStock, 0, itValidStock->second.first->close(date), 1, 1));
				}
			}
		}
	}
	else if (m_stockFund->hasAvailableFund())
	{
		std::vector<std::pair<std::string, BigNumber>> vecChooseStock = availBuyStock(date, validStock);
		if (!vecChooseStock.empty())
		{
			vecStockHandle.push_back(StockHandle(true, vecChooseStock[0].first, 1, vecChooseStock[0].second, m_stockFund->stockNum() == 0 ? 0.5 : 1, 0));
		}
	}
	return vecStockHandle;
}

std::vector<std::pair<std::string, BigNumber>> StockWrRsi::availBuyStock(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock)
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
	std::vector<std::pair<std::string, BigNumber>> result;
	std::map <BigNumber, std::vector<std::pair<std::string, BigNumber>>>  mapChooseStock;
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
		const BigNumber& close = spStockMarket->close(date);
		const BigNumber& wr10Pre = spStockIndex->wr10(preDate);
		const BigNumber& wr10 = spStockIndex->wr10(date);
		const BigNumber& rsi6 = spStockIndex->rsi6(date);
		const BigNumber& rsi24 = spStockIndex->rsi24(date);
		const BigNumber& downValue = spStockMarket->downValue(date);
		const BigNumber& entityValue = spStockMarket->entityValue(date);
		const BigNumber& chgValue = spStockMarket->chgValue(date);
		if ((wr10 > 95 && rsi24 < 45 && rsi24 > 30) &&
			(wr10Pre < wr10) &&
			(spStockMarket->low(date) < spStockMarket->low(preDate)) &&
			((entityValue != 0) && (downValue / entityValue.toPrec(4) < (1 / 4.0))) &&
			(chgValue < -2 || spStockMarket->chgValue(preDate) < -3) &&
			(close > 10) &&
			(!spStockMarket->isLimitDown(date)))
		//if ((wr10 > 80 && rsi24 < 50 && close > 10) &&
		//	//wr10Pre < wr10 &&
		//	chgValue > -9)
		{
			mapChooseStock[rsi24 - rsi6].push_back(std::pair<std::string, BigNumber>(stock, close));
		}
	}
	for (auto itChooseStock = mapChooseStock.rbegin(); itChooseStock != mapChooseStock.rend(); ++itChooseStock)
	{
		std::vector<std::pair<std::string, BigNumber>>& vecStock = itChooseStock->second;
		int32_t index = -1;
		while (index++ != vecStock.size() - 1)
		{
			result.push_back(vecStock[index]);
		}
	}
	return result;
}

bool StockWrRsi::addBuy(const IntDateTime& date, const std::shared_ptr<StockMarket>& spStockMarket, const std::shared_ptr<StockIndex>& spStockIndex)
{
	/**
	1.WR>90,RSI<45,RSI>30��
	2.ǰһ��WR�Ƚ��յͣ�
	3.������͵�����������ϸ�һ���㣬
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
	const BigNumber& wr10Pre = spStockIndex->wr10(preDate);
	const BigNumber& wr10 = spStockIndex->wr10(date);
	const BigNumber& rsi24 = spStockIndex->rsi24(date);
	if ((wr10 > 90 && rsi24 < 45 && rsi24 > 30) &&
		(wr10Pre < wr10) &&
		(spStockMarket->low(date) < spStockMarket->low(preDate) * 1.01))
	{
		return true;
	}
	return false;
}

bool StockWrRsi::needSell(const IntDateTime& date, const std::shared_ptr<StockMarket>& spStockMarket, const std::shared_ptr<StockIndex>& spStockIndex)
{
	return spStockIndex->wr10(date) < 50 && spStockMarket->chgValue(date) < 9.8;

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
