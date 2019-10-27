#include "SaveAllFilterStockTaskToRedis.h"
#include "StockMysql/StockMysqlAPI.h"
#include "StockClientLogicManager.h"

SaveAllFilterStockTaskToRedis::SaveAllFilterStockTaskToRedis() :
m_beginTime(nullptr),
m_endTime(nullptr),
m_stockClient(nullptr)
{

}

void SaveAllFilterStockTaskToRedis::DoTask()
{
	StockMysql::instance().saveFilterStockToRedis(*m_beginTime, *m_endTime);
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("����redis�������"));
}

void SaveAllFilterStockTaskToRedis::setParam(IntDateTime* beginTime, IntDateTime* endTime, StockClient* stockClient)
{
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stockClient = stockClient;
}