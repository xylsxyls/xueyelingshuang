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
	StockIndicator::instance().saveDateWr(m_stockClient->m_today);
	StockIndicator::instance().saveDateRsi(m_stockClient->m_today);
	StockIndicator::instance().saveDateSar(m_stockClient->m_today);
	StockIndicator::instance().saveDateBoll(m_stockClient->m_today);
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("zhibiao更新完成"));
}

void UpdateTodayIndicatorTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}