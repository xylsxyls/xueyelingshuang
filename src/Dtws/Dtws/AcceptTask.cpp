#include "AcceptTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include "Config.h"

AcceptTask::AcceptTask()
{

}

void AcceptTask::DoTask()
{
	if (g_config.m_accountCount != 1)
	{
		CMouse::MoveAbsolute(m_click, 0);
		CMouse::MiddleClick();
	}
	Sleep(g_config.m_accountCount == 1 ? 200 : 300);

	std::string currentExePath = CSystem::GetCurrentExePath();
	xyls::Rect findPoint = ScreenScript::FindPic(currentExePath + "res\\talkhead.png", m_findPicRect).center();

	int32_t index = -1;
	while (index++ != m_acceptPoint.size() - 1)
	{
		CMouse::MoveAbsolute(findPoint, 0);
		CMouse::MoveOpposite(m_acceptPoint[index], 0);
		CMouse::LeftClick(200);
		Sleep(1200);
	}
}

void AcceptTask::setParam(const xyls::Point& click, const xyls::Rect& findPicRect, const std::vector<xyls::Point>& acceptPoint)
{
	m_click = click;
	m_findPicRect = findPicRect;
	m_acceptPoint = acceptPoint;
}