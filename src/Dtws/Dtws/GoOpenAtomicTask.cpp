#include "GoOpenAtomicTask.h"
#include "Config.h"
#include "CMouse/CMouseAPI.h"

bool GoOpenAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (!vecPic.empty())
	{
		return false;
	}
	CMouse::MoveAbsolute(g_screen.m_goPoint[m_accountIndex], 0);
	CMouse::LeftClick(0);
	Sleep(1000);
	return false;
}

bool GoOpenAtomicTask::CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (vecPic.empty())
	{
		return true;
	}
	return false;
}