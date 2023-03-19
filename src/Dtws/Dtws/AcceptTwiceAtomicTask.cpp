#include "AcceptTwiceAtomicTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

bool AcceptTwiceAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (vecPic.empty())
	{
		return true;
	}
	CMouse::MoveAbsolute(vecPic[0].second.leftTop() + g_config.m_getOppositePoint, 0);
	CMouse::LeftClick(0);
	Sleep(500);
	CMouse::MoveAbsolute(vecPic[0].second.leftTop() + g_config.m_acceptOppositePoint, 0);
	CMouse::LeftClick(0);
	Sleep(500);
	return false;
}

bool AcceptTwiceAtomicTask::CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (!vecPic.empty())
	{
		return true;
	}
	return false;
}