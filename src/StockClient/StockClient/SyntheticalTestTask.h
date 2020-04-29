#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"

class StockClient;

class SyntheticalTestTask : public CTask
{
public:
	SyntheticalTestTask();

public:
	void DoTask();

	void setParam(const BigNumber& beginValue,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		const std::vector<std::string>& allStock,
		StockClient* stockClient);

private:
	BigNumber m_beginValue;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
	std::vector<std::string> m_allStock;
	StockClient* m_stockClient;
};