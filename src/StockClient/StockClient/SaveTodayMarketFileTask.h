#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class SaveTodayMarketFileTask : public CTask
{
public:
	SaveTodayMarketFileTask();

public:
	void DoTask();

	void setParam(StockClient* stockClient);

protected:
	StockClient* m_stockClient;
};