#include "SaveTodayMarketFileTask.h"
#include "StockClientLogicManager.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "StockClient.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMysql/StockMysqlAPI.h"

SaveTodayMarketFileTask::SaveTodayMarketFileTask() :
m_stockClient(nullptr)
{

}

void SaveTodayMarketFileTask::DoTask()
{
	std::string allStockPath = CSystem::GetCurrentExePath() + "all_stock.txt";
	//std::string allStockPath = "D:\\xueyelingshuang\\lib\\all_stock.txt";
	StockClientLogicManager::instance().openTonghuashun();
	CMouse::MoveAbsolute(xyls::Point(7, 94), 50);
	CMouse::LeftClick();
	CSystem::Sleep(3000);
	CMouse::MoveAbsolute(xyls::Point(134, 383), 50);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(1253, 384), 50);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(471, 373), 50);
	CMouse::RightClick();
	CMouse::MoveOpposite(xyls::Point(71, 237), 50);
	CMouse::MoveOpposite(xyls::Point(160, 0), 50);
	CMouse::LeftClick();
	CSystem::Sleep(1000);
	CMouse::MoveAbsolute(xyls::Point(959, 289), 50);
	CMouse::LeftClick();
	CSystem::setClipboardData((HWND)m_stockClient->winId(), allStockPath);
	CKeyboard::KeyDown(CKeyboard::Ctrl);
	CKeyboard::KeyDown('V');
	CKeyboard::KeyUp(CKeyboard::Ctrl);
	CKeyboard::KeyUp('V');
	CSystem::Sleep(200);
	CKeyboard::InputString("\n");
	CMouse::MoveAbsolute(xyls::Point(1085, 666), 50);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 327), 50);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 346), 50);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 384), 50);
	CMouse::LeftClick();
	CSystem::Sleep(500);
	CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
	CMouse::LeftClick();
	CSystem::Sleep(8000);
	CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
	CMouse::LeftClick();
}

void SaveTodayMarketFileTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}