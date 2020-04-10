#include "SarRiseBackCount.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndicator/StockIndicatorAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "SarRiseBackInfo.h"

SarRiseBackCount::SarRiseBackCount()
{
	m_strategyType = SAR_RISE_BACK_COUNT;
}

bool SarRiseBackCount::buy(const IntDateTime& date, StockInfo& stockInfo)
{
	const std::shared_ptr<SarRiseBackInfo>& sarRiseBackInfo = std::dynamic_pointer_cast<SarRiseBackInfo>(m_strategyInfo);

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

	StockSar::SarState sar5State = spStockSarIndicator->day(date)->m_sarState5;
	StockSar::SarState sar10State = spStockSarIndicator->day(date)->m_sarState10;
	StockSar::SarState sar20State = spStockSarIndicator->day(date)->m_sarState20;

	if (sar5State == StockSar::RED_TO_GREEN && sar10State == StockSar::RED_TO_GREEN)
	{
		stockInfo.m_percent = 100;
		stockInfo.m_score = 100;
		return true;
	}
	else if (sar5State == StockSar::RED_TO_GREEN || sar10State == StockSar::RED_TO_GREEN || sar20State == StockSar::RED_TO_GREEN)
	{
		stockInfo.m_percent = 100;
		stockInfo.m_score = 80;
		return true;
	}
	return false;
}