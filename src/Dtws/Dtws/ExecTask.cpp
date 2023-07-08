#include "ExecTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"

void ExecTask::DoTask()
{
	int32_t loopIndex = -1;
	while (loopIndex++ != m_loopTimes - 1 && !m_exit)
	{
		int32_t index = -1;
		while (index++ != m_vecExec.size() - 1 && !m_exit)
		{
			if (!m_vecExec[index].first.empty())
			{
				CMouse::MoveAbsolute(m_vecExec[index].first, 0);
				CMouse::LeftClick(0);
			}
			else
			{
				CKeyboard::KeyPress(m_vecExec[index].second, 0);
			}
			Sleep(50);
		}
	}
}

AssignTask* ExecTask::copy()
{
	return new ExecTask;
}

void ExecTask::setParam(const std::vector<std::pair<xyls::Point, int32_t>>& vecExec, int32_t loopTimes)
{
	m_vecExec = vecExec;
	m_loopTimes = loopTimes;
}