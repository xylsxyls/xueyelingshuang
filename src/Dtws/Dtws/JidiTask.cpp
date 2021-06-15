#include "JidiTask.h"
#include "GoTask.h"
#include "FindTask.h"
#include "Rect/RectAPI.h"
#include "CSystem/CSystemAPI.h"
#include "AcceptTask.h"

extern uint32_t* g_threadId;
extern int32_t g_accountCount;
extern xyls::Rect g_rightTopRect[3];
extern xyls::Point g_clickTop[3];
extern xyls::Rect g_talkheadRect[3];

JidiTask::JidiTask():
m_clientIndex(0),
m_exit(false)
{

}

void JidiTask::DoTask()
{
	Sleep(1500);

	std::string currentExePath = CSystem::GetCurrentExePath();

	std::shared_ptr<GoTask> spGoTask(new GoTask);
	spGoTask->setParam(g_clickTop[m_clientIndex], g_accountCount == 1 ? xyls::Rect(1869, 189, 1920, 249) : g_rightTopRect[m_clientIndex], "¸ÊÈª¹È½ÓÒýÈË", 1);
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
		spAcceptTask->setParam(g_clickTop[m_clientIndex], g_accountCount == 1 ? xyls::Rect(611, 146, 1336, 552) : g_talkheadRect[m_clientIndex], 1);
		CTaskThreadManager::Instance().GetThreadInterface(*g_threadId)->PostTask(spAcceptTask, 2);
	}
}

void JidiTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void JidiTask::setParam(int32_t clientIndex)
{
	m_clientIndex = clientIndex;
}

void JidiTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}