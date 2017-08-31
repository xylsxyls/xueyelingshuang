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
    bool threadExist = false;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        threadExist = (m_spThreadMap.find(threadId) != m_spThreadMap.end());
    }
    if (!threadExist)
    {
        CTaskThread* pTaskThread = new CTaskThread(threadId);
        if (pTaskThread != nullptr)
        {
            std::shared_ptr<CTaskThread> spTaskThread;
            spTaskThread.reset(pTaskThread);
            if (pTaskThread->CreateThread())
            {
                std::unique_lock<std::mutex> lock(m_mutex);
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
    std::shared_ptr<CTaskThread>* pTaskThread = NULL;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto itThread = m_spThreadMap.find(threadId);
        if (itThread != m_spThreadMap.end())
        {
            pTaskThread = &(itThread->second);
        }
    }
    if (pTaskThread != NULL)
    {
        std::shared_ptr<CTaskThread>& spTaskThread = *pTaskThread;
        if (spTaskThread != NULL)
        {
            spTaskThread->DeleteThread();
            std::unique_lock<std::mutex> lock(m_mutex);
            m_spThreadMap.erase(m_spThreadMap.find(threadId));
        }
        else
        {
            assert(0);
        }
    }
}

void CTaskThreadManager::UninitAll()
{
    std::map<__int32, std::shared_ptr<CTaskThread>>::iterator itThread;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        itThread = m_spThreadMap.begin();
    }
    bool inWhile = true;
    while (inWhile)
    {
        std::shared_ptr<CTaskThread>* pTaskThread = NULL;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (itThread == m_spThreadMap.end())
            {
                break;
            }
            pTaskThread = &(itThread->second);
        }
        
        if (pTaskThread != NULL)
        {
            std::shared_ptr<CTaskThread>& taskThread = *pTaskThread;
            if (taskThread != NULL)
            {
                taskThread->m_hasExitSignal = true;
                if (taskThread->m_spCurTask != NULL)
                {
                    taskThread->m_spCurTask->StopTask(false);
                }
                //taskThread->StopAllTask();
                /*std::unique_lock<std::mutex> lock(m_mutex);
                itThread = m_spThreadMap.erase(itThread);
                continue;*/
            }
        }
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            ++itThread;
        }
    }

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        itThread = m_spThreadMap.begin();
    }
    while (inWhile)
    {
        std::shared_ptr<CTaskThread>* pTaskThread = NULL;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (itThread == m_spThreadMap.end())
            {
                break;
            }
            pTaskThread = &(itThread->second);
        }

        if (pTaskThread != NULL)
        {
            std::shared_ptr<CTaskThread>& taskThread = *pTaskThread;
            if (taskThread != NULL)
            {
                //taskThread->DeleteThread();

                //如果线程正常启动
                if (taskThread->m_spWorkThread != NULL)
                {
                    taskThread->m_spWorkThread->join();

                    std::map<__int32, __int32> taskCountMap;
                    taskThread->GetWaitTaskCount(taskCountMap);
                    if (taskCountMap.size() != 0)
                    {
                        int x = 3;
                        x = 3;
                    }
                    taskThread->m_spWorkThread.reset(NULL);
                }

                std::unique_lock<std::mutex> lock(m_mutex);
                itThread = m_spThreadMap.erase(itThread);
                continue;
            }
        }
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            ++itThread;
        }
    }
}

std::shared_ptr<CTaskThread> CTaskThreadManager::GetThreadInterface(__int32 threadId)
{
    if (threadId < 1)
    {
        return std::shared_ptr<CTaskThread>();
    }
    std::unique_lock<std::mutex> lock(m_mutex);
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