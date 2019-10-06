#include "SarRiseBack.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"

SarRiseBack::SarRiseBack():
m_isBuy(false),
m_buyDate(0, 0)
{

}

void SarRiseBack::init(const std::shared_ptr<StockMarket>& spMarket,
	const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
	const std::shared_ptr<StockBollIndicator>& stockBollIndicator)
{
	m_spMarket = spMarket;
	m_spStockSarIndicator = stockSarIndicator;
	m_spStockBollIndicator = stockBollIndicator;
}

void SarRiseBack::load()
{
	m_spMarket->load();
	m_spStockSarIndicator->load();
	m_spStockBollIndicator->load();
}

bool SarRiseBack::buy(const IntDateTime& date, BigNumber& price, BigNumber& percent)
{
	if (!m_spMarket->setDate(date))
	{
		return false;
	}
	BigNumber close = m_spMarket->day()->close();
	if (close < 10)
	{
		return false;
	}

	BigNumber dayChg = m_spMarket->day()->chgValue();
	if (dayChg < -9)
	{
		return false;
	}

	BigNumber sar5 = m_spStockSarIndicator->day(date)->m_sar5;
	BigNumber sar10 = m_spStockSarIndicator->day(date)->m_sar10;
	BigNumber sar20 = m_spStockSarIndicator->day(date)->m_sar20;

	StockSar::SarState sar5State = m_spStockSarIndicator->day(date)->m_sarState5;
	StockSar::SarState sar10State = m_spStockSarIndicator->day(date)->m_sarState10;
	StockSar::SarState sar20State = m_spStockSarIndicator->day(date)->m_sarState20;

	if (sar5State == StockSar::RED_TO_GREEN || sar10State == StockSar::RED_TO_GREEN || sar20State == StockSar::RED_TO_GREEN)
	{
		if (sar5State == StockSar::RED_TO_GREEN)
		{
			percent = 80;
			price = sar5;
		}
		else if (sar10State == StockSar::RED_TO_GREEN)
		{
			percent = 100;
			price = sar10;
		}
		else
		{
			percent = 90;
			price = sar20;
		}
		m_isBuy = true;
		m_buyDate = date;
		m_buyPrice = price;
		return true;
	}
	return false;
}

bool SarRiseBack::sell(const IntDateTime& date, BigNumber& price, BigNumber& percent)
{
	if (!m_spMarket->setDate(date))
	{
		return false;
	}
	if (m_spMarket->getMemoryDays(m_buyDate, date) >= 5)
	{
		BigNumber close = m_spMarket->day()->close();
		BigNumber chg = ((close / m_buyPrice.toPrec(2)) - 1) * 100;
		if (chg < 2)
		{
			price = close;
			percent = 100;
			m_isBuy = false;
			return true;
		}
	}
	if (m_spMarket->day()->chgValue() > 9.5)
	{
		return false;
	}
	BigNumber high = m_spMarket->day()->high();
	BigNumber bollup = m_spStockBollIndicator->day(date)->m_up;
	if (bollup <= high)
	{
		price = bollup;
		percent = 100;
		m_isBuy = false;
		return true;
	}
	return false;
}