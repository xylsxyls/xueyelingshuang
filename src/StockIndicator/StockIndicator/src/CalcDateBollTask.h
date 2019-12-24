#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockMarket/StockMarketAPI.h"

class CalcDateBollTask : public CTask
{
public:
	CalcDateBollTask();

public:
	void DoTask();

	void setParam(const IntDateTime& date,
		const StockMarket& market,
		std::map<std::string, std::vector<std::vector<std::string>>>* indicatorData);

private:
	IntDateTime m_date;
	StockMarket m_market;
	std::map<std::string, std::vector<std::vector<std::string>>>* m_indicatorData;
};