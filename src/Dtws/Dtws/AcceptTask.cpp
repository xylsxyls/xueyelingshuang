#include "AcceptTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"

extern int32_t g_accountCount;

AcceptTask::AcceptTask()
{

}

void AcceptTask::DoTask()
{
	if (g_accountCount != 1)
	{
		CMouse::MoveAbsolute(m_click, 0);
		CMouse::MiddleClick();
	}
	Sleep(g_accountCount == 1 ? 200 : 300);

	std::string currentExePath = CSystem::GetCurrentExePath();
	xyls::Point findPoint = ScreenScript::FindPic(currentExePath + "res\\talkhead.png", m_findPicRect);

	int32_t index = -1;
	while (index++ != m_acceptPoint.size() - 1)
	{
		CMouse::MoveAbsolute(findPoint, 0);
		CMouse::MoveOpposite(m_acceptPoint[index], 0);
		CMouse::LeftClick(0);
		Sleep(800);
	}
}

void AcceptTask::setParam(const xyls::Point& click, const xyls::Rect& findPicRect, const std::vector<xyls::Point>& acceptPoint)
{
	m_click = click;
	m_findPicRect = findPicRect;
	m_acceptPoint = acceptPoint;
}