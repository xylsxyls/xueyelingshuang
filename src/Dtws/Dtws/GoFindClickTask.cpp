#include "GoFindClickTask.h"
#include "CSystem/CSystemAPI.h"
#include "GoTask.h"
#include "FindTask.h"
#include "AcceptTask.h"
#include "Rect/RectAPI.h"
#include "Config.h"

GoFindClickTask::GoFindClickTask():
m_preSleepTime(0),
m_clientIndex(0),
m_clickIndex(0),
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
	spGoTask->setParam(g_config.m_accountCount == 1 ? xyls::Point(149, 9) : g_screen.m_clickTop[m_clientIndex],
		g_config.m_accountCount == 1 ? xyls::Rect(1318, 191, 1920, 249) : g_screen.m_rightTopRect[m_clientIndex],
		m_placeName,
		m_clickIndex);
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spGoTask);

	bool isFindYizhan = false;
	bool isFind = false;
	while (!m_exit)
	{
		Semaphore findSemaphore;
		std::shared_ptr<FindTask> spFindTask(new FindTask);
		spFindTask->setParam(g_screen.m_clickTop[m_clientIndex],
			g_config.m_accountCount == 1 ? xyls::Rect(439, 77, 1336, 552) : g_screen.m_talkWindowHeadRect[m_clientIndex],
			currentExePath + "res\\talkhead.png", &isFind, &findSemaphore);
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spFindTask);
		findSemaphore.wait();
		if (isFind)
		{
			Semaphore findYizhanSemaphore;
			std::shared_ptr<FindTask> spFindYizhanTask(new FindTask);
			spFindYizhanTask->setParam(g_screen.m_clickTop[m_clientIndex],
				g_config.m_accountCount == 1 ? xyls::Rect(439, 77, 1336, 552) : g_screen.m_talkWindowHeadRect[m_clientIndex],
				currentExePath + "res\\yizhan.png", &isFindYizhan, &findYizhanSemaphore);
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spFindYizhanTask);
			findYizhanSemaphore.wait();
			if (!isFindYizhan)
			{
				Semaphore findSemaphore;
				std::shared_ptr<FindTask> spFindTask(new FindTask);
				spFindTask->setParam(g_screen.m_clickTop[m_clientIndex],
					g_config.m_accountCount == 1 ? xyls::Rect(439, 77, 1336, 552) : g_screen.m_talkWindowHeadRect[m_clientIndex],
					currentExePath + "res\\talkhead.png", &isFind, &findSemaphore);
				CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spFindTask);
				findSemaphore.wait();
				if (isFind)
				{
					break;
				}
			}
		}
		Sleep(1000);
	}
	if (m_exit)
	{
		return;
	}
	if (isFind)
	{
		std::shared_ptr<AcceptTask> spAcceptTask(new AcceptTask);
		spAcceptTask->setParam(g_config.m_accountCount == 1 ? xyls::Point(149, 9) : g_screen.m_clickTop[m_clientIndex],
			g_config.m_accountCount == 1 ? xyls::Rect(439, 77, 1336, 552) : g_screen.m_talkWindowHeadRect[m_clientIndex],
			m_acceptPoint);
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spAcceptTask, 2);
	}
}

void GoFindClickTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void GoFindClickTask::setParam(int32_t preSleepTime,
	int32_t clientIndex,
	const std::string& placeName,
	int32_t clickIndex,
	const std::vector<xyls::Point>& acceptPoint)
{
	m_preSleepTime = preSleepTime;
	m_clientIndex = clientIndex;
	m_placeName = placeName;
	m_clickIndex = clickIndex;
	m_acceptPoint = acceptPoint;
}

void GoFindClickTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}