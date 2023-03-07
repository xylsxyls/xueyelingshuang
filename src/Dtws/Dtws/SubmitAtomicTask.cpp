#include "SubmitAtomicTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

bool SubmitAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	CMouse::MoveAbsolute(g_screen.m_submitPoint[m_accountIndex], 0);
	CMouse::LeftClick(0);
	return false;
}