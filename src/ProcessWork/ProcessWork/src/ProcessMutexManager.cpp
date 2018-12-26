#include "ProcessMutexManager.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "ProcessHelper.h"

ProcessMutexManager::ProcessMutexManager()
{

}

ProcessMutexManager& ProcessMutexManager::instance()
{
	static ProcessMutexManager s_processMutexManager;
	return s_processMutexManager;
}

ProcessReadWriteMutex* ProcessMutexManager::positionMutex(int32_t pid)
{
	ProcessReadWriteMutex* result = nullptr;
	auto itMutex = m_positionMutex.find(pid);
	if (itMutex == m_positionMutex.end())
	{
		result = new ProcessReadWriteMutex(ProcessHelper::positionMutexName());
		m_positionMutex[pid] = result;
		return result;
	}
	return itMutex->second;
}