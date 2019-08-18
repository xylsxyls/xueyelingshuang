#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class SaveAllStockTask : public CTask
{
public:
	SaveAllStockTask();

public:
	void DoTask();

	void setParam(StockClient* stockClient);

protected:
	StockClient* m_stockClient;
};