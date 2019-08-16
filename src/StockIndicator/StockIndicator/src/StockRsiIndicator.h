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
	StockRsiIndicator();

public:
	void setRedisData(const std::string& stock, const std::vector<std::vector<std::string>>& vecIndicator);
	void load();

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
	std::vector<std::vector<std::string>> m_vecRedisIndicator;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};