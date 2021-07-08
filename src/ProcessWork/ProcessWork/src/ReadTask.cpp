#include "ReadTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CSystem/CSystemAPI.h"

ReadTask::ReadTask():
m_readSemaphore(nullptr),
m_areaCount(0),
m_areaRead(nullptr),
m_assignQueue(nullptr),
m_assignSemaphore(nullptr),
m_exit(false)
{
	
}

void ReadTask::DoTask()
{
	void* areaRead = m_areaRead->writeWithoutLock();
	if (areaRead == nullptr)
	{
		printf("ReadTask areaRead nullptr\n");
	}

	while (true)
	{
		m_readSemaphore->processWait();
		if (m_exit)
		{
			break;
		}

		int32_t& read = *((int32_t*)areaRead + 1);
		int32_t* beginReadPtr = (int32_t*)areaRead + 2;
		volatile int32_t& readPoint = *(beginReadPtr + read * 2 + 1);
		volatile int32_t& readAssign = *(beginReadPtr + read * 2);
		int32_t readCount = 0;
		int32_t beginReadTime = 0;
		bool isFind = true;
		while ((readPoint == 0 || readAssign == 0) && !m_exit)
		{
			++readCount;
			if (readCount == 10)
			{
				beginReadTime = CSystem::GetTickCount();
			}
			else if (readCount > 10)
			{
				//超过100毫秒仍然读不到正常数据时说明发送程序抢占到节点后崩溃了
				if (CSystem::GetTickCount() - beginReadTime > 5000)
				{
					read += 1;
					if (read == m_areaCount)
					{
						read = 0;
					}
					readAssign = 0;
					readPoint = 0;
					printf("find error point\n");
					isFind = false;
					break;
				}
			}
		}
		if (m_exit)
		{
			break;
		}
		if (!isFind)
		{
			continue;
		}
		
		int32_t assign = readAssign;
		m_assignQueue->push(assign);
		m_assignSemaphore->signal();

		*(beginReadPtr + read * 2) = 0;
		*(beginReadPtr + read * 2 + 1) = 0;
		read += 1;
		if (read == m_areaCount)
		{
			read = 0;
		}
	}
}

void ReadTask::StopTask()
{
	m_exit = true;
	m_readSemaphore->processSignal();
}

void ReadTask::setParam(Semaphore* readSemaphore,
	int32_t areaCount,
	SharedMemory* areaRead,
	LockFreeQueue<int32_t>* assignQueue,
	Semaphore* assignSemaphore)
{
	m_readSemaphore = readSemaphore;
	m_areaCount = areaCount;
	m_areaRead = areaRead;
	m_assignQueue = assignQueue;
	m_assignSemaphore = assignSemaphore;
}