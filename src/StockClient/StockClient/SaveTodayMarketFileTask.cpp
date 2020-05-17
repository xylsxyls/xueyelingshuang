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
	CMouse::MoveAbsolute(xyls::Point(7, 94), 200);
	CMouse::LeftClick();
	CSystem::Sleep(5000);
	CMouse::MoveAbsolute(xyls::Point(134, 383), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(1253, 384), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(471, 373), 100);
	CMouse::RightClick();
	CMouse::MoveOpposite(xyls::Point(71, 237), 100);
	CMouse::MoveOpposite(xyls::Point(160, 0), 100);
	CMouse::LeftClick();
	CSystem::Sleep(1000);
	CMouse::MoveAbsolute(xyls::Point(959, 289), 100);
	CMouse::LeftClick();
	Sleep(100);
	CSystem::setClipboardData((HWND)m_stockClient->winId(), CSystem::GetCurrentExePath() + "all_stock.txt");
	Sleep(100);
	CKeyboard::KeyDown(CKeyboard::Ctrl);
	CKeyboard::KeyDown('V');
	CKeyboard::KeyUp(CKeyboard::Ctrl);
	CKeyboard::KeyUp('V');
	CSystem::Sleep(200);
	CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(1085, 666), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 327), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 346), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 384), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(1214, 634), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(1214, 634), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(1214, 634), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(851, 564), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(853, 585), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 605), 100);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 624), 100);
	CMouse::LeftClick();
	CSystem::Sleep(500);
	CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
	CMouse::LeftClick();
	CSystem::Sleep(8000);
	CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
	CMouse::LeftClick();
	CSystem::Sleep(500);
}

void SaveTodayMarketFileTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}