#include "CompetitionAtomicTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

bool CompetitionAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	CMouse::MoveAbsolute(g_screen.m_competitionPoint[m_accountIndex], 0);
	CMouse::LeftDoubleClick(100);
	CMouse::LeftDoubleClick(100);
	return false;
}