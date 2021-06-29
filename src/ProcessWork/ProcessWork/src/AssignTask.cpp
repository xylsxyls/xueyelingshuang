#include "AssignTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"

AssignTask::AssignTask():
m_exit(false),
m_assignSemaphore(nullptr),
m_assignEndSemaphore(nullptr),
m_areaAssign(nullptr)
{
	
}

void AssignTask::DoTask()
{
	void* areaAssign = m_areaAssign->writeWithoutLock();
	if (areaAssign == nullptr)
	{
		printf("AssignTask area nullptr\n");
	}
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
		*(int32_t*)areaAssign = assign;
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
	SharedMemory* areaAssign)
{
	m_assignSemaphore = assignSemaphore;
	m_assignEndSemaphore = assignEndSemaphore;
	m_areaAssign = areaAssign;
}