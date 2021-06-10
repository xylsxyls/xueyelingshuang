#include "AcceptTask.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CMouse/CMouseAPI.h"

AcceptTask::AcceptTask():
m_times(0)
{

}

void AcceptTask::DoTask()
{
	ScreenScript::FindClick(m_findPic);
	CMouse::MoveOpposite(xyls::Point(0, 0));
	int32_t index = -1;
	while (index++ != m_times - 1)
	{
		CMouse::LeftClick();
		Sleep(800);
	}
}

void AcceptTask::setParam(const std::string& findPic, int32_t times)
{
	m_findPic = findPic;
	m_times = times;
}