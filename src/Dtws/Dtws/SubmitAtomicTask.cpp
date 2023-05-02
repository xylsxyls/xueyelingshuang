#include "SubmitAtomicTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"
#include "DtwsParam.h"
#include "CSystem/CSystemAPI.h"

SubmitAtomicTask::SubmitAtomicTask():
m_lineIndex(0)
{

}

bool SubmitAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (CSystem::getComputerName() != FIRST_COMPUTER)
	{
		Sleep(100);
	}
	CMouse::MoveAbsolute(g_screen.m_submitPoint[m_accountIndex] + xyls::Point(0, m_lineIndex * g_config.m_textLineHeight), 0);
	CMouse::LeftClick(0);
	return false;
}

void SubmitAtomicTask::setParam(int32_t lineIndex)
{
	m_lineIndex = lineIndex;
}