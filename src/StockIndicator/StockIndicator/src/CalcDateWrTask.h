#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockMarket;
class CalcDateWrTask : public CTask
{
public:
	CalcDateWrTask();

public:
	void DoTask();

	void setParam(const IntDateTime& date,
		const std::shared_ptr<StockMarket>& spMarket,
		std::map<std::string, std::vector<std::vector<std::string>>>* indicatorData);

private:
	IntDateTime m_date;
	std::shared_ptr<StockMarket> m_spMarket;
	std::map<std::string, std::vector<std::vector<std::string>>>* m_indicatorData;
};