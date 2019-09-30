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
		mapRsi[stockMarket.date()] = (++dateIndex >= days ? (riseAvg / allAvg.zero() * 100).toPrec(2) : BigNumber(100).toPrec(2));
	}
	return mapRsi;
}

std::map<IntDateTime, std::pair<BigNumber, int32_t>> StockIndicatorHelper::sar(StockMarket& stockMarket,
	int32_t cycle,
	const BigNumber& afBegin,
	const BigNumber& afSpeed,
	const BigNumber& afMax)
{
	std::map<IntDateTime, std::pair<BigNumber, int32_t>> mapSar;
	if (stockMarket.empty())
	{
		return mapSar;
	}
	stockMarket.setFirstDate();
	BigNumber open = stockMarket.day()->open();
	BigNumber highest = stockMarket.day()->high();
	BigNumber lowest = stockMarket.day()->low();
	int32_t state = 0;
	BigNumber af = afBegin;
	BigNumber lastSar = 0;
	int32_t dateNum = 0;
	do 
	{
		++dateNum;
		IntDateTime date = stockMarket.date();
		
		if (dateNum <= cycle)
		{
			mapSar[date] = std::pair<BigNumber, int32_t>(-1, 0);
			goto END;
		}
		if (dateNum == cycle + 1)
		{
			if (stockMarket.day()->close() > open)
			{
				state = 1;
				if (stockMarket.day()->low() <= lowest)
				{
					state = 3;
				}
				lastSar = lowest;
				mapSar[date] = std::pair<BigNumber, int32_t>(lowest.toPrec(2), state);
			}
			else
			{
				state = 2;
				if (stockMarket.day()->high() >= highest)
				{
					state = 4;
				}
				lastSar = highest;
				mapSar[date] = std::pair<BigNumber, int32_t>(highest.toPrec(2), state);
			}
			goto END;
		}

		if (state == 1)
		{
			lastSar = lastSar + af * (highest - lastSar);
			BigNumber high = stockMarket.day()->high();
			if (high > highest)
			{
				af = af + afSpeed;
				if (af > afMax)
				{
					af = afMax;
				}
			}
			if (stockMarket.day()->low() <= lastSar)
			{
				state = 3;
			}
		}
		else if (state == 2)
		{
			lastSar = lastSar + af * (lowest - lastSar);
			BigNumber low = stockMarket.day()->low();
			if (low < lowest)
			{
				af = af + afSpeed;
				if (af > afMax)
				{
					af = afMax;
				}
			}
			if (stockMarket.day()->high() >= lastSar)
			{
				state = 4;
			}
		}
		else if (state == 3)
		{
			lastSar = highest;
			af = afBegin;
			state = 2;
		}
		else if (state == 4)
		{
			lastSar = lowest;
			af = afBegin;
			state = 1;
		}
		mapSar[date] = std::pair<BigNumber, int32_t>(lastSar.toPrec(2), state);

		END:
		if (dateNum > 1)
		{
			highest = stockMarket.day()->high();
			lowest = stockMarket.day()->low();
			StockMarket calc = stockMarket;
			int32_t count = cycle;
			while (count-- != 0)
			{
				if (!calc.previous())
				{
					break;
				}
				BigNumber high = calc.day()->high();
				BigNumber low = calc.day()->low();
				if (high > highest)
				{
					highest = high;
				}
				if (low < lowest)
				{
					lowest = low;
				}
			}
		}
	} while (stockMarket.next());
	//SAR(Tn) = SAR(Tn - 1) + AF(Tn)*[EP(Tn - 1) - SAR(Tn - 1)]
	return mapSar;
}

std::pair < BigNumber, std::pair<BigNumber, BigNumber>> StockIndicatorHelper::boll(const IntDateTime& date,
	StockMarket& stockMarket,
	int32_t days,
	const BigNumber& expand)
{
	std::pair < BigNumber, std::pair<BigNumber, BigNumber>> result;
	result.first = -1;
	result.second.first = -1;
	result.second.second = -1;
	if (stockMarket.days() < days)
	{
		return result;
	}
	if (!stockMarket.setDate(date))
	{
		return result;
	}
	int32_t count = days - 1;
	BigNumber MA = 0;
	while (count-- != 0)
	{
		MA = MA + stockMarket.day()->close();
		if (!stockMarket.previous())
		{
			return result;
		}
	}
	MA = MA + stockMarket.day()->close();
	MA = MA / BigNumber(days).toPrec(6);
	stockMarket.setDate(date);
	BigNumber variance = 0;
	count = days;
	while (count-- != 0)
	{
		BigNumber dayExpand = stockMarket.day()->close() - MA;
		stockMarket.previous();
		variance = variance + dayExpand * dayExpand;
	}
	variance = variance / BigNumber(days).toPrec(16);
	variance = variance.pow("0.5") * expand;
	result.first = MA.toPrec(2);
	result.second.first = (MA + variance).toPrec(2);
	result.second.second = (MA - variance).toPrec(2);
	return result;
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