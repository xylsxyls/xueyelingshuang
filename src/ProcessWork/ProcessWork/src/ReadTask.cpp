#include "ReadTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CopyTask.h"
#include "CSystem/CSystemAPI.h"

ReadTask::ReadTask():
m_exit(false),
m_callback(nullptr),
m_readSemaphore(nullptr),
m_areaRead(nullptr),
m_copyThread(nullptr),
m_receiveThread(nullptr)
{
	
}

void ReadTask::DoTask()
{
	//m_readEndSemaphore->processSignal();
	void* areaRead = m_areaRead->writeWithoutLock();
	if (areaRead == nullptr)
	{
		printf("ReadTask area nullptr\n");
	}
	int32_t areaCount = m_memoryMap->size();
	while (true)
	{
		m_readSemaphore->processWait();
		if (m_exit)
		{
			break;
		}

		while (!m_exit)
		{
			int32_t& read = *((int32_t*)areaRead + 1);
			int32_t* beginReadPtr = (int32_t*)areaRead + 2;
			int32_t& readPoint = *(beginReadPtr + read * 2 + 1);
			if (readPoint == 0)
			{
				break;
			}
			int32_t assign = *(beginReadPtr + read * 2);
			if (assign == 0)
			{
				CSystem::Sleep(100);
				if (*(beginReadPtr + read * 2) == 0)
				{
					read += 1;
					if (read == areaCount)
					{
						read = 0;
					}
					readPoint = 0;
					continue;
				}
				assign = *(beginReadPtr + read * 2);
			}
			*(beginReadPtr + read * 2) = 0;
			*(beginReadPtr + read * 2 + 1) = 0;
			read += 1;
			if (read == areaCount)
			{
				read = 0;
			}
			std::shared_ptr<CopyTask> spCopyTask(new CopyTask);
			spCopyTask->setParam(assign, m_callback, m_areaAssign, m_memoryMap, m_receiveThread);
			m_copyThread->PostTask(spCopyTask);
		}
	}
}

void ReadTask::StopTask()
{
	m_exit = true;
	m_readSemaphore->processSignal();
}

void ReadTask::setParam(std::vector<ProcessReceiveCallback*>* callback,
	Semaphore* readSemaphore,
	SharedMemory* areaAssign,
	SharedMemory* areaRead,
	std::map<int32_t, std::shared_ptr<SharedMemory>>* memoryMap,
	const std::shared_ptr<CTaskThread>& copyThread,
	const std::shared_ptr<CTaskThread>& receiveThread)
{
	m_callback = callback;
	m_readSemaphore = readSemaphore;
	m_areaAssign = areaAssign;
	m_areaRead = areaRead;
	m_memoryMap = memoryMap;
	m_copyThread = copyThread;
	m_receiveThread = receiveThread;
}