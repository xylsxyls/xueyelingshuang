#include "SaveIndicatorToMysqlTask.h"
#include "StockClientLogicManager.h"
#include "StockIndicator/StockIndicatorAPI.h"

SaveIndicatorToMysqlTask::SaveIndicatorToMysqlTask():
m_stockClient(nullptr),
m_wr(false),
m_rsi(false),
m_sar(false),
m_boll(false)
{

}

void SaveIndicatorToMysqlTask::DoTask()
{
	if (m_wr)
	{
		StockIndicator::instance().saveWr();
	}
	if (m_rsi)
	{
		StockIndicator::instance().saveRsi();
	}
	if (m_sar)
	{
		StockIndicator::instance().saveSar();
	}
	if (m_boll)
	{
		StockIndicator::instance().saveBoll();
	}
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("所有zhibiao已入库"));
}

void SaveIndicatorToMysqlTask::setParam(bool wr, bool rsi, bool sar, bool boll, StockClient* stockClient)
{
	m_wr = wr;
	m_rsi = rsi;
	m_sar = sar;
	m_boll = boll;
	m_stockClient = stockClient;
}