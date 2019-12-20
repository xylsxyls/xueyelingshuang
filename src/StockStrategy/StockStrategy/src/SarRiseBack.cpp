#include "SarRiseBack.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SarRiseBackInfo.h"
#include "StockFund/StockFundAPI.h"

SarRiseBack::SarRiseBack()
{
	m_strategyType = SAR_RISE_BACK;
}

bool SarRiseBack::buy(const IntDateTime& date,
	BigNumber& price,
	BigNumber& percent,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	const std::shared_ptr<SarRiseBackInfo>& sarRiseBackInfo = std::dynamic_pointer_cast<SarRiseBackInfo>(strategyInfo);

	auto& stockFund = sarRiseBackInfo->m_fund;
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

	BigNumber low = spMarket->day()->low();

	BigNumber bollUp = spStockBollIndicator->day(date)->m_up;
	BigNumber bollMid = spStockBollIndicator->day(date)->m_mid;
	BigNumber bollDown = spStockBollIndicator->day(date)->m_down;
	BigNumber bollUpMid = (bollUp + bollMid) / "2.0";
	BigNumber bollDownMid = (bollDown + bollMid) / "2.0";
	
	BigNumber sar5 = spStockSarIndicator->day(date)->m_sar5;
	BigNumber sar10 = spStockSarIndicator->day(date)->m_sar10;
	BigNumber sar20 = spStockSarIndicator->day(date)->m_sar20;

	StockSar::SarState sar5State = spStockSarIndicator->day(date)->m_sarState5;
	StockSar::SarState sar10State = spStockSarIndicator->day(date)->m_sarState10;
	StockSar::SarState sar20State = spStockSarIndicator->day(date)->m_sarState20;

	if (sar5State == StockSar::RED_TO_GREEN && sar10State == StockSar::RED_TO_GREEN)
	{
		if (close >= bollUpMid)
		{
			return false;
		}
		else if (close > bollMid && close < bollUpMid)
		{
			price = close;
			percent = 100;
			score = 200 * (bollUp / close.toPrec(6) - 1).toPrec(6);
			return true;
		}
		else if (close >= bollDownMid && close <= bollMid)
		{
			return false;
		}
		else if (close > bollDown && close < bollDownMid)
		{
			price = close;
			percent = 100;
			score = 100 * (bollMid / close.toPrec(6) - 1).toPrec(6);
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}

bool SarRiseBack::sell(const IntDateTime& date,
	BigNumber& price,
	BigNumber& percent,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	const std::shared_ptr<SarRiseBackInfo>& sarRiseBackInfo = std::dynamic_pointer_cast<SarRiseBackInfo>(strategyInfo);

	auto& stockFund = sarRiseBackInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = sarRiseBackInfo->m_spMarket;
	std::shared_ptr<StockSarIndicator>& spStockSarIndicator = sarRiseBackInfo->m_spSarIndicator;
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = sarRiseBackInfo->m_spBollIndicator;

	if (!spMarket->setDate(date))
	{
		return false;
	}

	IntDateTime firstBuyDate = stockFund->firstBuyDate(spMarket->stock());

	if (spMarket->getMemoryDays(firstBuyDate, date) == 5)
	{
		BigNumber close = spMarket->day()->close();
		price = close;
		percent = 100;
		score = 100;
		return true;
	}
	if (spMarket->day()->chgValue() > 9.5)
	{
		return false;
	}

	if (spMarket->day()->isLimitDown())
	{
		return false;
	}

	BigNumber high = spMarket->day()->high();
	BigNumber close = spMarket->day()->close();
	BigNumber bollup = spStockBollIndicator->day(date)->m_up;
	BigNumber bollmid = spStockBollIndicator->day(date)->m_mid;

	bool result = false;
	BigNumber chg = 0;
	if (stockFund->stockChg(spMarket->stock(), spMarket->day(), chg) && chg > "100")
	{
		price = close;
		percent = 100;
		score = 90;
		result = false;
	}

	if (spMarket->getMemoryDays(firstBuyDate, date) == 4)
	{
		BigNumber chg = 0;
		if (stockFund->stockChg(spMarket->stock(), spMarket->day(), chg) && chg > "0.2")
		{
			price = close;
			percent = 100;
			score = 100;
			return true;
		}
	}

	if (bollup <= high)
	{
		price = bollup;
		percent = 100;
		score = 100;
		return true;
	}

	if ((bollup - bollmid) / 4 * 1 + bollmid < close)
	{
		price = close;
		percent = 100;
		score = 100;
		return true;
	}

	return result;
}