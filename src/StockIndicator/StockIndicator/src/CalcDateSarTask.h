#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockMarket;
class CalcDateSarTask : public CTask
{
public:
	CalcDateSarTask();

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