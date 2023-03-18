#include "GoWriteDestAtomicTask.h"
#include "Config.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

GoWriteDestAtomicTask::GoWriteDestAtomicTask():
m_clickIndex(0)
{

}

bool GoWriteDestAtomicTask::DoFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	if (vecPic.empty())
	{
		return true;
	}
	CMouse::MoveAbsolute(vecPic[0].second.leftTop() + g_config.m_destEditOppositePoint, 0);
	CMouse::LeftDoubleClick(10);
	Sleep(50);
	CKeyboard::InputString(" " + m_dest + " ");
	CMouse::MoveAbsolute(vecPic[0].second.leftTop() + g_config.m_xunOppositePoint, 0);
	CMouse::LeftClick(0);
	Sleep(10);
	CMouse::MoveAbsolute(vecPic[0].second.leftTop() + g_config.m_destOppositePoint +
		xyls::Point(0, m_clickIndex * g_config.m_destHeight), 0);
	CMouse::LeftDoubleClick(10);
	return false;
}

bool GoWriteDestAtomicTask::CheckFun(const std::vector<std::pair<std::string, xyls::Rect>>& vecPic)
{
	return false;
}

void GoWriteDestAtomicTask::setParam(const std::string& dest, int32_t clickIndex)
{
	m_dest = dest;
	m_clickIndex = clickIndex;
}