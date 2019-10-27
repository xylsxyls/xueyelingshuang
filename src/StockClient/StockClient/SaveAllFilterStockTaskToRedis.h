#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockClient;

class SaveAllFilterStockTaskToRedis : public CTask
{
public:
	SaveAllFilterStockTaskToRedis();

public:
	void DoTask();

	void setParam(IntDateTime* beginTime, IntDateTime* endTime, StockClient* stockClient);

private:
	IntDateTime* m_beginTime;
	IntDateTime* m_endTime;
	StockClient* m_stockClient;
};