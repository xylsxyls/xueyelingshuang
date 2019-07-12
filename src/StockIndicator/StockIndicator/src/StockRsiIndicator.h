#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include <map>
#include "StockIndicatorMacro.h"

class StockRsi;
class StockIndicatorAPI StockRsiIndicator
{
public:
	void load(const std::string& stock,
		const IntDateTime& beginTime = IntDateTime(0, 0),
		const IntDateTime& endTime = IntDateTime(0, 0));

	std::shared_ptr<StockRsi> day(const IntDateTime& date);

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
	std::map<IntDateTime, std::shared_ptr<StockRsi>> m_indicator;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};