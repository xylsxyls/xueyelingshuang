#include "SaveIndicatorToMysqlTask.h"
#include "StockClientLogicManager.h"
#include "StockIndicator/StockIndicatorAPI.h"

SaveIndicatorToMysqlTask::SaveIndicatorToMysqlTask():
m_stockClient(nullptr),
m_wr(false),
m_rsi(false)
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
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("所有zhibiao已入库"));
}

void SaveIndicatorToMysqlTask::setParam(bool wr, bool rsi, StockClient* stockClient)
{
	m_wr = wr;
	m_rsi = rsi;
	m_stockClient = stockClient;
}