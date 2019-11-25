#include "SarRiseBack.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SarRiseBackInfo.h"

SarRiseBack::SarRiseBack():
m_isBuy(false),
m_buyDate(0, 0)
{

}

bool SarRiseBack::buy(const IntDateTime& date,
	BigNumber& price,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	const std::shared_ptr<SarRiseBackInfo>& sarRiseBackInfo = std::dynamic_pointer_cast<SarRiseBackInfo>(strategyInfo);

	auto& buyInfo = sarRiseBackInfo->m_allBuyInfo;
	std::shared_ptr<StockMarket>& spMarket = sarRiseBackInfo->m_spMarket;
	std::shared_ptr<StockSarIndicator>& spStockSarIndicator = sarRiseBackInfo->m_spSarIndicator;
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = sarRiseBackInfo->m_spBollIndicator;
	
	if (!spMarket->setDate(date))
	{
		return false;
	}
	BigNumber close = spMarket->day()->close();
	if (close < 10)
	{
		return false;
	}

	BigNumber dayChg = spMarket->day()->chgValue();
	if (dayChg < -9)
	{
		return false;
	}

	std::string stock = spMarket->stock();
	if (CStringManager::Left(stock, 3) == "688")
	{
		return false;
	}

	BigNumber bollMid = spStockBollIndicator->day(date)->m_mid;
	BigNumber bollDown = spStockBollIndicator->day(date)->m_down;
	if (close >= (bollDown + bollMid) / 2 && close <= bollMid)
	{
		return false;
	}

	BigNumber sar5 = spStockSarIndicator->day(date)->m_sar5;
	BigNumber sar10 = spStockSarIndicator->day(date)->m_sar10;
	BigNumber sar20 = spStockSarIndicator->day(date)->m_sar20;

	StockSar::SarState sar5State = spStockSarIndicator->day(date)->m_sarState5;
	StockSar::SarState sar10State = spStockSarIndicator->day(date)->m_sarState10;
	StockSar::SarState sar20State = spStockSarIndicator->day(date)->m_sarState20;

	if (sar5State == StockSar::RED_TO_GREEN && sar10State == StockSar::RED_TO_GREEN/* || sar20State == StockSar::RED_TO_GREEN*/)
	{
		score = 33;
		price = close;
		m_isBuy = true;
		m_buyDate = date;
		m_buyPrice = price;
		return true;
		//if (sar10State == StockSar::RED_TO_GREEN)
		//{
		//	percent = 100;
		//	price = close;
		//}
		/*else */if (sar5State == StockSar::RED_TO_GREEN)
		{
			//return false;
			score = 10;
			price = close;
		}
		else if (sar10State == StockSar::RED_TO_GREEN)
		{
			//return false;
			score = 0;
			price = close;
		}
		else
		{
			score = 90;
			price = close;
		}
		m_isBuy = true;
		m_buyDate = date;
		m_buyPrice = price;
		return true;
	}
	return false;
}

bool SarRiseBack::sell(const IntDateTime& date,
	BigNumber& price,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	const std::shared_ptr<SarRiseBackInfo>& sarRiseBackInfo = std::dynamic_pointer_cast<SarRiseBackInfo>(strategyInfo);

	auto& buyInfo = sarRiseBackInfo->m_allBuyInfo;
	std::shared_ptr<StockMarket>& spMarket = sarRiseBackInfo->m_spMarket;
	std::shared_ptr<StockSarIndicator>& spStockSarIndicator = sarRiseBackInfo->m_spSarIndicator;
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = sarRiseBackInfo->m_spBollIndicator;

	if (!spMarket->setDate(date))
	{
		return false;
	}
	if (spMarket->getMemoryDays(m_buyDate, date) == 5)
	{
		BigNumber close = spMarket->day()->close();
		price = close;
		score = 100;
		m_isBuy = false;
		return true;
	}
	if (spMarket->day()->chgValue() > 9.5)
	{
		return false;
	}
	//if (m_spMarket->getMemoryDays(m_buyDate, date) >= 22)
	//{
	//	BigNumber close = m_spMarket->day()->close();
	//	price = close;
	//	percent = 100;
	//	m_isBuy = false;
	//	return true;
	//}
	BigNumber high = spMarket->day()->high();
	BigNumber close = spMarket->day()->close();
	BigNumber bollup = spStockBollIndicator->day(date)->m_up;
	BigNumber bollmid = spStockBollIndicator->day(date)->m_mid;
	if (bollup <= high || (bollmid + bollup) / 2 < close)
	{
		price = close;
		score = 100;
		m_isBuy = false;
		return true;
	}
	return false;
}