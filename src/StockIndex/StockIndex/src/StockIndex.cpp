#include "StockIndex.h"

BigNumber StockIndex::rsi(int32_t days, const IntDateTime& date, const StockMarket& stockMarket)
{
	//BigNumber X = MAX(stockMarket.close(date) - stockMarket.preClose(date), 0);
	//BigNumber N = days;
	//BigNumber M = 1;
	//BigNumber ssss = (M * X + (N - M) * (M * (0.01) / 1)) / N;
	//
	//X = ABS(stockMarket.close(date) - stockMarket.preClose(date));
	//N = days;
	//M = 1;
	//BigNumber ssss2 = (M * X + (N - M) * (M * (0.01) / 1)) / N;
	//return ssss / ssss2.toPrec(6) * 100;

	std::map<IntDateTime, std::vector<BigNumber>> preDaysData;
	if (!stockMarket.getMarketPre(date, days + 1, preDaysData))
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

BigNumber StockIndex::wr(int32_t days, const IntDateTime& date, const StockMarket& stockMarket)
{
	std::map<IntDateTime, std::vector<BigNumber>> preDaysData;
	if (!stockMarket.getMarketPre(date, days, preDaysData))
	{
		return -1;
	}
	BigNumber high = 0;
	BigNumber low = 10000;
	BigNumber close = stockMarket.close(date);
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