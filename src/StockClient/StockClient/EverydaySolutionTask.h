#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class EverydaySolutionTask : public CTask
{
public:
	EverydaySolutionTask();

public:
	void DoTask();

	void setParam(StockClient* stockClient);

private:
	StockClient* m_stockClient;
};