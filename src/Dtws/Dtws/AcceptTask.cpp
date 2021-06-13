#include "AcceptTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CScreen/CScreenAPI.h"
#include "CSystem/CSystemAPI.h"

AcceptTask::AcceptTask():
m_times(0)
{

}

void AcceptTask::DoTask()
{
	std::string currentExePath = CSystem::GetCurrentExePath();
	int32_t x = 0;
	int32_t y = 0;
	CScreen::FindPic(m_findPicRect, currentExePath + "res\\talkhead.png", x, y, xyls::Color(0, 0, 0), 0.7);
	CMouse::MoveAbsolute(xyls::Point(x, y), 0);
	CMouse::MoveOpposite(xyls::Point(109, 419), 0);
	int32_t index = -1;
	while (index++ != m_times - 1)
	{
		CMouse::LeftClick();
		Sleep(800);
	}
}

void AcceptTask::setParam(const xyls::Rect& findPicRect, int32_t times)
{
	m_findPicRect = findPicRect;
	m_times = times;
}