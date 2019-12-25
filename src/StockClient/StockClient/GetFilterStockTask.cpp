#include "GetFilterStockTask.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "StockParam.h"
#include "StockClient.h"
#include "CStringManager/CStringManagerAPI.h"

GetFilterStockTask::GetFilterStockTask():
m_hWnd(nullptr),
m_today(0, 0),
m_regain(false),
m_stockClient(nullptr)
{

}

void GetFilterStockTask::DoTask()
{
	if (m_today.empty())
	{
		m_today = m_stockClient->m_today;
	}
	std::string filePath = CSystem::GetCurrentExePath() + m_today.dateToString() + ".xls";
	if (m_regain || !CSystem::fileExist(filePath))
	{
		CMouse::MoveAbsolute(xyls::Point(521, 38), 0);
		CMouse::LeftClick();
		Sleep(5000);
		CMouse::MoveAbsolute(xyls::Point(1161, 105), 0);
		CMouse::LeftClick();
		Sleep(1000);
		CMouse::MoveAbsolute(xyls::Point(1161, 105), 0);
		CMouse::LeftClick();
		Sleep(3000);
		CMouse::MoveAbsolute(xyls::Point(645, 122), 0);
		CMouse::LeftClick();
		CSystem::setClipboardData(m_hWnd, SEARCH_STR);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('V');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('V');
		Sleep(100);
		CMouse::MoveAbsolute(xyls::Point(1337, 121), 0);
		CMouse::LeftClick();
		Sleep(5000);
		CMouse::MoveAbsolute(xyls::Point(423, 303), 0);
		CMouse::LeftClick();
		Sleep(1000);
		CSystem::setClipboardData(m_hWnd, m_today.dateToString() + ".xls");
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('V');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('V');
		Sleep(200);
		CMouse::MoveAbsolute(xyls::Point(459, 65), 0);
		CMouse::LeftClick();
		Sleep(50);
		CSystem::setClipboardData(m_hWnd, CSystem::GetCurrentExePath());
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('V');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('V');
		Sleep(50);
		CKeyboard::InputString("\n");
		Sleep(50);
		CMouse::MoveAbsolute(xyls::Point(782, 527), 0);
		CMouse::LeftClick();
		Sleep(2000);
	}
	ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOW);
	Sleep(4000);
	CMouse::MoveAbsolute(xyls::Point(77, 190), 0);
	CMouse::LeftClick();
	Sleep(100);
	CKeyboard::KeyDown(CKeyboard::Ctrl);
	CKeyboard::KeyDown('C');
	CKeyboard::KeyUp(CKeyboard::Ctrl);
	CKeyboard::KeyUp('C');
	Sleep(3000);
	std::string data = CSystem::GetClipboardData(m_hWnd);
	m_stockClient->m_allFilterStock = CStringManager::split(data, "\r\n");
	if (m_stockClient->m_allFilterStock.empty())
	{
		RCSend("filter stock empty");
		return;
	}
	m_stockClient->m_allFilterStock.erase(m_stockClient->m_allFilterStock.begin());
	m_stockClient->m_allFilterStock.pop_back();
	int32_t index = -1;
	while (index++ != m_stockClient->m_allFilterStock.size() - 1)
	{
		m_stockClient->m_allFilterStock[index] = CStringManager::split(m_stockClient->m_allFilterStock[index], ".")[0];
	}
	std::sort(m_stockClient->m_allFilterStock.begin(), m_stockClient->m_allFilterStock.end());
	CSystem::setClipboardData(m_hWnd, "");
	Sleep(1000);
	CMouse::MoveAbsolute(xyls::Point(1903, 7), 0);
	CMouse::LeftClick();
	Sleep(500);
	//CSystem::deleteFile((CSystem::GetCurrentExePath() + m_today.dateToString() + ".xls").c_str());
	//Sleep(50);
	CMouse::MoveAbsolute(xyls::Point(10, 98), 0);
	CMouse::LeftClick();
}

void GetFilterStockTask::setParam(HWND hWnd, const IntDateTime& today, bool regain, StockClient* stockClient)
{
	m_hWnd = hWnd;
	m_today = today;
	m_regain = regain;
	m_stockClient = stockClient;
}