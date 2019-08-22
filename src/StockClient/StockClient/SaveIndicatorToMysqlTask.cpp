#include "SaveIndicatorToMysqlTask.h"
#include "StockClientLogicManager.h"
#include "StockIndicator/StockIndicatorAPI.h"

SaveIndicatorToMysqlTask::SaveIndicatorToMysqlTask():
m_stockClient(nullptr)
{

}

void SaveIndicatorToMysqlTask::DoTask()
{
	StockIndicator::instance().saveWr();
	StockIndicator::instance().saveRsi();
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("����zhibiao�����"));
}

void SaveIndicatorToMysqlTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}