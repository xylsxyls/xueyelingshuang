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
        m_memory(nullptr),
		m_size(0)
    {

    }

public:
    void DoTask()
    {
		m_server->m_callback->receive((char*)m_memory, m_size);
		delete[] m_memory;
		m_memory = nullptr;
    }

    void setServer(ProcessServer* server)
    {
        m_server = server;
    }

    void setMemory(void* memory)
    {
        m_memory = memory;
    }

	void setSize(int32_t size)
	{
		m_size = size;
	}

protected:
    ProcessServer* m_server;
    void* m_memory;
	int32_t m_size;
};

class ListenTask : public CTask
{
public:
    ListenTask():
        m_server(nullptr),
        m_semaphore(nullptr)
    {
		m_semaphore = ::CreateSemaphore(NULL, 0, 0xffff, "ProcessWorkSemaphore");
    }
public:
    void DoTask()
    {
        while (true)
        {
			//wait
			::WaitForSingleObject(m_semaphore, INFINITE);

			int32_t writePosition = 0;
			{
				//readposition
				ReadLock readLock(*(m_server->m_position));
				void* position = m_server->m_position->memory();
				writePosition = *((int32_t*)position);
			}

            void* memory = nullptr;
            {
                //readmemory
                ReadLock readLock(*(m_server->m_memory));
                memory = m_server->m_memory->memory();
            }
			void* currentMemory = (char*)memory + m_server->m_currentPosition;
			int32_t size = *((int32_t*)currentMemory);
			char* buffer = new char[size + 1];
			buffer[size] = 0;
			::memcpy(buffer, (char*)currentMemory + sizeof(int32_t), size);

			m_server->m_currentPosition += size + sizeof(int32_t);

			//::ResetEvent(m_semaphore);

            //sendtothread
            std::shared_ptr<WorkTask> spTask;
            WorkTask* workTask = new WorkTask;
            workTask->setServer(m_server);
			workTask->setMemory(buffer);
			workTask->setSize(size);
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
    HANDLE m_semaphore;
};


ProcessServer::ProcessServer():
m_callback(nullptr),
m_memory(nullptr),
m_processMutex(nullptr),
m_listenThreadId(0),
m_currentPosition(0),
m_workThreadId(0)
{
    m_processMutex = new ProcessReadWriteMutex("ProcessWorkMutex");
    m_listenThreadId = CTaskThreadManager::Instance().Init();
    m_workThreadId = CTaskThreadManager::Instance().Init();
}

void ProcessServer::listen(const std::string& mapName, ServerReceiveCallback* receive)
{
    m_callback = receive;
	m_position = new SharedMemory(mapName + "Position", 16);
	{
		//writeposition
		WriteLock writeLock(*m_position);
		void* position = m_position->memory();
		::memset(position, 0, 16);
	}
    m_memory = new SharedMemory(mapName, 1024);
    ListenTask* listenTask = new ListenTask;
    listenTask->setServer(this);
    std::shared_ptr<ListenTask> spTask;
    spTask.reset(listenTask);
    CTaskThreadManager::Instance().GetThreadInterface(m_listenThreadId)->PostTask(spTask);
}