#include "TalkHeroAtomicTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

bool TalkHeroAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	CMouse::MoveAbsolute(g_screen.m_talkHeroHeadPoint[m_accountIndex], 0);
	CMouse::LeftClick(0);
	return false;
}