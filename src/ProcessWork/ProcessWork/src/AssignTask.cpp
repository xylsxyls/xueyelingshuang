#include "AssignTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"

AssignTask::AssignTask():
m_exit(false),
m_assignSemaphore(nullptr),
m_assignEndSemaphore(nullptr),
m_area(nullptr),
m_memoryMap(nullptr)
{
	
}

void AssignTask::DoTask()
{
	void* area = m_area->writeWithoutLock();
	while (true)
	{
		m_assignSemaphore->processWait();
		if (m_exit)
		{
			break;
		}
		int32_t assign = 0;
		while (true)
		{
			for (auto itMemory = m_memoryMap->begin(); itMemory != m_memoryMap->end(); ++itMemory)
			{
				if (!(*(itMemory->second.second)))
				{
					assign = itMemory->first;
					*(itMemory->second.second) = true;
					break;
				}
			}
			if (assign != 0)
			{
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		*(int32_t*)area = assign;
		m_assignEndSemaphore->processSignal();
	}
}

void AssignTask::StopTask()
{
	m_exit = true;
	m_assignSemaphore->processSignal();
}

void AssignTask::setParam(Semaphore* assignSemaphore,
	Semaphore* assignEndSemaphore,
	SharedMemory* area,
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap)
{
	m_assignSemaphore = assignSemaphore;
	m_assignEndSemaphore = assignEndSemaphore;
	m_area = area;
	m_memoryMap = memoryMap;
}