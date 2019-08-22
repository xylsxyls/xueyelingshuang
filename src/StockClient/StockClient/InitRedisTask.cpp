#include "InitRedisTask.h"
#include "StockClientLogicManager.h"
#include "StockMysql/StockMysqlAPI.h"

InitRedisTask::InitRedisTask():
m_stockClient(nullptr)
{

}

void InitRedisTask::DoTask()
{
	StockMysql::instance().initRedis();
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("redis初始化完成"));
}

void InitRedisTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}