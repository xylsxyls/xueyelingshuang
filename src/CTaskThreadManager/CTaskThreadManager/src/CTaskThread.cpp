#include "CTaskThread.h"
#include <assert.h>
#include <windows.h>

CTaskThread::CTaskThread(int32_t threadId) :
m_threadId(threadId)
{

}

bool CTaskThread::CreateThread()
{
    //�����Ƚ�m_hasExitSignal��Ϊfalse������һ�����̺߳������˳���
    std::thread* pThread = new std::thread(std::bind(&CTaskThread::WorkThread, this));
    if (pThread == nullptr)
    {
        m_hasExitSignal = true;
        return false;
    }
    m_spWorkThread.reset(pThread);
	m_waitEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
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
    //�������˳��źž������ѵ�ǰ����ֹͣ
	StopCurTask();
	::SetEvent(m_waitEvent);
}

void CTaskThread::WaitForExit()
{
    //����߳���������
    if (m_spWorkThread != nullptr)
    {
        m_spWorkThread->join();
        m_spWorkThread.reset(nullptr);
		::CloseHandle(m_waitEvent);
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

	HANDLE waitForSend = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	spTask->SetWaitForSendHandle(waitForSend);
    PostTask(spTask, taskLevel);
    
    bool inWhile = true;
    while (inWhile)
    {
        if (spTask.use_count() == 1)
        {
            break;
        }
        ::WaitForSingleObject(waitForSend, 50);
    }
	::CloseHandle(waitForSend);
	spTask->SetWaitForSendHandle(nullptr);
}

void CTaskThread::WorkThread()
{
    //���˳��źţ����Ƕ������еȴ�����ʱ��Ȼ����ѭ������ѭ���ڴ����������
	while (true)
    {
		if (m_hasExitSignal)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_taskMap.clear();
			break;
		}
		//ȡ����������ִ�У�û����������
		PopToCurTask();

		//�����ǰ���˳��źţ�����յȴ�����ͬʱ�ѵ�ǰ����ֹͣ
		if (m_hasExitSignal)
		{
			std::unique_lock<std::mutex> lock(m_mutex);
			m_taskMap.clear();
			break;
		}

        if (m_spCurTask != nullptr)
        {
            m_spCurTask->DoTask();
			HANDLE waitForSend = m_spCurTask->GetWaitForSendHandle();
			if (waitForSend != nullptr)
			{
				::SetEvent(waitForSend);
			}
        }
		else
		{
			if (m_waitForEndSignal)
			{
				break;
			}
			::WaitForSingleObject(m_waitEvent, INFINITE);
			if (m_waitForEndSignal)
			{
				break;
			}
		}
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
    //����л�δִ����ϵ�����
	StopCurTask();
}

void CTaskThread::HandlePostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel)
{
	::SetEvent(m_waitEvent);
	m_taskMap[taskLevel].push_back(spTask);
	//��������������ȼ����ڵ�ǰ������ǰ����ֹͣ
	if (m_spCurTask != nullptr && taskLevel > m_curTaskLevel)
	{
		//���������������Ǳ�������������������Ӵ������п��Ƿ�ֹ��ǰ����û���ü��˳�������˶�����ȼ����ߵ�����
		if (m_spCurTaskBk != nullptr && m_spCurTask->ReExecute())
		{
			m_taskMap[m_curTaskLevel].push_front(m_spCurTaskBk);
			m_spCurTaskBk = nullptr;
		}
		StopCurTask();
	}
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
		::ResetEvent(m_waitEvent);
		return;
	}
	//���������ȡ�����һ�����ȼ��Ķ������񼯺�
	std::unique_lock<std::mutex> lock(m_mutex);
	auto itTaskList = --(m_taskMap.end());
	std::list<std::shared_ptr<CTask>>& listTask = itTaskList->second;
	//�����һ�����ȼ����е��׸�����ȡ��
	m_spCurTask = listTask.front();
	m_spCurTaskBk.reset(m_spCurTask->Clone());
	m_curTaskLevel = itTaskList->first;
	//ֻҪ�м��Ͼͱ���������
	if (m_spCurTask == nullptr)
	{
		assert(0);
	}
	listTask.pop_front();
	//����ü��������û��������ɾ���ü�����map�еĽڵ�
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
	::SetEvent(m_waitEvent);
	WaitForExit();
}