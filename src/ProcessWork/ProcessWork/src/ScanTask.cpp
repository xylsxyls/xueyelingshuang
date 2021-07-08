#include "ScanTask.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "AtomicMath/AtomicMathAPI.h"

ScanTask::ScanTask():
m_areaAssign(nullptr),
m_exit(false)
{

}

void ScanTask::DoTask()
{
	void* areaAssign = m_areaAssign->writeWithoutLock();
	if (areaAssign == nullptr)
	{
		printf("ScanTask areaAssign nullptr\n");
	}

	int32_t areaCount = *(int32_t*)areaAssign;

	while (!m_exit)
	{
		int32_t index = -1;
		while (!m_exit && (index++ != areaCount - 1))
		{
			int32_t assignPoint = *((int32_t*)areaAssign + 1 + index * 2 + 1);
			if (assignPoint == 0)
			{
				continue;
			}
			Sleep(1000);
			if (m_exit)
			{
				break;
			}
			int32_t currentAssignPoint = *((int32_t*)areaAssign + 1 + index * 2 + 1);
			if (assignPoint != currentAssignPoint)
			{
				continue;
			}
			int32_t addAssignPoint = AtomicMath::selfAdd((int32_t*)areaAssign + 1 + index * 2 + 1, areaCount);
			Sleep(5000);
			if (m_exit)
			{
				break;
			}
			int32_t lastAssignPoint = *((int32_t*)areaAssign + 1 + index * 2 + 1);
			if (lastAssignPoint != addAssignPoint)
			{
				continue;
			}
			//»Ö¸´»µÇø
			//RCSend("»Ö¸´»µÇø£¬index = %d", index);
			*((int32_t*)areaAssign + 1 + index * 2 + 1) = 0;
		}
	}
}

void ScanTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void ScanTask::setParam(SharedMemory* areaAssign)
{
	m_areaAssign = areaAssign;
}

void ScanTask::Sleep(int32_t timeout)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(timeout);
}