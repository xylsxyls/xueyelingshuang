#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class SaveIndicatorToMysqlTask : public CTask
{
public:
	SaveIndicatorToMysqlTask();

public:
	void DoTask();

	void setParam(bool wr, bool rsi, bool sar, bool boll, StockClient* stockClient);

protected:
	bool m_wr;
	bool m_rsi;
	bool m_sar;
	bool m_boll;
	StockClient* m_stockClient;
};