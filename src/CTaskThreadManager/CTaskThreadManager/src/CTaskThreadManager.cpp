#include "CTaskThreadManager.h"
#include <assert.h>

CTaskThreadManager::CTaskThreadManager():
m_threadId(0)
{

}

CTaskThreadManager::~CTaskThreadManager()
{

}

CTaskThreadManager& CTaskThreadManager::Instance()
{
    static CTaskThreadManager taskManager;
    return taskManager;
}

uint32_t CTaskThreadManager::Init()
{
	uint32_t threadId = GetThreadId();
	CTaskThread* pTaskThread = new CTaskThread(threadId);
	if (pTaskThread == nullptr)
	{
		return 0;
	}
	std::shared_ptr<CTaskThread> spTaskThread;
	spTaskThread.reset(pTaskThread);
	if (pTaskThread->CreateThread())
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_spThreadMap[threadId] = spTaskThread;
		return threadId;
	}
    return 0;
}

void CTaskThreadManager::WaitForEnd(uint32_t threadId)
{
	if (threadId == 0)
	{
		return;
	}
	m_mutex.lock();
	auto itThread = m_spThreadMap.find(threadId);
	if (itThread != m_spThreadMap.end())
	{
		std::shared_ptr<CTaskThread>& spTaskThread = itThread->second;
		if (spTaskThread != nullptr)
		{
			m_mutex.unlock();
			spTaskThread->WaitForEnd();
			m_mutex.lock();
		}
		m_spThreadMap.erase(m_spThreadMap.find(threadId));
	}
	m_mutex.unlock();
}

void CTaskThreadManager::Uninit(uint32_t threadId)
{
    if (threadId == 0)
    {
        return;
    }
	m_mutex.lock();
	auto itThread = m_spThreadMap.find(threadId);
	if (itThread != m_spThreadMap.end())
	{
		std::shared_ptr<CTaskThread>& spTaskThread = itThread->second;
		if (spTaskThread != nullptr)
		{
			m_mutex.unlock();
			spTaskThread->DestroyThread();
			m_mutex.lock();
		}
		m_spThreadMap.erase(m_spThreadMap.find(threadId));
	}
	m_mutex.unlock();
}

void CTaskThreadManager::UninitAll()
{
    //该map循环两次，第一次停掉所有任务，第二次等待线程结束
	m_mutex.lock();
	for (auto itThread = m_spThreadMap.begin(); itThread != m_spThreadMap.end(); ++itThread)
	{
		std::shared_ptr<CTaskThread>& spTaskThread = itThread->second;
		if (spTaskThread != nullptr)
		{
			m_mutex.unlock();
			spTaskThread->SetExitSignal();
			m_mutex.lock();
		}
	}
	for (auto itThread = m_spThreadMap.begin(); itThread != m_spThreadMap.end(); ++itThread)
	{
		std::shared_ptr<CTaskThread>& spTaskThread = itThread->second;
		if (spTaskThread != nullptr)
		{
			m_mutex.unlock();
			spTaskThread->WaitForExit();
			m_mutex.lock();
		}
	}
	m_spThreadMap.clear();
	m_mutex.unlock();
}

std::shared_ptr<CTaskThread> CTaskThreadManager::GetThreadInterface(uint32_t threadId)
{
    if (threadId == 0)
    {
        return std::shared_ptr<CTaskThread>();
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    auto itThread = m_spThreadMap.find(threadId);
    if (itThread == m_spThreadMap.end())
    {
		return std::shared_ptr<CTaskThread>();
    }
	return itThread->second;
}

int32_t CTaskThreadManager::Count()
{
	std::unique_lock<std::mutex> lock(m_mutex);
	return (int32_t)m_spThreadMap.size();
}

uint32_t CTaskThreadManager::GetThreadId()
{
	return ++m_threadId;
}

//class CPrintTask : public CTask
//{
//public:
//    CPrintTask(int32_t taskId) :
//        CTask(taskId)
//    {
//
//    }
//    virtual void DoTask()
//    {
//        printf("DoTaskBegin\n");
//        std::chrono::milliseconds dura(5000);
//        std::this_thread::sleep_for(dura);
//        printf("DoTaskEnd\n");
//    }
//    virtual void StopTask(bool ifChoke)
//    {
//        ifChoke = ifChoke;
//        printf("StopTask\n");
//        return;
//    }
//    virtual bool ReExecute()
//    {
//        printf("ReExecute\n");
//        return true;
//    }
//};
//
//class CPrintTask2 : public CTask
//{
//public:
//    CPrintTask2(int32_t taskId) :
//        CTask(taskId)
//    {
//
//    }
//    virtual void DoTask()
//    {
//        printf("DoTaskBegin2\n");
//        std::chrono::milliseconds dura(5000);
//        std::this_thread::sleep_for(dura);
//        printf("DoTaskEnd2\n");
//    }
//    virtual void StopTask(bool ifChoke)
//    {
//        ifChoke = ifChoke;
//        printf("StopTask2\n");
//        return;
//    }
//    virtual bool ReExecute()
//    {
//        printf("ReExecute2\n");
//        return true;
//    }
//};
//
//int main(){
//	CTaskThreadManager::Instance().Init(1);
//	CTaskThreadManager::Instance().Init(2);
//	std::shared_ptr<CPrintTask> printTask;
//	CPrintTask* pPrintTask = new CPrintTask(100);
//	std::shared_ptr<CPrintTask2> printTask2;
//	CPrintTask2* pPrintTask2 = new CPrintTask2(100);
//	if (pPrintTask != nullptr)
//	{
//		printTask.reset(pPrintTask);
//		printTask2.reset(pPrintTask2);
//        std::chrono::milliseconds dura(2000);
//        std::this_thread::sleep_for(dura);
//	}
//	getchar();
//	return 0;
//}