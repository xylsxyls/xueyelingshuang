#include "CTaskThreadManager.h"
#include <assert.h>

CTaskThreadManager::CTaskThreadManager()
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

bool CTaskThreadManager::Init(__int32 threadId)
{
    if (threadId < 1)
    {
        return false;
    }
    auto itThread = m_spThreadMap.find(threadId);
    if (itThread == m_spThreadMap.end())
    {
        CTaskThread* pTaskThread = new CTaskThread(threadId);
        if (pTaskThread != nullptr)
        {
            std::shared_ptr<CTaskThread> spTaskThread;
            spTaskThread.reset(pTaskThread);
            if (pTaskThread->CreateThread())
            {
                m_spThreadMap[threadId] = spTaskThread;
                return true;
            }
        }
    }
    return false;
}

void CTaskThreadManager::Uninit(__int32 threadId)
{
    if (threadId < 1)
    {
        return;
    }
    auto itThread = m_spThreadMap.find(threadId);
    if (itThread != m_spThreadMap.end())
    {
        std::shared_ptr<CTaskThread>& taskThread = itThread->second;
        if (taskThread != NULL)
        {
            taskThread->DeleteThread();
            m_spThreadMap.erase(itThread);
        }
        else
        {
            assert(0);
        }
    }
}

void CTaskThreadManager::UninitAll()
{
    for (auto itThread = m_spThreadMap.begin(); itThread != m_spThreadMap.end(); ++itThread)
    {
        std::shared_ptr<CTaskThread>& taskThread = itThread->second;
        if (taskThread != NULL)
        {
            taskThread->DeleteThread();
        }
    }
    m_spThreadMap.clear();
}

std::shared_ptr<CTaskThread> CTaskThreadManager::GetThreadInterface(__int32 threadId)
{
    if (threadId < 1)
    {
        return std::shared_ptr<CTaskThread>();
    }
    auto itThread = m_spThreadMap.find(threadId);
    if (itThread != m_spThreadMap.end())
    {
        return itThread->second;
    }
    return std::shared_ptr<CTaskThread>();
}

class CPrintTask : public CTask
{
public:
    CPrintTask(__int32 taskId) :
        CTask(taskId)
    {

    }
    virtual void DoTask()
    {
        printf("DoTaskBegin\n");
        std::chrono::milliseconds dura(5000);
        std::this_thread::sleep_for(dura);
        printf("DoTaskEnd\n");
    }
    virtual void StopTask(bool ifChoke)
    {
        ifChoke = ifChoke;
        printf("StopTask\n");
        return;
    }
    virtual bool ReExecute()
    {
        printf("ReExecute\n");
        return true;
    }
};

class CPrintTask2 : public CTask
{
public:
    CPrintTask2(__int32 taskId) :
        CTask(taskId)
    {

    }
    virtual void DoTask()
    {
        printf("DoTaskBegin2\n");
        std::chrono::milliseconds dura(5000);
        std::this_thread::sleep_for(dura);
        printf("DoTaskEnd2\n");
    }
    virtual void StopTask(bool ifChoke)
    {
        ifChoke = ifChoke;
        printf("StopTask2\n");
        return;
    }
    virtual bool ReExecute()
    {
        printf("ReExecute2\n");
        return true;
    }
};

int main(){
	/*CTaskThreadManager::Instance().Init(1);
	CTaskThreadManager::Instance().Init(2);
	std::shared_ptr<CPrintTask> printTask;
	CPrintTask* pPrintTask = new CPrintTask(100);
	std::shared_ptr<CPrintTask2> printTask2;
	CPrintTask2* pPrintTask2 = new CPrintTask2(100);
	if (pPrintTask != NULL)
	{
		printTask.reset(pPrintTask);
		printTask2.reset(pPrintTask2);
        std::chrono::milliseconds dura(2000);
        std::this_thread::sleep_for(dura);
	}
	getchar();*/
	return 0;
}