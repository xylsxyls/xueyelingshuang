#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class UpdateTodayMarketTask : public CTask
{
public:
	UpdateTodayMarketTask();

public:
	void DoTask();

	void setParam(StockClient* stockClient);

	virtual bool ReExecute();

	virtual CTask* Clone();

protected:
	StockClient* m_stockClient;
};