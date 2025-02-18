#include "SubmitAtomicTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

SubmitAtomicTask::SubmitAtomicTask():
m_lineIndex(0)
{

}

bool SubmitAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	lowSleep(100);
	CMouse::MoveAbsolute(g_screen.m_submitPoint[m_accountIndex] + xyls::Point(0, m_lineIndex * g_config.m_textLineHeight), 0);
	lowSleep(100);
	CMouse::LeftClick(50);
	lowSleep(100);
	return false;
}

void SubmitAtomicTask::setParam(int32_t lineIndex)
{
	m_lineIndex = lineIndex;
}