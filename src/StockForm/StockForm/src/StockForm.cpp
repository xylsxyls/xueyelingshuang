#include "StockForm.h"

StockForm::StockForm():
m_spMarket(nullptr),
m_stockWrIndicator(nullptr),
m_stockRsiIndicator(nullptr),
m_stockSarIndicator(nullptr),
m_stockBollIndicator(nullptr)
{

}

void StockForm::init(const std::string& stock,
	const std::shared_ptr<StockMarket>& spMarket,
	const std::shared_ptr<StockWrIndicator>& stockWrIndicator,
	const std::shared_ptr<StockRsiIndicator>& stockRsiIndicator,
	const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
	const std::shared_ptr<StockBollIndicator>& stockBollIndicator)
{
	m_stock = stock;
	m_spMarket = spMarket;
	m_stockWrIndicator = stockWrIndicator;
	m_stockRsiIndicator = stockRsiIndicator;
	m_stockSarIndicator = stockSarIndicator;
	m_stockBollIndicator = stockBollIndicator;
}

bool StockForm::hasLongRise(const IntDateTime& date, int32_t preDays)
{
	return false;
}