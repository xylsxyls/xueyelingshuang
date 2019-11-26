#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockStrategy/StockStrategyAPI.h"

class Strategy;
class StockClient;

class EveryTestTask : public CTask
{
public:
	EveryTestTask();

	void DoTask();

	void setParam(StrategyType strategyType,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		StockClient* stockClient);

private:
	StrategyType m_strategyType;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
	StockClient* m_stockClient;
};