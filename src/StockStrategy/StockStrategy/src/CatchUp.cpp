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

bool CatchUp::buy(const IntDateTime& date,
	BigNumber& price,
	BigNumber& percent,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	const std::shared_ptr<CatchUpInfo>& catchUpInfo = std::dynamic_pointer_cast<CatchUpInfo>(strategyInfo);

	auto& stockFund = catchUpInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = catchUpInfo->m_spMarket;
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = catchUpInfo->m_spBollIndicator;

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
		price = close;
		percent = 100;
		score = 200 * (bollUp / bollMid.toPrec(6) - 1).toPrec(6);
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

bool CatchUp::sell(const IntDateTime& date,
	BigNumber& price,
	BigNumber& percent,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	const std::shared_ptr<CatchUpInfo>& catchUpInfo = std::dynamic_pointer_cast<CatchUpInfo>(strategyInfo);

	auto& stockFund = catchUpInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = catchUpInfo->m_spMarket;
	std::string stock = spMarket->stock();
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = catchUpInfo->m_spBollIndicator;

	if (!spMarket->setDate(date))
	{
		return false;
	}

	IntDateTime firstBuyDate = stockFund->firstBuyDate(stock);
	BigNumber high = spMarket->day()->high();
	BigNumber close = spMarket->day()->close();
	BigNumber bollup = spStockBollIndicator->day(date)->m_up;
	BigNumber bollmid = spStockBollIndicator->day(date)->m_mid;

	bool result = false;
	BigNumber chg = 0;
	stockFund->stockChg(stock, spMarket->day(), chg);

	if (spMarket->day()->chgValue() > 9.5)
	{
		return false;
	}

	if (spMarket->day()->isLimitDown())
	{
		return false;
	}

	if (spMarket->getMemoryDays(firstBuyDate, date) >= 3)
	{
		price = close;
		percent = 100;
		score = 100;
		return true;
	}

	return result;
}