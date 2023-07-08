#include "ClickWindowAtomicTask.h"
#include "CMouse/CMouseAPI.h"

bool ClickWindowAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (vecPic.empty())
	{
		return true;
	}
	lowSleep(200);
	CMouse::MoveAbsolute(vecPic[0].second.leftTop() + m_oppositePoint, 10);
	lowSleep(200);
	CMouse::LeftClick(100);
	lowSleep(200);
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