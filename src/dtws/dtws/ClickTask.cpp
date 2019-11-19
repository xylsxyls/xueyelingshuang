#include "ClickTask.h"
#include "CMouse/CMouseAPI.h"
#include "CheckTask.h"

ClickTask::ClickTask():
m_checkWorkThreadId(0)
{

}

void ClickTask::DoTask()
{
	RCSend("step = %d", m_step);
	CTaskThreadManager::Instance().GetThreadInterface(m_checkWorkThreadId)->StopCurTask();
	int32_t index = -1;
	while (index++ != m_vecClick.size() - 1)
	{
		CMouse::MoveClick(m_vecClick[index].first, m_vecClick[index].second);
	}
	std::shared_ptr<CheckTask> spCheckTask(new CheckTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_checkWorkThreadId)->PostTask(spCheckTask);
}

void ClickTask::setParam(uint32_t checkWorkThreadId, const std::vector<std::pair<xyls::Point, int32_t>>& vecClick)
{
	m_checkWorkThreadId = checkWorkThreadId;
	m_vecClick = vecClick;
}