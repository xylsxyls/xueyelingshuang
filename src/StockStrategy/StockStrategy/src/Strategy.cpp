#include "Strategy.h"

void Strategy::init(const std::shared_ptr<StockMarket>& spMarket)
{
	m_spMarket = spMarket;
}

std::shared_ptr<StockMarket> Strategy::market()
{
	return m_spMarket;
}