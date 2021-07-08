#include "CopyTask.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "Semaphore/SemaphoreAPI.h"

CopyTask::CopyTask() :
m_areaAssign(nullptr),
m_memoryMap(nullptr),
m_assignQueue(nullptr),
m_assignSemaphore(nullptr),
m_receiveQueue(nullptr),
m_receiveSemaphore(nullptr),
m_exit(false)
{
	
}

void CopyTask::DoTask()
{
	void* areaAssign = m_areaAssign->writeWithoutLock();
	if (areaAssign == nullptr)
	{
		printf("CopyTask areaAssign nullptr\n");
	}
	int32_t areaCount = *(int32_t*)areaAssign;

	while (!m_exit)
	{
		m_assignSemaphore->wait();
		if (m_exit)
		{
			break;
		}
		int32_t assign = 0;
		m_assignQueue->pop(&assign);

		void* memory = m_memoryMap->find(assign)->second->writeWithoutLock();
		if (memory == nullptr)
		{
			printf("CopyTask memory nullptr\n");
		}

		int32_t length = *((int32_t*)memory + 1) - 4 + 12;
		char* receive = new char[length + 1];
		receive[length] = 0;
		::memcpy(receive, (char*)memory, length);

		int32_t index = -1;
		while (index++ != areaCount - 1)
		{
			if (*((int32_t*)areaAssign + 1 + index * 2) == assign)
			{
				*((int32_t*)areaAssign + 1 + index * 2 + 1) = 0;
				break;
			}
		}

		m_receiveQueue->push(receive);
		m_receiveSemaphore->signal();
	}
}

void CopyTask::StopTask()
{
	m_exit = true;
	m_assignSemaphore->signal();
}

void CopyTask::setParam(SharedMemory* areaAssign,
	std::map<int32_t, std::shared_ptr<SharedMemory>>* memoryMap,
	LockFreeQueue<int32_t>* assignQueue,
	Semaphore* assignSemaphore,
	LockFreeQueue<char*>* receiveQueue,
	Semaphore* receiveSemaphore)
{
	m_areaAssign = areaAssign;
	m_memoryMap = memoryMap;
	m_assignQueue = assignQueue;
	m_assignSemaphore = assignSemaphore;
	m_receiveQueue = receiveQueue;
	m_receiveSemaphore = receiveSemaphore;
}