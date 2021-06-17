#include "GoTask.h"
#include "CMouse/CMouseAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

extern xyls::Rect g_rightTopRect[3];
extern int32_t g_accountCount;

GoTask::GoTask():
m_clickIndex(0)
{

}

void GoTask::DoTask()
{
	CMouse::MoveAbsolute(m_click, 0);
	CMouse::MiddleClick();
	Sleep(50);

	std::string currentExePath = CSystem::GetCurrentExePath();
	ScreenScript::FindClick(currentExePath + "res\\go.png", true, false, m_placeRect, 0);
	int32_t findTimes = 0;
	while (ScreenScript::FindPic(currentExePath + "res\\xun.png", xyls::Rect(385, 257, 1647, 900)).empty())
	{
		++findTimes;
		Sleep(50);
		if (findTimes % 10 == 0)
		{
			ScreenScript::FindClick(currentExePath + "res\\go.png", true, false, m_placeRect, 0);
		}
	}

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
	Sleep(300);
	CMouse::MoveOpposite(xyls::Point(0, -305 + m_clickIndex * 45), 0);
	CMouse::LeftDoubleClick(0);
}

void GoTask::setParam(const xyls::Point& click, const xyls::Rect& placeRect, const std::string& placeName, int32_t clickIndex)
{
	m_click = click;
	m_placeRect = placeRect;
	m_placeName = placeName;
	m_clickIndex = clickIndex;
}