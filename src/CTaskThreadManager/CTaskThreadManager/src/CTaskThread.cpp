﻿#include "CTaskThread.h"
#include <functional>
#include "Semaphore/SemaphoreAPI.h"

CTaskThread::CTaskThread(int32_t threadId) :
m_threadId(threadId),
m_semaphore(nullptr),
m_hasExitSignal(false),
m_waitForEndSignal(false),
m_curTaskLevel(0)
{
	m_semaphore = new Semaphore;
}

CTaskThread::~CTaskThread()
{
	delete m_semaphore;
	m_semaphore = nullptr;
}

bool CTaskThread::CreateThread()
{
	//必须先将m_hasExitSignal设为false，否则一进入线程函数就退出了
	std::thread* pThread = new std::thread(std::bind(&CTaskThread::WorkThread, this));
	if (pThread == nullptr)
	{
		m_hasExitSignal = true;
		return false;
	}
	m_spWorkThread.reset(pThread);
	
	return true;
}

void CTaskThread::DestroyThread()
{
	SetExitSignal();
	WaitForExit();
}

void CTaskThread::SetExitSignal()
{
	m_hasExitSignal = true;
	//设置了退出信号就立即把当前任务停止
	StopCurTask();
	m_semaphore->signal();
}

void CTaskThread::WaitForExit()
{
	//如果线程正常启动
	if (m_spWorkThread != nullptr)
	{
		m_spWorkThread->join();
		m_spWorkThread.reset(nullptr);
		//::CloseHandle(m_semaphore);
	}
}

void CTaskThread::PostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel)
{
	if (spTask == nullptr || taskLevel < 1 || m_hasExitSignal)
	{
		return;
	}
	std::unique_lock<std::mutex> lock(m_mutex);
	HandlePostTask(spTask, taskLevel);
}

bool CTaskThread::TryPostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel)
{
	if (spTask == nullptr || taskLevel < 1 || m_hasExitSignal)
	{
		return false;
	}
	if (!m_mutex.try_lock())
	{
		return false;
	}
	HandlePostTask(spTask, taskLevel);
	m_mutex.unlock();
	return true;
}

void CTaskThread::SendTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel)
{
	if (spTask == nullptr || taskLevel < 1 || m_hasExitSignal)
	{
		return;
	}

	Semaphore wairForSendSemaphore;
	spTask->SetWaitForSendHandle(&wairForSendSemaphore);
	PostTask(spTask, taskLevel);
	
	bool inWhile = true;
	while (inWhile)
	{
		if (spTask.use_count() == 1)
		{
			break;
		}
		wairForSendSemaphore.eventWait(50);
	}
	//::CloseHandle(waitForSend);
	spTask->SetWaitForSendHandle(nullptr);
}

void CTaskThread::WorkThread()
{
	//有退出信号，但是队列中有等待任务时仍然进入循环，在循环内处理多余任务
	while (true)
	{
		if (m_hasExitSignal)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_taskMap.clear();
			break;
		}

		//取任务，有任务执行，没有任务阻塞
		PopToCurTask();

		//如果当前有退出信号，则清空等待队列同时把当前任务停止
		if (m_hasExitSignal)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_taskMap.clear();
			break;
		}

		if (m_spCurTask != nullptr)
		{
			m_spCurTask->DoTask();
			Semaphore* waitForSend = m_spCurTask->GetWaitForSendHandle();
			if (waitForSend != nullptr)
			{
				waitForSend->event();
			}
		}
		else if (m_waitForEndSignal)
		{
			break;
		}
		
		//任务执行完之后清空指针执行task析构
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_spCurTask = nullptr;
			m_spCurTaskBk = nullptr;
			m_curTaskLevel = 0;
		}
		m_semaphore->wait();
	}
}

void CTaskThread::StopAllTaskUnlock()
{
	for (auto itListTask = m_taskMap.begin(); itListTask != m_taskMap.end(); ++itListTask)
	{
		auto listTask = itListTask->second;
		for (auto itTask = listTask.begin(); itTask != listTask.end(); ++itTask)
		{
			(*itTask)->StopTask();
		}
	}
	//如果有还未执行完毕的任务
	StopCurTask();
}

void CTaskThread::HandlePostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel)
{
	m_taskMap[taskLevel].push_back(spTask);
	//如果添加任务的优先级高于当前任务则当前任务停止
	if (m_spCurTask != nullptr && taskLevel > m_curTaskLevel)
	{
		//如果该任务的属性是被顶掉后重做，则先添加此任务，判空是防止当前任务还没来得及退出就添加了多个优先级更高的任务
		if (m_spCurTaskBk != nullptr && m_spCurTask->ReExecute())
		{
			m_taskMap[m_curTaskLevel].push_front(m_spCurTaskBk);
			m_spCurTaskBk = nullptr;
			m_semaphore->signal();
		}
		StopCurTask();
	}
	m_semaphore->signal();
}

bool CTaskThread::HasTask()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return !m_taskMap.empty();
}

void CTaskThread::StopCurTask()
{
	if (m_spCurTask != nullptr)
	{
		m_spCurTask->StopTask();
	}
}

void CTaskThread::PopToCurTask()
{
	if (!HasTask())
	{
		m_spCurTask = nullptr;
		m_spCurTaskBk = nullptr;
		m_curTaskLevel = 0;
		//::ResetEvent(m_semaphore);
		return;
	}
	//如果有任务，取出最后一个优先级的队列任务集合
	std::unique_lock<std::mutex> lock(m_mutex);
	auto itTaskList = --(m_taskMap.end());
	std::list<std::shared_ptr<CTask>>& listTask = itTaskList->second;
	//将最后一个优先级队列的首个任务取出，只要有集合就必须有任务
	m_spCurTask = listTask.front();
	m_spCurTaskBk.reset(m_spCurTask->Clone());
	m_curTaskLevel = itTaskList->first;
	listTask.pop_front();
	//如果该级别队列中没有任务则删除该级别在map中的节点
	if (listTask.empty())
	{
		m_taskMap.erase(itTaskList);
	}
}

void CTaskThread::StopTask(int32_t taskId, int32_t taskLevel)
{
	if (m_spCurTask != nullptr && m_spCurTask->GetTaskId() == taskId)
	{
		StopCurTask();
	}
	if (taskLevel != 0)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		auto itListTask = m_taskMap.find(taskLevel);
		if (itListTask != m_taskMap.end())
		{
			StopTaskInList(itListTask->second, taskId);
		}
	}
	else
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		for (auto itListTask = m_taskMap.begin(); itListTask != m_taskMap.end(); ++itListTask)
		{
			StopTaskInList(itListTask->second, taskId);
		}
	}
}

void CTaskThread::StopTaskInList(const std::list<std::shared_ptr<CTask>>& taskList, int32_t taskId)
{
	for (auto itTask = taskList.begin(); itTask != taskList.end(); ++itTask)
	{
		if ((*itTask)->GetTaskId() == taskId)
		{
			(*itTask)->StopTask();
		}
	}
}

void CTaskThread::StopAllTask()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	StopAllTaskUnlock();
}

void CTaskThread::GetWaitTaskInfo(std::map<int32_t, int32_t>& taskCountMap)
{
	taskCountMap.clear();
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		for (auto itTaskList = m_taskMap.begin(); itTaskList != m_taskMap.end(); ++itTaskList)
		{
			taskCountMap[itTaskList->first] = (int32_t)itTaskList->second.size();
		}
	}
}

int32_t CTaskThread::GetWaitTaskCount()
{
	int32_t count = 0;
	std::unique_lock<std::mutex> lock(m_mutex);
	for (auto itTaskList = m_taskMap.begin(); itTaskList != m_taskMap.end(); ++itTaskList)
	{
		count += (int32_t)itTaskList->second.size();
	}
	return count;
}

int32_t CTaskThread::GetCurTaskLevel()
{
	return m_curTaskLevel;
}

void CTaskThread::WaitForEnd()
{
	m_waitForEndSignal = true;
	m_semaphore->signal();
	WaitForExit();
}