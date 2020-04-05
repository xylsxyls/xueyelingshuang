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

	if (sar5State == StockSar::RED_TO_GREEN && sar10State == StockSar::RED_TO_GREEN)
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

bool SarRiseBack::sell(const IntDateTime& date,
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
		if (chg > "0.3")
		{
			BigNumber sell = ((bollup - bollmid) / "100.0" * 80 + bollmid).toPrec(2);
			if (spMarket->day()->high() > sell)
			{
				price = sell;
				percent = 100;
				score = 100;
				return true;
			}
		}
	}
	if (spMarket->getMemoryDays(firstBuyDate, date) == 3)
	{
		BigNumber secondChg;
		spMarket->previous();
		stockFund->stockChg(spMarket->stock(), spMarket->day(), secondChg);
		spMarket->next();
		if (secondChg < -3)
		{
			spMarket->previous();
			spMarket->previous();
			BigNumber buyPrice = spMarket->day()->close();
			spMarket->next();
			spMarket->next();
			if (spMarket->day()->high() / "1.03" > buyPrice)
			{
				price = (buyPrice * "1.03").toPrec(2);
				percent = 100;
				score = 100;
				return true;
			}
		}
	}

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
	if (spMarket->getMemoryDays(firstBuyDate, date) <= 3)
	{
		if (((bollup - bollmid) / "100.0" * 20 + bollmid) < close && chg > "0.3")
		{
			price = close;
			percent = 100;
			score = 100;
			return true;
		}
	}
	

	if (spMarket->getMemoryDays(firstBuyDate, date) >= 5)
	{
		price = spMarket->day()->close();
		percent = 100;
		score = 100;
		return true;
	}

	return result;
}

bool SarRiseBack::observeSell(const IntDateTime& date,
	BigNumber& price,
	BigNumber& percent,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	const std::shared_ptr<SarRiseBackInfo>& sarRiseBackInfo = std::dynamic_pointer_cast<SarRiseBackInfo>(strategyInfo);

	auto& stockFund = sarRiseBackInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = sarRiseBackInfo->m_spMarket;
	std::string stock = spMarket->stock();
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = std::dynamic_pointer_cast<StockBollIndicator>(sarRiseBackInfo->m_spIndicator.find("boll")->second);

	if (!spMarket->setDate(date))
	{
		return false;
	}

	IntDateTime firstBuyDate = stockFund->firstBuyDate(stock);
	int32_t holdDays = spMarket->getMemoryDays(firstBuyDate, date);
	BigNumber open = spMarket->day()->open();
	BigNumber high = spMarket->day()->high();
	BigNumber low = spMarket->day()->low();
	BigNumber close = spMarket->day()->close();
	BigNumber chg = spMarket->day()->chgValue();
	BigNumber bollup = spStockBollIndicator->day(date)->m_up;
	BigNumber bollmid = spStockBollIndicator->day(date)->m_mid;
	BigNumber costChg = "0.3";
	BigNumber bigChg = "3";
	spMarket->setDate(firstBuyDate);
	BigNumber buyPrice = spMarket->day()->close();
	BigNumber costPrice = (buyPrice * (1 + costChg / 100)).toPrec(2);
	spMarket->next();
	BigNumber firstDayChg = spMarket->day()->chgValue();
	spMarket->setDate(date);

	bool result = false;
	BigNumber allChg = 0;
	stockFund->stockChg(stock, spMarket->day(), allChg);

	if (spMarket->day()->isLimitUp())
	{
		return false;
	}

	if (spMarket->day()->isLimitDown())
	{
		return false;
	}

	if (holdDays == 3)
	{
		if (allChg > 12)
		{
			price = close;
			percent = 100;
			score = 100;
			return true;
		}
	}
	else if (holdDays == 4)
	{
		if (allChg > costChg)
		{
			if (firstDayChg > bigChg && chg > bigChg)
			{
				return false;
			}
			spMarket->setDate(firstBuyDate);
			spMarket->next();
			BigNumber secondDayChg = spMarket->day()->chgValue();
			BigNumber secondDayOpen = spMarket->day()->open();
			BigNumber secondDayClose = spMarket->day()->close();
			if (secondDayChg > bigChg && secondDayClose > open && secondDayOpen < close)
			{
				spMarket->setDate(date);
				return false;
			}

			spMarket->setDate(date);
			price = close;
			percent = 100;
			score = 100;
			return true;
		}
	}
	else if (holdDays == 5)
	{
		spMarket->previous();
		BigNumber threeDaysChg;
		stockFund->stockChg(stock, spMarket->day(), threeDaysChg);
		//前3天在有yingli的情况下
		if (threeDaysChg > costChg)
		{
			//第四天zuidi低于chengbenxian
			if (low < costPrice && high > costPrice)
			{
				spMarket->setDate(date);
				price = costPrice;
				percent = 100;
				score = 100;
				return true;
			}
			if (chg > 0 || allChg > costChg)
			{
				spMarket->setDate(date);
				price = close;
				percent = 100;
				score = 100;
				return true;
			}
		}
		//前三天kuiben但是今天shangzhang
		else if (chg > 0)
		{
			//kaipan就huiben
			if (open > costPrice)
			{
				spMarket->setDate(date);
				price = open;
				percent = 100;
				score = 100;
				return true;
			}
			//zhongtuhuiben
			else if (high > costPrice)
			{
				spMarket->setDate(date);
				price = costPrice;
				percent = 100;
				score = 100;
				return true;
			}
			//前三天kuiben但是今天shangzhang没huiben就geroumai
			spMarket->setDate(date);
			price = close;
			percent = 100;
			score = 100;
			return true;
		}
	}
	else if (holdDays >= 6)
	{
		spMarket->previous();
		BigNumber fourDaysChg;
		stockFund->stockChg(stock, spMarket->day(), fourDaysChg);
		//前4天kuiben
		if (fourDaysChg < costChg)
		{
			//kaipan回本kaipanmai
			if (open > costPrice)
			{
				spMarket->setDate(date);
				price = open;
				percent = 100;
				score = 100;
				return true;
			}
			//zhongtuhuibenzhongtumai
			if (high > costPrice)
			{
				spMarket->setDate(date);
				price = costPrice;
				percent = 100;
				score = 100;
				return true;
			}
		}
		//yingli
		else
		{
			//zhongtuhuiben
			if (low < costPrice && high > costPrice)
			{
				spMarket->setDate(date);
				price = costPrice;
				percent = 100;
				score = 100;
				return true;
			}
		}
		//第五天一定qingcang
		spMarket->setDate(date);
		price = close;
		percent = 100;
		score = 100;
		return true;
	}

	return false;
}

std::set<std::string> SarRiseBack::needIndicator()
{
	std::set<std::string> result;
	result.insert("sar");
	result.insert("boll");
	return result;
}