#include "StockIndicatorHelper.h"
#include "StockMarket/StockMarketAPI.h"

BigNumber StockIndicatorHelper::wr(int32_t days, const IntDateTime& date, StockMarket& spStockMarket)
{
	spStockMarket.setDate(date);
	BigNumber high = spStockMarket.day()->high();
	BigNumber low = spStockMarket.day()->low();
	BigNumber close = spStockMarket.day()->close();

	while (spStockMarket.previous())
	{
		--days;
		BigNumber currentHigh = spStockMarket.day()->high();
		BigNumber currentLow = spStockMarket.day()->low();
		if (currentHigh > high)
		{
			high = currentHigh;
		}
		if (currentLow < low)
		{
			low = currentLow;
		}
		if (days == 0)
		{
			return ((high - close) / (high - low).toPrec(6).zero() * 100).toPrec(2);
		}
	}
	return -1;
}

BigNumber StockIndicatorHelper::rsi(int32_t days, const IntDateTime& date, StockMarket& spStockMarket)
{
	spStockMarket.setDate(date);
	BigNumber positiveNumber = 0;
	BigNumber negativeNumber = 0;
	while (spStockMarket.previous())
	{
		--days;
		BigNumber current = spStockMarket.day()->close();
		BigNumber pre = spStockMarket.day()->preClose();
		BigNumber closeNode = current - pre;
		if (closeNode > 0)
		{
			positiveNumber = positiveNumber + closeNode;
		}
		else if (closeNode < 0)
		{
			negativeNumber = negativeNumber + closeNode;
		}
		if (days == 0)
		{
			return (positiveNumber.setPrec(6) / (positiveNumber - negativeNumber).zero() * 100).toPrec(2);
		}
	}
	return 100;
}

std::map<IntDateTime, BigNumber> StockIndicatorHelper::rsiTongHuaShun(int32_t days, StockMarket& stockMarket)
{
	std::map<IntDateTime, BigNumber> mapRsi;
	BigNumber riseAvg = 0;
	BigNumber allAvg = 0;
	stockMarket.setFirstDate();
	IntDateTime date = stockMarket.date();
	if (date.empty())
	{
		return mapRsi;
	}
	mapRsi[date] = BigNumber(100).toPrec(2);
	int32_t dateIndex = 1;
	while (stockMarket.next())
	{
		BigNumber riseFall = stockMarket.day()->riseFallValue();
		riseAvg = (riseAvg * (days - 1) + (riseFall > 0 ? riseFall : 0)).toPrec(16) / days;
		allAvg = (allAvg * (days - 1) + riseFall.abs()).toPrec(16) / days;
		mapRsi[date] = (++dateIndex >= days ? (riseAvg / allAvg.zero() * 100).toPrec(2) : BigNumber(100).toPrec(2));
	}
	return mapRsi;
}

BigNumber StockIndicatorHelper::SMA(const BigNumber& X, const BigNumber& N, const BigNumber& M)
{
	if (N <= M)
	{
		return 0;
	}
	return (X * M + SMA(X, N - 1, M) * (N - M)) / N.toPrec(6);
}

BigNumber StockIndicatorHelper::MAX(const BigNumber& X, const BigNumber& Y)
{
	return X > Y ? X : Y;
}

BigNumber StockIndicatorHelper::ABS(const BigNumber& X)
{
	return (X < 0) ? (X * -1) : X;
}