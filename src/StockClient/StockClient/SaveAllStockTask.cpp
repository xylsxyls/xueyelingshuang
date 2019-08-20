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
	std::string allStockPath = CSystem::GetCurrentExePath() + "all_stock.txt";
	//std::string allStockPath = "D:\\xueyelingshuang\\lib\\all_stock.txt";
	StockClientLogicManager::instance().openTonghuashun();
	CSystem::Sleep(8000);
	CMouse::MoveAbsolute(xyls::Point(7, 94), 10);
	CMouse::LeftClick();
	CSystem::Sleep(3000);
	CMouse::MoveAbsolute(xyls::Point(134, 383), 10);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(1253, 384), 10);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(471, 373), 10);
	CMouse::RightClick();
	CMouse::MoveOpposite(xyls::Point(71, 237), 10);
	CMouse::MoveOpposite(xyls::Point(160, 0), 10);
	CMouse::LeftClick();
	CSystem::Sleep(1000);
	CMouse::MoveAbsolute(xyls::Point(959, 289), 10);
	CMouse::LeftClick();
	CSystem::setClipboardData((HWND)m_stockClient->winId(), allStockPath);
	CKeyboard::KeyDown(CKeyboard::Ctrl);
	CKeyboard::KeyDown('V');
	CKeyboard::KeyUp(CKeyboard::Ctrl);
	CKeyboard::KeyUp('V');
	CSystem::Sleep(200);
	CKeyboard::InputString("\n");
	CMouse::MoveAbsolute(xyls::Point(1085, 666), 10);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 327), 10);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 346), 10);
	CMouse::LeftClick();
	CMouse::MoveAbsolute(xyls::Point(852, 384), 10);
	CMouse::LeftClick();
	CSystem::Sleep(500);
	CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
	CMouse::LeftClick();
	CSystem::Sleep(8000);
	CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
	CMouse::LeftClick();

	std::vector<std::vector<std::string>> allStockData;
	Ctxt txt(allStockPath);
	txt.LoadTxt(Ctxt::SPLIT, "\t");
	int32_t index = 0;
	while (index++ != txt.m_vectxt.size() - 2)
	{
		allStockData.push_back(std::vector<std::string>());
		std::string stock = txt.m_vectxt[index][0];
		stock = CStringManager::Mid(stock, 2, 6);
		allStockData.back().push_back(stock);
		std::string& name = txt.m_vectxt[index][1];
		allStockData.back().push_back(name);
		allStockData.back().push_back((CStringManager::Replace(name, "ST", "ST") != 0) ? "1" : "0");
	}
	StockMysql::instance().saveAllStock(allStockData);
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("±£´æÍê³É"));
}

void SaveAllStockTask::setParam(StockClient* stockClient)
{
	m_stockClient = stockClient;
}