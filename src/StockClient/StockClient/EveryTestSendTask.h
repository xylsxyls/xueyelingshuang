#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class EveryTestSendTask : public CTask
{
public:
	EveryTestSendTask();

	void DoTask();

	void setParam(StockClient* stockClient);

protected:
	StockClient* m_stockClient;
};