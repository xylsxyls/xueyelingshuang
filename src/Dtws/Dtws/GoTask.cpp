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
	std::string currentExePath = CSystem::GetCurrentExePath();
	int32_t index = -1;
	while (index++ != g_accountCount - 1)
	{
		if (g_accountCount == 1)
		{
			ScreenScript::FindClick(currentExePath + "res\\go.png", true, false, xyls::Rect(1869, 189, 1920, 249), 0);
		}
		else
		{
			ScreenScript::FindClick(currentExePath + "res\\go.png", true, false, g_rightTopRect[index], 0);
		}
		Sleep(100);
		xyls::Point picPoint = ScreenScript::FindPic(currentExePath + "res\\xun.png", xyls::Rect(357, 257, 1600, 831), true, 0);
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
}

void GoTask::setParam(const std::string& placeName, int32_t clickIndex)
{
	m_placeName = placeName;
	m_clickIndex = clickIndex;
}