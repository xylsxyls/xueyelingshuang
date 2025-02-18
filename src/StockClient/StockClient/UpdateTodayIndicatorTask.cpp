#include "UpdateTodayIndicatorTask.h"
#include "StockClient.h"
#include "StockClientLogicManager.h"
#include "StockIndicator/StockIndicatorAPI.h"

UpdateTodayIndicatorTask::UpdateTodayIndicatorTask():
m_stockClient(nullptr)
{

}

void UpdateTodayIndicatorTask::DoTask()
{
	StockIndicator::instance().saveDateIndicator(m_stockClient->m_today);
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("zhibiao�������"));
}

void UpdateTodayIndicatorTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}