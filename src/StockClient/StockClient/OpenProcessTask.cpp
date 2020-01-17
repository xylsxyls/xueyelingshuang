#include "OpenProcessTask.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CMouse/CMouseAPI.h"

OpenProcessTask::OpenProcessTask():
m_findTime(0),
m_unfindTime(0)
{

}

void OpenProcessTask::DoTask()
{
	std::string fileName = CSystem::GetName(m_path, 1);
	if (CSystem::processFirstPid(fileName) != 0)
	{
		if (m_findTime != 0)
		{
			Sleep(m_findTime);
		}
		if (!m_point.empty())
		{
			CMouse::MoveClick(m_point);
		}
		return;
	}
	ShellExecuteA(NULL, "open", m_path.c_str(), NULL, NULL, SW_SHOW);
	if (m_unfindTime != 0)
	{
		Sleep(m_unfindTime);
	}
}

void OpenProcessTask::setParam(const std::string& path, int32_t findTime, int32_t unfindTime, const xyls::Point& point)
{
	m_path = path;
	m_findTime = findTime;
	m_unfindTime = unfindTime;
	m_point = point;
}