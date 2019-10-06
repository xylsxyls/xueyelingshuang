#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;

class UpdateTodayToMemoryTask : public CTask
{
public:
	UpdateTodayToMemoryTask();

public:
	void DoTask();

	void setParam(StockClient* stockClient);

private:
	StockClient* m_stockClient;
};