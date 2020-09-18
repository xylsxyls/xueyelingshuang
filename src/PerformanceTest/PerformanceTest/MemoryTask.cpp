#include "MemoryTask.h"
#include "PerformanceManager.h"
#include "CSystem/CSystemAPI.h"

MemoryTask::MemoryTask() :
m_exit(false),
m_pid(0)
{

}

void MemoryTask::DoTask()
{
	while (!m_exit)
	{
		PerformanceManager::instance().modifyMemoryPerformance(m_pid, PerformanceManager::instance().MemoryOccupation(m_pid));
		CSystem::Sleep(50);
	}
}

void MemoryTask::StopTask()
{
	m_exit = true;
}

void MemoryTask::setParam(uint32_t pid)
{
	m_pid = pid;
}