#include "StockIndex.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "StockMarket/StockMarketAPI.h"

BigNumber StockIndex::rsi(int32_t days, const IntDateTime& date, const std::shared_ptr<StockMarket>& spStockMarket)
{
	std::map<IntDateTime, std::vector<BigNumber>> preDaysData;
	if (!spStockMarket->getMarketPre(date, days + 1, preDaysData))
	{
		return -1;
	}
	BigNumber positiveNumber = 0;
	BigNumber negativeNumber = 0;
	auto itData = preDaysData.rbegin();
	while (days-- != 0)
	{
		BigNumber current = itData->second[StockMarket::CLOSE];
		BigNumber pre = (++itData)->second[StockMarket::CLOSE];
		BigNumber closeNode = current - pre;
		if (closeNode > 0)
		{
			positiveNumber = positiveNumber + closeNode;
		}
		else if(closeNode < 0)
		{
			negativeNumber = negativeNumber + closeNode;
		}
	}
	return (positiveNumber.setPrec(6) / (positiveNumber - negativeNumber) * 100).toPrec(2);
}

BigNumber StockIndex::wr(int32_t days, const IntDateTime& date, const std::shared_ptr<StockMarket>& spStockMarket)
{
	std::map<IntDateTime, std::vector<BigNumber>> preDaysData;
	if (!spStockMarket->getMarketPre(date, days, preDaysData))
	{
		return -1;
	}
	BigNumber high = 0;
	BigNumber low = 10000;
	BigNumber close = spStockMarket->close(date);
	auto itData = preDaysData.begin();
	while (days-- != 0)
	{
		BigNumber currentHigh = itData->second[StockMarket::HIGH];
		BigNumber currentLow = itData->second[StockMarket::LOW];
		if (currentHigh > high)
		{
			high = currentHigh;
		}
		if (currentLow < low)
		{
			low = currentLow;
		}
		++itData;
	}
	return ((high - close) / (high - low).toPrec(6).zero() * 100).toPrec(2);
}

void StockIndex::load(const std::map<IntDateTime, std::vector<BigNumber>>& data)
{
	m_stockIndex = data;
}

BigNumber StockIndex::rsi6(const IntDateTime& date) const
{
	auto itDate = m_stockIndex.find(date);
	if (itDate == m_stockIndex.end())
	{
		return -1;
	}
	return itDate->second[STOCK_RSI6];
}

BigNumber StockIndex::rsi12(const IntDateTime& date) const
{
	auto itDate = m_stockIndex.find(date);
	if (itDate == m_stockIndex.end())
	{
		return -1;
	}
	return itDate->second[STOCK_RSI12];
}

BigNumber StockIndex::rsi24(const IntDateTime& date) const
{
	auto itDate = m_stockIndex.find(date);
	if (itDate == m_stockIndex.end())
	{
		return -1;
	}
	return itDate->second[STOCK_RSI24];
}

BigNumber StockIndex::wr10(const IntDateTime& date) const
{
	auto itDate = m_stockIndex.find(date);
	if (itDate == m_stockIndex.end())
	{
		return -1;
	}
	return itDate->second[STOCK_WR10];
}

BigNumber StockIndex::wr20(const IntDateTime& date) const
{
	auto itDate = m_stockIndex.find(date);
	if (itDate == m_stockIndex.end())
	{
		return -1;
	}
	return itDate->second[STOCK_WR20];
}

BigNumber StockIndex::SMA(const BigNumber& X, const BigNumber& N, const BigNumber& M)
{
	if (N <= M)
	{
		return 0;
	}
	return (X * M + SMA(X, N - 1, M) * (N - M)) / N.toPrec(6);
}

BigNumber StockIndex::MAX(const BigNumber& X, const BigNumber& Y)
{
	return X > Y ? X : Y;
}

BigNumber StockIndex::ABS(const BigNumber& X)
{
	return (X < 0) ? (X * -1) : X;
}