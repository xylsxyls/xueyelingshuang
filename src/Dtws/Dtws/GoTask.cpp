#include "GoTask.h"
#include "CMouse/CMouseAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "DtwsParam.h"

extern xyls::Rect m_rightTopRect[3];
extern int32_t m_accountCount;

GoTask::GoTask():
m_clickIndex(0)
{

}

void GoTask::DoTask()
{
	CMouse::MoveAbsolute(m_click, 0);
	CMouse::MiddleClick();
	sleep(50);

	std::string currentExePath = CSystem::GetCurrentExePath();
	ScreenScript::FindClick(currentExePath + "res\\go.png", true, false, m_placeRect, 0.7, 0);
	int32_t findTimes = 0;
	while (!m_exit && ScreenScript::FindPic(currentExePath + "res\\xun.png", xyls::Rect(385, 257, 1647, 900)).empty())
	{
		++findTimes;
		sleep(50);
		if (findTimes % 10 == 0)
		{
			if (m_exit)
			{
				return;
			}
			ScreenScript::FindClick(currentExePath + "res\\go.png", true, false, m_placeRect, 0.7, 0);
		}
	}

	if (m_exit)
	{
		return;
	}

	xyls::Point picPoint = ScreenScript::FindPic(currentExePath + "res\\xun.png", xyls::Rect(385, 257, 1647, 900), 0.7, true, 0);
	CMouse::MoveOpposite(xyls::Point(-100, 0), 0);
	CMouse::LeftClick(0);
	//CSystem::setClipboardData(nullptr, m_placeName);
	//CKeyboard::KeyDown(CKeyboard::Ctrl);
	//CKeyboard::KeyDown('A');
	//CKeyboard::KeyUp(CKeyboard::Ctrl);
	//CKeyboard::KeyUp('A');
	//CKeyboard::KeyDown(CKeyboard::Ctrl);
	//CKeyboard::KeyDown('V');
	//CKeyboard::KeyUp(CKeyboard::Ctrl);
	//CKeyboard::KeyUp('V');
	sleep(1000);
	bool isSecond = false;
	if (CSystem::getComputerName() == SECOND_COMPUTER)
	{
		isSecond = true;
		sleep(2000);
	}
	CKeyboard::InputString(m_placeName + " ", isSecond ? 50 : 20);
	sleep(200);
	CMouse::MoveAbsolute(picPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveOpposite(xyls::Point(0, -305 + m_clickIndex * 45), 0);
	int32_t xunCount = 0;
	do
	{
		++xunCount;
		if (xunCount > 20)
		{
			break;
		}
		sleep(50);
		CMouse::LeftDoubleClick(0);
		sleep(200);
	} while (!ScreenScript::FindPic(currentExePath + "res\\xun.png", xyls::Rect(385, 257, 1647, 900)).empty());
}

void GoTask::setParam(const xyls::Point& click, const xyls::Rect& placeRect, const std::string& placeName, int32_t clickIndex)
{
	m_click = click;
	m_placeRect = placeRect;
	m_placeName = placeName;
	m_clickIndex = clickIndex;
}