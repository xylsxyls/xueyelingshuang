#include "CStopWatch.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <windows.h>

class CStopWatchCountDownTask : public CTask
{
public:
	CStopWatchCountDownTask(int32_t taskId) :
		m_countDownMsec(0),
		m_task(nullptr)
	{
		m_taskId = taskId;
		m_handle = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
	~CStopWatchCountDownTask()
	{
		::CloseHandle(m_handle);
	}

public:
	void setCountDownSeconds(unsigned long countDownMsec)
	{
		m_countDownMsec = countDownMsec;
	}

	void setCountDownTask(CountDownTask* task)
	{
		m_task = task;
	}

	void DoTask()
	{
		auto res = ::WaitForSingleObject(m_handle, m_countDownMsec);
		switch (res)
		{
		//如果是超时说明需要执行任务了
		case WAIT_TIMEOUT:
		{
			if (m_task != nullptr)
			{
				m_task->DoTask();
			}
			break;
		}
		default:
			break;
		}
	}

	void StopTask()
	{
		::SetEvent(m_handle);
	}

private:
	unsigned long m_countDownMsec;
	CountDownTask* m_task;
	HANDLE m_handle;
};

CStopWatch::CStopWatch():
m_stopTime(0),
m_runTime(0),
m_stopOrRun(true)
{
	m_time = ::GetTickCount();
}

CStopWatch::~CStopWatch()
{

}

int32_t CStopWatch::GetHour()
{
	return GetWatchTime() / 3600000;
}

int32_t CStopWatch::GetMinute()
{
	return (GetWatchTime() - GetHour() * 3600000) / 60000;
}

double CStopWatch::GetSeconds()
{
	return (GetWatchTime() - GetHour() * 3600000 - GetMinute() * 60000) / 1000.0;
}

unsigned long CStopWatch::GetWatchTime()
{
	return m_stopOrRun == false ? m_stopTime - m_time : ::GetTickCount() - m_time;
}

void CStopWatch::SetWatchTime(unsigned long SetTime)
{
	m_time = m_time - (SetTime - GetWatchTime());
}

void CStopWatch::Run()
{
	if (m_stopOrRun == true)
	{
		return;
	}
	m_stopOrRun = true;
	m_runTime = ::GetTickCount();
	m_time = m_time + m_runTime - m_stopTime;
}

void CStopWatch::Stop()
{
	if (m_stopOrRun == false)
	{
		return;
	}
	m_stopOrRun = false;
	m_stopTime = ::GetTickCount();
}

void CStopWatch::CountDown(unsigned long countDownMsec, CountDownTask* task, int32_t taskId, int32_t& threadId)
{
	if (threadId == 0)
	{
		threadId = CTaskThreadManager::Instance().Init();
	}
	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
	if (taskThread == nullptr)
	{
		return;
	}
	std::shared_ptr<CStopWatchCountDownTask> countDownTask;
	countDownTask.reset(new CStopWatchCountDownTask(taskId));
	if (countDownTask == nullptr)
	{
		return;
	}
	countDownTask->setCountDownTask(task);
	countDownTask->setCountDownSeconds(countDownMsec);
	taskThread->PostTask(countDownTask, 1);
}

void CStopWatch::DiscardCountDown(int32_t taskId, int32_t threadId)
{
	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
	if (taskThread == nullptr)
	{
		return;
	}
	taskThread->StopTask(taskId, 1);
}

void CStopWatch::destroyThread(int32_t threadId)
{
	CTaskThreadManager::Instance().Uninit(threadId);
}

//class Task : public CountDownTask
//{
//public:
//	virtual void DoTask()
//	{
//		printf("1");
//	}
//};
//
//int main()
//{
//	int32_t threadId = 0;
//	CStopWatch::CountDown(5000, new Task, 1, threadId);
//	int32_t threadId2 = 0;
//	CStopWatch::CountDown(5000, new Task, 2, threadId);
//	Sleep(3000);
//	CStopWatch::DiscardCountDown(1, threadId);
//	
//	getchar();
//	return 0;
//}