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

void StockClientLogicManager::openTonghuashun()
{
	if (CSystem::processFirstPid(L"hexin.exe") != 0)
	{
		Sleep(3000);
		return;
	}
	ShellExecute(NULL, L"open", L"C:\\同花顺软件\\同花顺\\hexin.exe", NULL, NULL, SW_SHOW);
	Sleep(8000);
}

void StockClientLogicManager::init()
{

}