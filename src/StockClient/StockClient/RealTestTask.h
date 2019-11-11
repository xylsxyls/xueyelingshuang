#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockStrategy/StockStrategyAPI.h"

class StockClient;

class RealTestTask : public CTask
{
public:
	RealTestTask();

public:
	void DoTask();

	void setParam(StrategyEnum strategyEnum,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		StockClient* stockClient);

private:
	StrategyEnum m_strategyEnum;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
	StockClient* m_stockClient;
};