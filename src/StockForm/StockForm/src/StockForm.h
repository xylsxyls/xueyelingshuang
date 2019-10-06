#pragma once
#include "StockFormMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>

class StockMarket;
class StockWrIndicator;
class StockRsiIndicator;
class StockSarIndicator;
class StockBollIndicator;

class StockFormAPI StockForm
{
public:
	StockForm();

public:
	void init(const std::string& stock,
		const std::shared_ptr<StockMarket>& spMarket,
		const std::shared_ptr<StockWrIndicator>& stockWrIndicator,
		const std::shared_ptr<StockRsiIndicator>& stockRsiIndicator,
		const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
		const std::shared_ptr<StockBollIndicator>& stockBollIndicator);

	bool hasLongRise(const IntDateTime& date, int32_t preDays);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_stock;
	std::shared_ptr<StockMarket> m_spMarket;
	std::shared_ptr<StockWrIndicator> m_stockWrIndicator;
	std::shared_ptr<StockRsiIndicator> m_stockRsiIndicator;
	std::shared_ptr<StockSarIndicator> m_stockSarIndicator;
	std::shared_ptr<StockBollIndicator> m_stockBollIndicator;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};