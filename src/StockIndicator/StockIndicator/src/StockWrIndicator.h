#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockIndicatorMacro.h"
#include <memory>
#include <map>

class StockWr;
class StockIndicatorAPI StockWrIndicator
{
public:
	StockWrIndicator();

public:
	void setRedisData(const std::string& stock, const std::vector<std::vector<std::string>>& vecIndicator);
	void load();

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
	std::vector<std::vector<std::string>> m_vecRedisIndicator;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};