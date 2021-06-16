#include "GoFindClickTask.h"
#include "CSystem/CSystemAPI.h"
#include "GoTask.h"
#include "FindTask.h"
#include "AcceptTask.h"
#include "Rect/RectAPI.h"

extern uint32_t* g_threadId;
extern int32_t g_accountCount;
extern xyls::Rect g_rightTopRect[3];
extern xyls::Point g_clickTop[3];
extern xyls::Rect g_talkheadRect[3];

GoFindClickTask::GoFindClickTask():
m_preSleepTime(0),
m_clientIndex(0),
m_clickIndex(0),
m_findClickTimes(0),
m_exit(false)
{

}

void GoFindClickTask::DoTask()
{
	if (m_preSleepTime != 0)
	{
		Sleep(m_preSleepTime);
	}

	std::string currentExePath = CSystem::GetCurrentExePath();

	std::shared_ptr<GoTask> spGoTask(new GoTask);
	spGoTask->setParam(g_clickTop[m_clientIndex], g_accountCount == 1 ? xyls::Rect(1869, 189, 1920, 249) : g_rightTopRect[m_clientIndex], m_placeName, m_clickIndex);
	CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spGoTask);

	bool isFind = false;
	while (!m_exit && !isFind)
	{
		Semaphore findSemaphore;
		std::shared_ptr<FindTask> spFindTask(new FindTask);
		spFindTask->setParam(g_clickTop[m_clientIndex], g_accountCount == 1 ? xyls::Rect(611, 146, 1336, 552) : g_talkheadRect[m_clientIndex], currentExePath + "res\\talkhead.png", &isFind, &findSemaphore);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spFindTask);
		findSemaphore.wait();
		if (!isFind)
		{
			Sleep(1000);
		}
	}
	if (isFind)
	{
		std::shared_ptr<AcceptTask> spAcceptTask(new AcceptTask);
		spAcceptTask->setParam(g_clickTop[m_clientIndex], g_accountCount == 1 ? xyls::Rect(611, 146, 1336, 552) : g_talkheadRect[m_clientIndex], m_findClickTimes);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spAcceptTask, 2);
	}
}

void GoFindClickTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void GoFindClickTask::setParam(int32_t preSleepTime, int32_t clientIndex, const std::string& placeName, int32_t clickIndex, int32_t findClickTimes)
{
	m_preSleepTime = preSleepTime;
	m_clientIndex = clientIndex;
	m_placeName = placeName;
	m_clickIndex = clickIndex;
	m_findClickTimes = findClickTimes;
}

void GoFindClickTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}