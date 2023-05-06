#include "ClickWindowAtomicTask.h"
#include "CMouse/CMouseAPI.h"

bool ClickWindowAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (vecPic.empty())
	{
		return true;
	}
	lowSleep(100);
	CMouse::MoveAbsolute(vecPic[0].second.leftTop() + m_oppositePoint, 0);
	lowSleep(100);
	CMouse::LeftClick(50);
	lowSleep(100);
	return false;
}

bool ClickWindowAtomicTask::CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (!vecPic.empty())
	{
		return true;
	}
	return false;
}

void ClickWindowAtomicTask::setParam(const xyls::Point& oppositePoint)
{
	m_oppositePoint = oppositePoint;
}