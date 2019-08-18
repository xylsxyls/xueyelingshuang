#include "StockClientLogicManager.h"

StockClientLogicManager::StockClientLogicManager()
{
	init();
}

StockClientLogicManager& StockClientLogicManager::instance()
{
	static StockClientLogicManager s_stockClientLogicManager;
	return s_stockClientLogicManager;
}

void StockClientLogicManager::openTonghuashun()
{
	ShellExecute(NULL, L"open", L"C:\\同花顺软件\\同花顺\\hexin.exe", NULL, NULL, SW_SHOW);
}

void StockClientLogicManager::init()
{

}