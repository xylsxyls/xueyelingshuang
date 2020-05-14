#include "T0.h"
#include "T0Info.h"
#include "StockFund/StockFundAPI.h"
#include "CStringManager/CStringManagerAPI.h"

T0::T0()
{
	m_strategyType = T_ADD_0;
}

bool T0::buy(const IntDateTime& date, StockInfo& stockInfo)
{
	const std::shared_ptr<T0Info>& t0Info = std::dynamic_pointer_cast<T0Info>(m_strategyInfo);
	std::shared_ptr<StockMarket>& spMarket = t0Info->m_spMarket;
	
	if (!spMarket->setDate(date))
	{
		return false;
	}
	
	std::string stock = spMarket->stock();
	if (CStringManager::Left(stock, 3) == "688")
	{
		return false;
	}
	BigNumber open = spMarket->day()->open();
	BigNumber high = spMarket->day()->high();
	BigNumber low = spMarket->day()->low();
	BigNumber close = spMarket->day()->close();

	int32_t hasBuySize = t0Info->m_fund->ownedStock().size();
	if (hasBuySize >= 1)
	{
		return false;
	}
	BigNumber buyPrice = (high * "0.995").toPrec(2);
	stockInfo.m_price = (buyPrice >= low ? buyPrice : close);
	stockInfo.m_percent = 100;
	stockInfo.m_score = 100;
	return true;
}

bool T0::sell(const IntDateTime& date, StockInfo& stockInfo)
{
	const std::shared_ptr<T0Info>& t0Info = std::dynamic_pointer_cast<T0Info>(m_strategyInfo);
	auto& stockFund = t0Info->m_fund;
	std::shared_ptr<StockMarket>& spMarket = t0Info->m_spMarket;
	std::string stock = spMarket->stock();

	if (!spMarket->setDate(date))
	{
		return false;
	}

	BigNumber high = spMarket->day()->high();
	stockInfo.m_price = high;
	stockInfo.m_percent = 100;
	stockInfo.m_score = 100;
	return true;
}

bool T0::observeSell(const IntDateTime& date, StockInfo& stockInfo)
{
	const std::shared_ptr<T0Info>& t0Info = std::dynamic_pointer_cast<T0Info>(m_strategyInfo);
	return sell(date, stockInfo);
}