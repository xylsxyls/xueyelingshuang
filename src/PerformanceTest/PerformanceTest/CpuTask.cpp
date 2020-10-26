#include "CpuTask.h"
#include "PerformanceManager.h"
#include "CSystem/CSystemAPI.h"

CpuTask::CpuTask():
m_exit(false),
m_pid(0)
{

}

void CpuTask::DoTask()
{
#ifdef _MSC_VER
	PerformanceManager::instance().addCPUPid(m_pid);
#endif
	while (!m_exit)
	{
		PerformanceManager::instance().modifyCpuPerformance(m_pid, PerformanceManager::instance().CPUOccupation(m_pid));
		CSystem::Sleep(800);
	}
}

void CpuTask::StopTask()
{
	m_exit = true;
}

void CpuTask::setParam(uint32_t pid)
{
	m_pid = pid;
}