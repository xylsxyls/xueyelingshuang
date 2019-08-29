#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

class StockClient;
class MairubishengTask : public CTask
{
public:
	MairubishengTask();

public:
	void DoTask();

	void setParam(int32_t sleepTime, StockClient* stockClient);

	void StopTask();

private:
	int32_t m_sleepTime;
	StockClient* m_stockClient;
	std::atomic<bool> m_exit;
};