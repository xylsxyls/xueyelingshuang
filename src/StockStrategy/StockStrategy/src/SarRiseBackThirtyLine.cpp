#include "SarRiseBackThirtyLine.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SarRiseBackInfo.h"
#include "StockFund/StockFundAPI.h"

SarRiseBackThirtyLine::SarRiseBackThirtyLine()
{
	m_strategyType = SAR_RISE_BACK_THIRTY_LINE;
}

bool SarRiseBackThirtyLine::buy(const IntDateTime& date,
	BigNumber& price,
	BigNumber& percent,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	const std::shared_ptr<SarRiseBackInfo>& sarRiseBackInfo = std::dynamic_pointer_cast<SarRiseBackInfo>(strategyInfo);

	auto& stockFund = sarRiseBackInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = sarRiseBackInfo->m_spMarket;
	std::shared_ptr<StockSarIndicator>& spStockSarIndicator = std::dynamic_pointer_cast<StockSarIndicator>(sarRiseBackInfo->m_spIndicator.find("sar")->second);
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = std::dynamic_pointer_cast<StockBollIndicator>(sarRiseBackInfo->m_spIndicator.find("boll")->second);
	
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
	//BigNumber bollUpMid = (bollUp + bollMid) / "2.0";
	//BigNumber bollDownMid = (bollDown + bollMid) / "2.0";
	
	BigNumber sar5 = spStockSarIndicator->day(date)->m_sar5;
	BigNumber sar10 = spStockSarIndicator->day(date)->m_sar10;
	BigNumber sar20 = spStockSarIndicator->day(date)->m_sar20;

	StockSar::SarState sar5State = spStockSarIndicator->day(date)->m_sarState5;
	StockSar::SarState sar10State = spStockSarIndicator->day(date)->m_sarState10;
	StockSar::SarState sar20State = spStockSarIndicator->day(date)->m_sarState20;

	spMarket->previous();
	StockSar::SarState previousSar5State = spStockSarIndicator->day(spMarket->date())->m_sarState5;
	StockSar::SarState previousSar10State = spStockSarIndicator->day(spMarket->date())->m_sarState10;
	StockSar::SarState previousSar20State = spStockSarIndicator->day(spMarket->date())->m_sarState20;
	spMarket->next();

	bool though = false;

	if (close / spMarket->thirtyLine().toPrec(6).zero() < "1.02" && close > spMarket->thirtyLine())
	{
		though = true;
	}

	if (!though)
	{
		return false;
	}

	if (sar5State == StockSar::RED_TO_GREEN || sar10State == StockSar::RED_TO_GREEN || sar20State == StockSar::RED_TO_GREEN)
	{
		//if (close >= bollUpMid)
		//{
		//	return false;
		//}
		if (close > bollMid && close < ((bollUp - bollMid) / "100.0" * 50 + bollMid))
		{
			price = close;
			percent = 100;
			score = 200 * (bollUp / close.toPrec(6) - 1).toPrec(6);
			return true;
		}
		//else if (close >= bollDownMid && close <= bollMid)
		//{
		//	return false;
		//}
		else if (close > bollDown && close < ((bollMid - bollDown) / "100.0" * 50 + bollDown))
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

bool SarRiseBackThirtyLine::sell(const IntDateTime& date,
	BigNumber& price,
	BigNumber& percent,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	const std::shared_ptr<SarRiseBackInfo>& sarRiseBackInfo = std::dynamic_pointer_cast<SarRiseBackInfo>(strategyInfo);

	auto& stockFund = sarRiseBackInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = sarRiseBackInfo->m_spMarket;
	std::shared_ptr<StockSarIndicator>& spStockSarIndicator = std::dynamic_pointer_cast<StockSarIndicator>(sarRiseBackInfo->m_spIndicator.find("sar")->second);
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = std::dynamic_pointer_cast<StockBollIndicator>(sarRiseBackInfo->m_spIndicator.find("boll")->second);

	if (!spMarket->setDate(date))
	{
		return false;
	}

	IntDateTime firstBuyDate = stockFund->firstBuyDate(spMarket->stock());

	BigNumber high = spMarket->day()->high();
	BigNumber close = spMarket->day()->close();
	BigNumber bollup = spStockBollIndicator->day(date)->m_up;
	BigNumber bollmid = spStockBollIndicator->day(date)->m_mid;

	bool result = false;
	BigNumber chg = 0;
	stockFund->stockChg(spMarket->stock(), spMarket->day(), chg);
	if (spMarket->getMemoryDays(firstBuyDate, date) == 2)
	{
		//if (chg > "0.3")
		//{
		//	BigNumber sell = ((bollup - bollmid) / "100.0" * 80 + bollmid).toPrec(2);
		//	if (spMarket->day()->high() > sell)
		//	{
		//		price = sell;
		//		percent = 100;
		//		score = 100;
		//		return true;
		//	}
		//}
	}
	//if (spMarket->getMemoryDays(firstBuyDate, date) == 3)
	//{
	//	BigNumber secondChg;
	//	spMarket->previous();
	//	stockFund->stockChg(spMarket->stock(), spMarket->day(), secondChg);
	//	spMarket->next();
	//	if (secondChg < -3)
	//	{
	//		spMarket->previous();
	//		spMarket->previous();
	//		BigNumber buyPrice = spMarket->day()->close();
	//		spMarket->next();
	//		spMarket->next();
	//		if (spMarket->day()->high() / "1.03" > buyPrice)
	//		{
	//			price = (buyPrice * "1.03").toPrec(2);
	//			percent = 100;
	//			score = 100;
	//			return true;
	//		}
	//	}
	//}

	if (chg > "100")
	{
		price = close;
		percent = 100;
		score = 90;
		result = false;
	}

	//if (spMarket->getMemoryDays(firstBuyDate, date) == 4)
	//{
	//	BigNumber chg = 0;
	//	if (stockFund->stockChg(spMarket->stock(), spMarket->day(), chg) && chg > "0.2")
	//	{
	//		price = close;
	//		percent = 100;
	//		score = 100;
	//		return true;
	//	}
	//}

	if (bollup <= high)
	{
		price = bollup;
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

	//4 * 1
	//4 * 2
	if (spMarket->getMemoryDays(firstBuyDate, date) >= 2)
	{
		if (((bollup - bollmid) / "100.0" * 75 + bollmid) < close && chg > "0.3")
		{
			price = close;
			percent = 100;
			score = 100;
			return true;
		}
	}
	

	if (spMarket->getMemoryDays(firstBuyDate, date) >= 15)
	{
		price = spMarket->day()->close();
		percent = 100;
		score = 100;
		return true;
	}

	return result;
}

std::set<std::string> SarRiseBackThirtyLine::needIndicator()
{
	std::set<std::string> result;
	result.insert("sar");
	result.insert("boll");
	return result;
}