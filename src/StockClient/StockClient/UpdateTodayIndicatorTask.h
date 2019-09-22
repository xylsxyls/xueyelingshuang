#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class UpdateTodayIndicatorTask : public CTask
{
public:
	UpdateTodayIndicatorTask();

public:
	void DoTask();

	void setParam(StockClient* stockClient);

protected:
	StockClient* m_stockClient;
};