#include "GoTask.h"
#include "CMouse/CMouseAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CScreen/CScreenAPI.h"

extern xyls::Rect g_rightTopRect[3];
extern int32_t g_accountCount;

GoTask::GoTask():
m_clickIndex(0)
{

}

void GoTask::DoTask()
{
	if (g_accountCount != 1)
	{
		CMouse::MoveAbsolute(m_clickTopPoint, 0);
		CMouse::LeftClick(0);
		Sleep(200);
	}

	std::string currentExePath = CSystem::GetCurrentExePath();
	ScreenScript::FindClick(currentExePath + "res\\go.png", true, false, m_placeRect, 0);
	Sleep(g_accountCount == 1 ? 400 : 100);
	xyls::Point picPoint = ScreenScript::FindPic(currentExePath + "res\\xun.png", xyls::Rect(385, 257, 1647, 900), true, 0);
	CMouse::MoveOpposite(xyls::Point(-100, 0), 0);
	CMouse::LeftClick(0);
	CSystem::setClipboardData(nullptr, m_placeName);
	CKeyboard::KeyDown(CKeyboard::Ctrl);
	CKeyboard::KeyDown('A');
	CKeyboard::KeyUp(CKeyboard::Ctrl);
	CKeyboard::KeyUp('A');
	CKeyboard::KeyDown(CKeyboard::Ctrl);
	CKeyboard::KeyDown('V');
	CKeyboard::KeyUp(CKeyboard::Ctrl);
	CKeyboard::KeyUp('V');
	CMouse::MoveAbsolute(picPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveOpposite(xyls::Point(0, -305 + (m_clickIndex - 1) * 45), 0);
	CMouse::LeftDoubleClick(0);
}

void GoTask::setParam(const xyls::Point& clickTopPoint, const xyls::Rect& placeRect, const std::string& placeName, int32_t clickIndex)
{
	m_clickTopPoint = clickTopPoint;
	m_placeRect = placeRect;
	m_placeName = placeName;
	m_clickIndex = clickIndex;
}