#include "ProcessServer.h"
#include "ServerReceiveCallback.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "WorkParam.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "ProcessHelper.h"
#include "D:\\SendToMessageTest.h"
#include "LogManager/LogManagerAPI.h"

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
		m_server->m_callback->receive((char*)m_memory, m_size, m_protocolId);
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

	void setProtocolId(int32_t protocolId)
	{
		m_protocolId = protocolId;
	}

protected:
    ProcessServer* m_server;
    void* m_memory;
	int32_t m_size;
	int32_t m_protocolId;
};

class ListenTask : public CTask
{
public:
    ListenTask():
        m_server(nullptr),
        m_semaphore(nullptr),
		m_receivePosition(0),
		m_receiveMemoryIndex(0)
    {
		m_semaphore = ::CreateSemaphore(NULL, 0, 128000001, "ProcessWorkMemorySemaphore");
    }
public:
    void DoTask()
    {
        while (true)
        {
			//wait
			//LOGINFO("WaitForSingleObject begin");
			::WaitForSingleObject(m_semaphore, INFINITE);
			//static int x = 0;
			//++x;
			//if (x % 10000 == 0)
			//{
			//	RCSend("x = %d", x);
			//}
			static int readCount = 0;
			void* position = m_server->m_position->writeWithoutLock();
			void* memory = m_server->m_memory->readWithoutLock();
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
			//int32_t receivePosition = m_receivePosition;// ProcessHelper::receivePosition(position);
			char* buffer = nullptr;
			int32_t length = 0;
			int32_t protocolId = 0;
			//LOGINFO("readMemory");
			++readCount;
			if (!ProcessHelper::readMemory(memory, m_receivePosition, &buffer, &length, &protocolId))
			{
				--readCount;
				//LOGINFO("readCount = %d", readCount);
				//LOGINFO("read nullptr");
				delete m_server->m_memory;
				SharedMemory* currentSharedMemory = nullptr;
				m_server->m_memoryQueue->pop(&currentSharedMemory);
				++m_receiveMemoryIndex;
				std::string receiveMemoryMapName = ProcessHelper::receiveMemoryMapName(m_mapName, m_receiveMemoryIndex);
				//LOGINFO("new SharedMemory mapName = %s", receiveMemoryMapName.c_str());
				m_server->m_memory = new SharedMemory(receiveMemoryMapName);
				m_receivePosition = 0;
				memory = m_server->m_memory->readWithoutLock();
				if (memory == nullptr)
				{
					//LOGERROR("currentSharedMemory->mapName() = %s, m_server->m_memory->mapName() = %s, readCount = %d", currentSharedMemory->mapName().c_str(), m_server->m_memory->mapName().c_str(), readCount);
					//LOGERROR("memory = nullptr");
				}
				//LOGINFO("read new Memory, mapName = %s", m_server->m_memory->mapName().c_str());
				++readCount;
				ProcessHelper::readMemory(memory, m_receivePosition, &buffer, &length, &protocolId);
				//LOGINFO("read new Memory end");
			}

			//{
			//	//writeposition
			//	WriteLock writeLock(*(m_server->m_position));
			//	void* position = m_server->m_position->memory();
			//	ProcessHelper::addReceivePosition(position, sizeof(int32_t) * 2 + length);
			//}
			m_receivePosition += sizeof(int32_t) * 2 + length;
			//LOGINFO("m_receivePosition = %d", m_receivePosition);

			//if (readCount % 10000 == 0)
			//{
			//	RCSend("readCount = %d", readCount);
			//}

			//sendtothread
			std::shared_ptr<WorkTask> spTask;
			WorkTask* workTask = new WorkTask;
			workTask->setServer(m_server);
			workTask->setMemory(buffer);
			workTask->setSize(length);
			workTask->setProtocolId(protocolId);
			spTask.reset(workTask);
			//LOGINFO("sendtothread");
            CTaskThreadManager::Instance().GetThreadInterface(m_server->m_workThreadId)->PostTask(spTask);
        }
    }

    void setServer(ProcessServer* server)
    {
        m_server = server;
    }

	void setMapName(const std::string& mapName)
	{
		m_mapName = mapName;
	}

protected:
    ProcessServer* m_server;
    HANDLE m_semaphore;
	int32_t m_receivePosition;
	std::string m_mapName;
	int32_t m_receiveMemoryIndex;
};

class CreateMemoryTask : public CTask
{
public:
	CreateMemoryTask():
		m_positionBeginSemaphore(nullptr),
		m_server(nullptr)
	{
		m_positionBeginSemaphore = ::CreateSemaphore(NULL, 0, 128000001, "ProcessWorkPositionBeginSemaphore");
		m_positionEndSemaphore = ::CreateSemaphore(NULL, 0, 128000001, "ProcessWorkPositionEndSemaphore");
	}

	void DoTask()
	{
		while (true)
		{
			//LOGINFO("WaitForSingleObject begin");
			::WaitForSingleObject(m_positionBeginSemaphore, INFINITE);
			{
				//WriteLock writeLock(*m_server->m_position);
				//由客户端锁住
				//LOGINFO("CreateMemoryTask begin");
				void* position = m_server->m_position->writeWithoutLock();
				int32_t& sendMemoryIndex = ProcessHelper::sendMemoryIndex(position);
				SharedMemory* sharedMemory = new SharedMemory(ProcessHelper::sendMemoryMapName(m_mapName, sendMemoryIndex + 1), WorkParam::STEP_LENGTH);
				m_server->m_memoryQueue->push(sharedMemory);
				++sendMemoryIndex;
				//LOGINFO("sharedMemory->mapName() = %s", sharedMemory->mapName().c_str());
			}
			//LOGINFO("ReleaseSemaphore begin");
			::ReleaseSemaphore(m_positionEndSemaphore, 1, NULL);
			//static int xx = 0;
			//xx++;
			//if (xx == 2)
			//{
			//	break;
			//}
		}
	}

	void setServer(ProcessServer* server)
	{
		m_server = server;
	}

	void setMapName(const std::string& mapName)
	{
		m_mapName = mapName;
	}

	HANDLE m_positionBeginSemaphore;
	HANDLE m_positionEndSemaphore;
	ProcessServer* m_server;
	std::string m_mapName;
};

ProcessServer::ProcessServer():
m_callback(nullptr),
m_memory(nullptr),
m_processMutex(nullptr),
m_listenThreadId(0),
m_createThreadId(0),
m_currentPosition(0),
m_workThreadId(0),
m_memoryQueue(nullptr)
{
    m_processMutex = new ProcessReadWriteMutex("ProcessWorkMutex");
	m_memoryQueue = new LockFreeQueue<SharedMemory*>;
    m_listenThreadId = CTaskThreadManager::Instance().Init();
	m_createThreadId = CTaskThreadManager::Instance().Init();
    m_workThreadId = CTaskThreadManager::Instance().Init();
}

void ProcessServer::listen(const std::string& mapName, ServerReceiveCallback* receive)
{
    m_callback = receive;
	m_position = new SharedMemory(ProcessHelper::positionMapName(mapName));
	if (m_position->writeWithoutLock() == nullptr)
	{
		delete m_position;
		//LOGINFO("new position");
		m_position = new SharedMemory(ProcessHelper::positionMapName(mapName), sizeof(int32_t) * 4);
		{
			//writeposition
			WriteLock writeLock(*m_position);
			void* position = m_position->memory();
			::memset(position, 0, 16);
		}
	}
	else
	{
		//destroy position
		//LOGINFO("destroy position");
	}

	void* position = m_position->writeWithoutLock();
	std::string receiveMapName = ProcessHelper::receiveMemoryMapName(mapName, position);
	//LOGINFO("new memory");
	m_memory = new SharedMemory(receiveMapName.c_str(), WorkParam::STEP_LENGTH);
	m_memoryQueue->push(m_memory);
	
	//LOGINFO("CreateMemoryTask");
	CreateMemoryTask* createMemoryTask = new CreateMemoryTask;
	createMemoryTask->setServer(this);
	createMemoryTask->setMapName(mapName);
	std::shared_ptr<CreateMemoryTask> spCreateMemoryTask;
	spCreateMemoryTask.reset(createMemoryTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_createThreadId)->PostTask(spCreateMemoryTask);

	//LOGINFO("ListenTask");
    ListenTask* listenTask = new ListenTask;
    listenTask->setServer(this);
	listenTask->setMapName(mapName);
    std::shared_ptr<ListenTask> spListenTask;
	spListenTask.reset(listenTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_listenThreadId)->PostTask(spListenTask);
}