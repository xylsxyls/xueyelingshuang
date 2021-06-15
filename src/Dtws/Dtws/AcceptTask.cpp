#include "AcceptTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"

AcceptTask::AcceptTask():
m_times(0)
{

}

void AcceptTask::DoTask()
{
	std::string currentExePath = CSystem::GetCurrentExePath();
	ScreenScript::FindPic(currentExePath + "res\\talkhead.png", m_findPicRect, true, 0);
	CMouse::MoveOpposite(xyls::Point(63, 418), 0);
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