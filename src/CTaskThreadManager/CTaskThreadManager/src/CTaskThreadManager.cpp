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

bool CTaskThreadManager::Init(int32_t threadId)
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

void CTaskThreadManager::Uninit(int32_t threadId)
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
            spTaskThread->DestroyThread();
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
    std::map<int32_t, std::shared_ptr<CTaskThread>>::iterator itThread;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        itThread = m_spThreadMap.begin();
    }
    //该map循环两次，第一次停掉所有任务，第二次等待线程结束
    int32_t times = 1;
    bool inWhile = true;
    while (inWhile)
    {
        std::shared_ptr<CTaskThread> spTaskThread = NULL;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (itThread == m_spThreadMap.end())
            {
                if (times == 1)
                {
                    itThread = m_spThreadMap.begin();
                    times = 2;
                    continue;
                }
                else
                {
                    break;
                }
            }
            spTaskThread = itThread->second;
        }
        
        if (spTaskThread != NULL)
        {
            if (times == 1)
            {
                spTaskThread->SetExitSignal();
                std::unique_lock<std::mutex> lock(m_mutex);
                ++itThread;
            }
            else
            {
                spTaskThread->WaitForExit();
                std::unique_lock<std::mutex> lock(m_mutex);
                itThread = m_spThreadMap.erase(itThread);
            }
        }
    }
}

std::shared_ptr<CTaskThread> CTaskThreadManager::GetThreadInterface(int32_t threadId)
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
    CPrintTask(int32_t taskId) :
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
    CPrintTask2(int32_t taskId) :
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