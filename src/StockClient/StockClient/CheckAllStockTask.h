#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class CheckAllStockTask : public CTask
{
public:
	CheckAllStockTask();

public:
	void DoTask();

	void setParam(StockClient* stockClient);

protected:
	StockClient* m_stockClient;
};