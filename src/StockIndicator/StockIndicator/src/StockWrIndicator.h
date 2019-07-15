#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockIndicatorMacro.h"
#include <memory>
#include <map>

class StockWr;
class StockMysql;
class StockIndicatorAPI StockWrIndicator
{
public:
	StockWrIndicator();

public:
	void load(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	std::shared_ptr<StockWr> day(const IntDateTime& date);

	void clear();

	bool empty();

protected:
	void calc();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_stock;
	std::map<IntDateTime, std::shared_ptr<StockWr>> m_indicator;
	std::shared_ptr<StockMysql> m_spStockMysql;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};