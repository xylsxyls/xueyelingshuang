#include "LineBack.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LineBackInfo.h"
#include "StockFund/StockFundAPI.h"

LineBack::LineBack()
{
	m_strategyType = LINE_BACK;
}

bool LineBack::buy(const IntDateTime& date, StockInfo& stockInfo)
{
	const std::shared_ptr<LineBackInfo>& lineBackInfo = std::dynamic_pointer_cast<LineBackInfo>(m_strategyInfo);

	auto& stockFund = lineBackInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = lineBackInfo->m_spMarket;
	std::shared_ptr<StockSarIndicator>& spStockSarIndicator = std::dynamic_pointer_cast<StockSarIndicator>(lineBackInfo->m_spIndicator.find("sar")->second);
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = std::dynamic_pointer_cast<StockBollIndicator>(lineBackInfo->m_spIndicator.find("boll")->second);

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

	BigNumber amplitude = spMarket->day()->amplitudeValue();
	BigNumber thirtyLine = spMarket->thirtyLine();
	StockSar::SarState sar10State = spStockSarIndicator->day(spMarket->date())->m_sarState10;

	spMarket->previous();
	BigNumber previousClose = spMarket->day()->close();
	BigNumber previousAmplitude = spMarket->day()->amplitudeValue();
	BigNumber previousThirtyLine = spMarket->thirtyLine();
	StockSar::SarState previousSar10State = spStockSarIndicator->day(spMarket->date())->m_sarState10;
	spMarket->previous();
	BigNumber previousPreviousChg = spMarket->day()->chgValue();
	BigNumber previousPreviousClose = spMarket->day()->close();
	BigNumber previousPreviousAmplitude = spMarket->day()->amplitudeValue();
	BigNumber previousPreviousThirtyLine = spMarket->thirtyLine();
	spMarket->next();
	spMarket->next();

	if (previousPreviousChg < -2 &&
		previousPreviousAmplitude > 3 &&
		previousPreviousClose > previousPreviousThirtyLine &&
		//previousAmplitude < 3 &&
		previousSar10State == StockSar::GREEN &&
		previousClose > previousThirtyLine &&
		previousClose < previousThirtyLine * "1.03" &&
		//amplitude < 3 &&
		sar10State == StockSar::GREEN &&
		close > thirtyLine &&
		close < thirtyLine.toPrec(6) * "1.03")
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

bool LineBack::sell(const IntDateTime& date, StockInfo& stockInfo)
{
	const std::shared_ptr<LineBackInfo>& lineBackInfo = std::dynamic_pointer_cast<LineBackInfo>(m_strategyInfo);

	auto& stockFund = lineBackInfo->m_fund;
	std::shared_ptr<StockMarket>& spMarket = lineBackInfo->m_spMarket;
	std::string stock = spMarket->stock();
	std::shared_ptr<StockBollIndicator>& spStockBollIndicator = std::dynamic_pointer_cast<StockBollIndicator>(lineBackInfo->m_spIndicator.find("boll")->second);

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

	if (spMarket->getMemoryDays(firstBuyDate, date) >= 2)
	{
		stockInfo.m_price = close;
		stockInfo.m_percent = 100;
		stockInfo.m_score = 100;
		return true;
	}

	return result;
}