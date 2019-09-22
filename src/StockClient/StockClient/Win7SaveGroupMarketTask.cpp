#include "Win7SaveGroupMarketTask.h"
#include "StockClientLogicManager.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "StockClient.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "StockMysql/StockMysqlAPI.h"

Win7SaveGroupMarketTask::Win7SaveGroupMarketTask():
m_beginIndex(0),
m_end(0),
m_stockClient(nullptr),
m_exit(false)
{
	
}

void Win7SaveGroupMarketTask::DoTask()
{
	if (m_groupStock.empty())
	{
		m_groupStock = StockMysql::instance().allStockFromMysql();
	}

	if (m_end == 0)
	{
		m_end = m_groupStock.size();
	}
	
	CSystem::CreateDir(CSystem::GetCurrentExePath() + "CurrentAllMarketFile\\");
	std::string marketFolder = CSystem::GetCurrentExePath() + "CurrentAllMarketFile\\";
	CMouse::MoveAbsolute(xyls::Point(98, 7), 50);
	CMouse::LeftClick();
	CMouse::MoveOpposite(xyls::Point(46, 188), 50);
	CMouse::LeftDoubleClick();
	int32_t index = m_beginIndex - 1;
	while (index++ < m_end - 1)
	{
		if (m_exit)
		{
			return;
		}
		const std::string& marketFilePath = marketFolder + m_groupStock[index] + ".txt";
		CMouse::MoveAbsolute(xyls::Point(408, 250), 0);
		CMouse::LeftClick();
		CSystem::Sleep(500);
		CKeyboard::InputString(m_groupStock[index] + "\n", 100);
		CSystem::Sleep(1000);
		CMouse::RightClick();
		CMouse::MoveAbsolute(xyls::Point(539, 412), 50);
		CMouse::LeftClick();
		CMouse::MoveAbsolute(xyls::Point(539 + 160, 412), 50);
		CMouse::LeftClick();
		CSystem::Sleep(1000);
		CMouse::MoveAbsolute(xyls::Point(953, 356), 0);
		CMouse::LeftClick();
		CSystem::setClipboardData((HWND)m_stockClient->winId(), marketFilePath);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('V');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('V');
		CSystem::Sleep(200);
		CMouse::MoveAbsolute(xyls::Point(1074, 671), 0);
		CMouse::LeftClick();
		CSystem::Sleep(500);
		CMouse::MoveAbsolute(xyls::Point(1085, 632), 0);
		CMouse::LeftClick();
		CMouse::MoveAbsolute(xyls::Point(889, 390), 0);
		CMouse::LeftClick();
		CMouse::MoveOpposite(xyls::Point(0, 14), 0);
		CMouse::LeftClick();
		CMouse::MoveOpposite(xyls::Point(0, 14), 0);
		CMouse::LeftClick();
		CMouse::MoveOpposite(xyls::Point(0, 14), 0);
		CMouse::LeftClick();
		CMouse::MoveOpposite(xyls::Point(0, 14), 0);
		CMouse::LeftClick();
		CSystem::Sleep(500);
		CMouse::MoveAbsolute(xyls::Point(1074, 671), 0);
		CMouse::LeftClick();
		CSystem::Sleep(2000);
		CMouse::MoveAbsolute(xyls::Point(1074, 671), 0);
		CMouse::LeftClick();
	}
	
	emit StockClientLogicManager::instance().taskTip(QStringLiteral("所有hangqing保存完成，01357,0中英文逗号分隔时间和星期"));
}

void Win7SaveGroupMarketTask::setParam(int32_t beginIndex, int32_t endIndex, StockClient* stockClient, const std::vector<std::string>& groupStock)
{
	m_beginIndex = beginIndex;
	m_end = endIndex;
	m_stockClient = stockClient;
	m_groupStock = groupStock;
}

void Win7SaveGroupMarketTask::StopTask()
{
	m_exit = true;
}