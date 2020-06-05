#include "ReadTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "ReceiveTask.h"

ReadTask::ReadTask():
m_exit(false),
m_callback(nullptr),
m_readSemaphore(nullptr),
m_readEndSemaphore(nullptr),
m_area(nullptr),
m_memoryMap(nullptr),
m_receiveThread(nullptr)
{
	
}

void ReadTask::DoTask()
{
	m_readEndSemaphore->processSignal();
	void* area = m_area->writeWithoutLock();
	while (true)
	{
		m_readSemaphore->processWait();
		if (m_exit)
		{
			break;
		}
		int32_t assign = 0;
		assign = *((int32_t*)area + 1);
		m_readEndSemaphore->processSignal();
		std::shared_ptr<ReceiveTask> spReceiveTask(new ReceiveTask);
		spReceiveTask->setParam(assign, m_callback, m_memoryMap);
		m_receiveThread->PostTask(spReceiveTask);
	}
}

void ReadTask::StopTask()
{
	m_exit = true;
	m_readSemaphore->processSignal();
}

void ReadTask::setParam(ProcessReceiveCallback* callback,
	Semaphore* readSemaphore,
	Semaphore* readEndSemaphore,
	SharedMemory* area,
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap,
	const std::shared_ptr<CTaskThread>& receiveThread)
{
	m_callback = callback;
	m_readSemaphore = readSemaphore;
	m_readEndSemaphore = readEndSemaphore;
	m_area = area;
	m_memoryMap = memoryMap;
	m_receiveThread = receiveThread;
}