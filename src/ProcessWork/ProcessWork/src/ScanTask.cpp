#include "ScanTask.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "AtomicMath/AtomicMathAPI.h"
#include "CSystem/CSystemAPI.h"

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
				if (index == areaCount - 1)
				{
					Sleep(5000);
				}
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
			//过了一秒后发现两个Point相等则怀疑发送进程是否崩溃了
			Sleep(5000);
			if (m_exit)
			{
				break;
			}
			int32_t currentAssignPointForFiveSeconds = *((int32_t*)areaAssign + 1 + index * 2 + 1);
			if (currentAssignPoint != currentAssignPointForFiveSeconds)
			{
				continue;
			}
			//如果过了5秒发现两个Point仍然相等，则检测发送进程是否崩溃了
			if (!CSystem::processName(assignPoint).empty())
			{
				continue;
			}
			//恢复坏区
			//RCSend("恢复坏区，index = %d", index);
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