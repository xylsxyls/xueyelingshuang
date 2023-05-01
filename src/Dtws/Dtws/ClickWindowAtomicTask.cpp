#include "ClickWindowAtomicTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

bool ClickWindowAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (vecPic.empty())
	{
		return true;
	}
	CMouse::MoveAbsolute(vecPic[0].second.leftTop() + m_oppositePoint, 0);
	CMouse::LeftClick(0);
	Sleep(500);
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