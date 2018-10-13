#include "ProcessServer.h"
#include "ServerReceiveCallback.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class WorkTask : public CTask
{
public:
    WorkTask():
        m_server(nullptr),
        m_memory(nullptr)
    {

    }

public:
    void DoTask()
    {
        m_server->m_callback->receive((char*)m_memory, 6);
    }

    void setServer(ProcessServer* server)
    {
        m_server = server;
    }

    void setMemory(void* memory)
    {
        m_memory = memory;
    }

protected:
    ProcessServer* m_server;
    void* m_memory;
};

class ListenTask : public CTask
{
public:
    ListenTask():
        m_server(nullptr),
        m_event(nullptr)
    {
        m_event = ::CreateEvent(NULL, FALSE, FALSE, "ProcessWorkEvent");
    }
public:
    void DoTask()
    {
        while (true)
        {
            //wait
            ::WaitForSingleObject(m_event, INFINITE);

            void* memory = nullptr;
            {
                //readmemory
                ReadLock readLock(*(m_server->m_memory));
                memory = m_server->m_memory->memory();
            }

            //sendtothread
            std::shared_ptr<WorkTask> spTask;
            WorkTask* workTask = new WorkTask;
            workTask->setServer(m_server);
            workTask->setMemory(memory);
            spTask.reset(workTask);
            CTaskThreadManager::Instance().GetThreadInterface(m_server->m_workThreadId)->PostTask(spTask);
        }
    }

    void setServer(ProcessServer* server)
    {
        m_server = server;
    }

protected:
    ProcessServer* m_server;
    HANDLE m_event;
};


ProcessServer::ProcessServer():
m_callback(nullptr),
m_memory(nullptr),
m_processMutex(nullptr),
m_listenThreadId(0),
m_workThreadId(0)
{
    m_processMutex = new ProcessReadWriteMutex("ProcessWorkMutex");
    m_listenThreadId = CTaskThreadManager::Instance().Init();
    m_workThreadId = CTaskThreadManager::Instance().Init();
}

void ProcessServer::listen(const std::string& mapName, ServerReceiveCallback* receive)
{
    m_callback = receive;
    m_memory = new SharedMemory(mapName, 1024);
    ListenTask* listenTask = new ListenTask;
    listenTask->setServer(this);
    std::shared_ptr<ListenTask> spTask;
    spTask.reset(listenTask);
    CTaskThreadManager::Instance().GetThreadInterface(m_listenThreadId)->PostTask(spTask);
}