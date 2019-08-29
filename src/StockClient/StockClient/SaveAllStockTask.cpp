#include "SaveAllStockTask.h"
#include "StockClientLogicManager.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "StockClient.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMysql/StockMysqlAPI.h"

SaveAllStockTask::SaveAllStockTask():
m_stockClient(nullptr)
{

}

void SaveAllStockTask::DoTask()
{
	StockMysql::instance().saveAllStock(m_stockClient->m_todayMarket);
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("±£´æÍê³É"));
}

void SaveAllStockTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}