#include "SaveAllMarketTask.h"
#include "StockClientLogicManager.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "StockClient.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMysql/StockMysqlAPI.h"

SaveAllMarketTask::SaveAllMarketTask():
m_beginIndex(0),
m_stockClient(nullptr)
{
	
}

void SaveAllMarketTask::DoTask()
{
	if (m_allStock.empty())
	{
		m_allStock = StockMysql::instance().allStockFromMysql();
	}
	
	CSystem::CreateDir(CSystem::GetCurrentExePath() + "CurrentAllMarketFile\\");
	std::string marketFolder = CSystem::GetCurrentExePath() + "CurrentAllMarketFile\\";
	//std::string marketFolder = "D:\\xueyelingshuang\\lib\\CurrentAllMarketFile\\";
	StockClientLogicManager::instance().openTonghuashun();
	CSystem::Sleep(8000);
	CMouse::MoveAbsolute(xyls::Point(98, 7), 50);
	CMouse::LeftClick();
	CMouse::MoveOpposite(xyls::Point(46, 188), 50);
	CMouse::LeftDoubleClick();
	int32_t index = m_beginIndex - 1;
	while (index++ != m_allStock.size() - 1)
	{
		const std::string& marketFilePath = marketFolder + m_allStock[index] + ".txt";
		CMouse::MoveAbsolute(xyls::Point(179, 189), 0);
		CMouse::LeftClick();
		CSystem::Sleep(200);
		CKeyboard::InputString(m_allStock[index] + "\n");
		CSystem::Sleep(1000);
		CMouse::RightClick();
		CMouse::MoveAbsolute(xyls::Point(179 + 93, 189 + 163), 50);
		CMouse::LeftClick();
		CMouse::MoveAbsolute(xyls::Point(189 + 93 + 160, 189 + 163), 50);
		CMouse::LeftClick();
		CSystem::Sleep(1000);
		CMouse::MoveAbsolute(xyls::Point(959, 289), 0);
		CMouse::LeftClick();
		CSystem::setClipboardData((HWND)m_stockClient->winId(), marketFilePath);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('V');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('V');
		CSystem::Sleep(200);
		CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
		CMouse::LeftClick();
		CSystem::Sleep(500);
		CMouse::MoveAbsolute(xyls::Point(1085, 666), 0);
		CMouse::LeftClick();
		CMouse::MoveAbsolute(xyls::Point(852, 327), 0);
		CMouse::LeftClick();
		CMouse::MoveOpposite(xyls::Point(0, 19), 0);
		CMouse::LeftClick();
		CMouse::MoveOpposite(xyls::Point(0, 19), 0);
		CMouse::LeftClick();
		CMouse::MoveOpposite(xyls::Point(0, 19), 0);
		CMouse::LeftClick();
		CMouse::MoveOpposite(xyls::Point(0, 19), 0);
		CMouse::LeftClick();
		CSystem::Sleep(500);
		CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
		CMouse::LeftClick();
		CSystem::Sleep(2000);
		CMouse::MoveAbsolute(xyls::Point(1082, 733), 0);
		CMouse::LeftClick();
	}
	
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("所有hangqing保存完成，01357,0中英文逗号分隔时间和星期"));
}

void SaveAllMarketTask::setParam(int32_t beginIndex, StockClient* stockClient, const std::vector<std::string>& allStock)
{
	m_beginIndex = beginIndex;
	m_stockClient = stockClient;
	m_allStock = allStock;
}