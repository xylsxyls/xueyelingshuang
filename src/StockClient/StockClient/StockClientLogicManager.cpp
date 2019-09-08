#include "StockClientLogicManager.h"
#include "CSystem/CSystemAPI.h"

StockClientLogicManager::StockClientLogicManager()
{
	init();
}

StockClientLogicManager& StockClientLogicManager::instance()
{
	static StockClientLogicManager s_stockClientLogicManager;
	return s_stockClientLogicManager;
}

std::string StockClientLogicManager::tonghuashunPath()
{
	return "C:\\同花顺软件\\同花顺\\hexin.exe";
}

void StockClientLogicManager::init()
{

}