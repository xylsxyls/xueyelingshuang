#include "CatchUp.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CatchUpInfo.h"
#include "StockFund/StockFundAPI.h"

CatchUp::CatchUp()
{
	m_strategyType = CATCH_UP;
}

bool CatchUp::buy(const IntDateTime& date, StockInfo& stockInfo)
{
	const std::shared_ptr<CatchUpInfo>& catchUpInfo = std::dynamic_pointer_cast<CatchUpInfo>(m_strategyInfo);

	auto& stockFund = catchUpInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = catchUpInfo->m_spMarket;
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = std::dynamic_pointer_cast<StockBollIndicator>(catchUpInfo->m_spIndicator.find("boll")->second);

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

	BigNumber bollUp = spStockBollIndicator->day(date)->m_up;
	BigNumber bollMid = spStockBollIndicator->day(date)->m_mid;

	spMarket->previous();
	BigNumber previousOpen = spMarket->day()->open();
	BigNumber previousClose = spMarket->day()->close();
	BigNumber previousEntity = spMarket->day()->entityValue();
	BigNumber previousChg = spMarket->day()->chgValue();
	spMarket->previous();
	BigNumber previousPreviousChg = spMarket->day()->chgValue();
	spMarket->next();
	spMarket->next();
	BigNumber open = spMarket->day()->open();
	BigNumber entity = spMarket->day()->entityValue();
	BigNumber upValue = spMarket->day()->upValue();

	if (dayChg > 1 &&
		previousChg > -2 &&
		previousPreviousChg > 2 &&
		previousOpen > previousClose &&
		(open > previousClose && open < previousOpen) &&
		close > previousClose && close > previousOpen &&
		(entity / previousEntity.toPrec(6) - 1).abs() < "0.8" &&
		upValue / entity.toPrec(6) < "0.8")
	{
		stockInfo.m_price = close;
		stockInfo.m_percent = 100;
		stockInfo.m_score = 200 * (bollUp / bollMid.toPrec(6) - 1).toPrec(6);
		return true;
	}
	//if (dayChg < 1 &&
	//	previousPreviousChg > 2 &&
	//	previousClose > previousOpen &&
	//	(open > previousOpen && open < previousClose) &&
	//	close > previousClose &&
	//	(entity / previousEntity.toPrec(6) - 1).abs() < "0.8" &&
	//	upValue / entity.toPrec(6) < "0.8" &&
	//	open / previousOpen < "1.02")
	//{
	//	price = close;
	//	percent = 100;
	//	score = 100 * (bollUp / bollMid.toPrec(6) - 1).toPrec(6);
	//	return true;
	//}
	return false;
}

bool CatchUp::sell(const IntDateTime& date, StockInfo& stockInfo)
{
	const std::shared_ptr<CatchUpInfo>& catchUpInfo = std::dynamic_pointer_cast<CatchUpInfo>(m_strategyInfo);

	auto& stockFund = catchUpInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = catchUpInfo->m_spMarket;
	std::string stock = spMarket->stock();
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = std::dynamic_pointer_cast<StockBollIndicator>(catchUpInfo->m_spIndicator.find("boll")->second);

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
			stockInfo.m_price = close;
			stockInfo.m_percent = 100;
			stockInfo.m_score = 100;
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
			stockInfo.m_price = close;
			stockInfo.m_percent = 100;
			stockInfo.m_score = 100;
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
				stockInfo.m_price = costPrice;
				stockInfo.m_percent = 100;
				stockInfo.m_score = 100;
				return true;
			}
			if (chg > 0 || allChg > costChg)
			{
				spMarket->setDate(date);
				stockInfo.m_price = close;
				stockInfo.m_percent = 100;
				stockInfo.m_score = 100;
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
				stockInfo.m_price = open;
				stockInfo.m_percent = 100;
				stockInfo.m_score = 100;
				return true;
			}
			//zhongtuhuiben
			else if (high > costPrice)
			{
				spMarket->setDate(date);
				stockInfo.m_price = costPrice;
				stockInfo.m_percent = 100;
				stockInfo.m_score = 100;
				return true;
			}
			//前三天kuiben但是今天shangzhang没huiben就geroumai
			spMarket->setDate(date);
			stockInfo.m_price = close;
			stockInfo.m_percent = 100;
			stockInfo.m_score = 100;
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
				stockInfo.m_price = open;
				stockInfo.m_percent = 100;
				stockInfo.m_score = 100;
				return true;
			}
			//zhongtuhuibenzhongtumai
			if (high > costPrice)
			{
				spMarket->setDate(date);
				stockInfo.m_price = costPrice;
				stockInfo.m_percent = 100;
				stockInfo.m_score = 100;
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
				stockInfo.m_price = costPrice;
				stockInfo.m_percent = 100;
				stockInfo.m_score = 100;
				return true;
			}
		}
		//第五天一定qingcang
		spMarket->setDate(date);
		stockInfo.m_price = close;
		stockInfo.m_percent = 100;
		stockInfo.m_score = 100;
		return true;
	}

	return false;
}