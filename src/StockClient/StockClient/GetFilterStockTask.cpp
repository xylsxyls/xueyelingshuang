#include "GetFilterStockTask.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "StockClient.h"
#include "CStringManager/CStringManagerAPI.h"

GetFilterStockTask::GetFilterStockTask():
m_hWnd(nullptr),
m_today(0, 0),
m_vecResult(nullptr),
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
	std::string filePath = CSystem::GetCurrentExePath() + m_today.dateToString() + m_fileName + ".xls";
	if (m_regain || !CSystem::fileExist(filePath))
	{
		CMouse::MoveAbsolute(xyls::Point(521, 38), 0);
		CMouse::LeftClick();
		Sleep(8000);
		CMouse::MoveAbsolute(xyls::Point(1161, 105), 0);
		CMouse::LeftClick();
		Sleep(5000);
		CMouse::MoveAbsolute(xyls::Point(1909, 105), 0);
		CMouse::LeftClick();
		Sleep(1000);
		CMouse::MoveAbsolute(xyls::Point(649, 352), 0);
		CMouse::LeftClick();
		CSystem::setClipboardData(m_hWnd, m_searchStr);
		Sleep(200);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('A');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('A');
		Sleep(50);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('V');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('V');
		Sleep(100);
		CMouse::MoveAbsolute(xyls::Point(1332, 350), 0);
		CMouse::LeftClick();
		Sleep(8000);
		CMouse::MoveAbsolute(xyls::Point(423, 303), 0);
		CMouse::LeftClick();
		Sleep(8000);
		CMouse::MoveAbsolute(xyls::Point(294, 542), 0);
		CMouse::LeftClick();
		Sleep(50);
		CSystem::setClipboardData(m_hWnd, m_today.dateToString() + m_fileName + ".xls");
		Sleep(200);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('A');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('A');
		Sleep(50);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('V');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('V');
		Sleep(200);
		CMouse::MoveAbsolute(xyls::Point(508, 154), 0);
		CMouse::LeftClick();
		Sleep(50);
		CSystem::setClipboardData(m_hWnd, CSystem::GetCurrentExePath());
		Sleep(200);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('A');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('A');
		Sleep(50);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('V');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('V');
		Sleep(50);
		CMouse::MoveAbsolute(xyls::Point(774, 156), 0);
		CMouse::LeftClick();
		Sleep(200);
		CMouse::MoveAbsolute(xyls::Point(850, 615), 0);
		CMouse::LeftClick();
		if (m_regain)
		{
			Sleep(50);
			CKeyboard::KeyPress(CKeyboard::Left);
			Sleep(50);
			CKeyboard::InputString("\n");
		}
		Sleep(2000);
		//CSystem::deleteFile((CSystem::GetCurrentExePath() + m_today.dateToString() + ".xls").c_str());
		//Sleep(50);
		CMouse::MoveAbsolute(xyls::Point(1688, 122), 0);
		CMouse::LeftClick();
		Sleep(1000);
		CMouse::MoveAbsolute(xyls::Point(10, 98), 0);
		CMouse::LeftClick();
	}
	if (m_regain || m_vecResult->empty())
	{
		ShellExecuteA(NULL, "open", filePath.c_str(), NULL, NULL, SW_SHOW);
		Sleep(5000);
		CMouse::MoveAbsolute(xyls::Point(1104, 574), 0);
		CMouse::LeftClick();
		Sleep(1000);
		CMouse::LeftClick();
		CMouse::MoveAbsolute(xyls::Point(77, 190), 0);
		CMouse::LeftClick();
		Sleep(100);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('C');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('C');
		Sleep(3000);
		std::string data = CSystem::GetClipboardData(m_hWnd);
		*m_vecResult = CStringManager::split(data, "\r\n");
		if (m_vecResult->empty())
		{
			RCSend("vec result stock empty, fileName = %s", m_fileName.c_str());
			return;
		}
		m_vecResult->erase(m_vecResult->begin());
		m_vecResult->pop_back();
		int32_t index = -1;
		while (index++ != m_vecResult->size() - 1)
		{
			(*m_vecResult)[index] = CStringManager::split((*m_vecResult)[index], ".")[0];
			CStringManager::Replace((*m_vecResult)[index], "\"", "");
			CStringManager::Replace((*m_vecResult)[index], "\t", "");
		}
		std::sort(m_vecResult->begin(), m_vecResult->end());
		CSystem::setClipboardData(m_hWnd, "");
		Sleep(1000);
		CMouse::MoveAbsolute(xyls::Point(1903, 7), 0);
		CMouse::LeftClick();
		Sleep(500);
	}
}

void GetFilterStockTask::setParam(HWND hWnd,
	const IntDateTime& today,
	const std::string& searchStr,
	std::vector<std::string>* vecResult,
	const std::string& fileName,
	bool regain,
	StockClient* stockClient)
{
	m_hWnd = hWnd;
	m_today = today;
	m_searchStr = searchStr;
	m_vecResult = vecResult;
	m_fileName = fileName;
	m_regain = regain;
	m_stockClient = stockClient;
}