#include "ConvoyTask.h"
#include "GoTask.h"
#include "FindTask.h"
#include "AcceptTask.h"
#include "WaitTask.h"
#include "CSystem/CSystemAPI.h"
#include "Config.h"

ConvoyTask::ConvoyTask():
m_preSleepTime(0),
m_clientIndex(0),
m_clickIndex(0),
m_exit(false)
{

}

void ConvoyTask::DoTask()
{
	if (m_preSleepTime != 0)
	{
		Sleep(m_preSleepTime);
	}

	std::string currentExePath = CSystem::GetCurrentExePath();

	std::shared_ptr<GoTask> spGoTask(new GoTask);
	spGoTask->setParam(g_config.m_accountCount == 1 ? xyls::Point(149, 9) : g_config.m_clickTop[m_clientIndex],
		g_config.m_accountCount == 1 ? xyls::Rect(1869, 189, 1920, 249) : g_config.m_rightTopRect[m_clientIndex],
		m_placeName,
		m_clickIndex);
	CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spGoTask);

	bool isFind = false;
	bool isFindError = false;
	while (!m_exit)
	{
		Semaphore findSemaphore;
		std::shared_ptr<FindTask> spFindTask(new FindTask);
		spFindTask->setParam(g_config.m_clickTop[m_clientIndex],
			g_config.m_accountCount == 1 ? xyls::Rect(611, 146, 1336, 552) : g_config.m_talkheadRect[m_clientIndex],
			currentExePath + "res\\talkhead.png",
			&isFind,
			&findSemaphore);
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spFindTask);
		findSemaphore.wait();
		if (isFind)
		{
			break;
		}
		Semaphore findErrorSemaphore;
		std::shared_ptr<FindTask> spFindErrorTask(new FindTask);
		spFindErrorTask->setParam(g_config.m_clickTop[m_clientIndex],
			g_config.m_accountCount == 1 ? xyls::Rect(0, 770, 348, 1009) : g_config.m_chatRect[m_clientIndex],
			currentExePath + "res\\jiaohumaer.png",
			&isFindError,
			&findErrorSemaphore);
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spFindErrorTask);
		findErrorSemaphore.wait();
		if (isFindError)
		{
			std::shared_ptr<WaitTask> spWaitTask(new WaitTask);
			spWaitTask->setParam(g_config.m_accountCount == 1 ? xyls::Point(149, 9) : g_config.m_clickTop[m_clientIndex]);
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spWaitTask, 2);
			Sleep(15000);
			std::shared_ptr<GoTask> spGoTask(new GoTask);
			spGoTask->setParam(g_config.m_accountCount == 1 ? xyls::Point(149, 9) : g_config.m_clickTop[m_clientIndex],
				g_config.m_accountCount == 1 ? xyls::Rect(1869, 189, 1920, 249) : g_config.m_rightTopRect[m_clientIndex],
				m_placeName,
				m_clickIndex);
			CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spGoTask, 2);
			continue;
		}
		Sleep(500);
	}
	if (isFind)
	{
		std::shared_ptr<AcceptTask> spAcceptTask(new AcceptTask);
		spAcceptTask->setParam(g_config.m_accountCount == 1 ? xyls::Point(149, 9) : g_config.m_clickTop[m_clientIndex],
			g_config.m_accountCount == 1 ? xyls::Rect(611, 146, 1336, 552) : g_config.m_talkheadRect[m_clientIndex],
			m_acceptPoint);
		CTaskThreadManager::Instance().GetThreadInterface(*g_config.m_threadId)->PostTask(spAcceptTask, 2);
	}
}

void ConvoyTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void ConvoyTask::setParam(int32_t preSleepTime,
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

void ConvoyTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}