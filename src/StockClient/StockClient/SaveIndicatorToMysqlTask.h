#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class SaveIndicatorToMysqlTask : public CTask
{
public:
	SaveIndicatorToMysqlTask();

public:
	void DoTask();

	void setParam(bool wr, bool rsi, StockClient* stockClient);

protected:
	bool m_wr;
	bool m_rsi;
	StockClient* m_stockClient;
};