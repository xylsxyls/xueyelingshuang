#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockMarket;
class AssignCalcTask : public CTask
{
public:
	AssignCalcTask();

public:
	void DoTask();

	void setParam(uint32_t threadId,
		const std::shared_ptr<StockMarket>& spMarket,
		const IntDateTime& date,
		std::map<std::string, std::vector<std::vector<std::string>>>* wrIndicatorData,
		std::map<std::string, std::vector<std::vector<std::string>>>* rsiIndicatorData,
		std::map<std::string, std::vector<std::vector<std::string>>>* sarIndicatorData,
		std::map<std::string, std::vector<std::vector<std::string>>>* bollIndicatorData);

private:
	uint32_t m_threadId;
	std::shared_ptr<StockMarket> m_spMarket;
	IntDateTime m_date;
	std::map<std::string, std::vector<std::vector<std::string>>>* m_wrIndicatorData;
	std::map<std::string, std::vector<std::vector<std::string>>>* m_rsiIndicatorData;
	std::map<std::string, std::vector<std::vector<std::string>>>* m_sarIndicatorData;
	std::map<std::string, std::vector<std::vector<std::string>>>* m_bollIndicatorData;
};